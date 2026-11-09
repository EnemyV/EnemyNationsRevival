#!/usr/bin/env python3
"""Build a release ZIP from a staged runtime directory and loose game data.

The loose-data directory must be the extractor's ``data/`` directory. Its
manifest is treated as a complete inventory: every entry is path-checked,
size-checked and SHA-256 checked before a package is written.
"""

from __future__ import annotations

import argparse
import hashlib
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import sys
import tempfile
import zipfile

CHUNK_SIZE = 4 * 1024 * 1024
ROOT_FILES = {"enations", "enations.exe", "iserve", "iserve.exe", "readme.txt", "vdmplay.ini"}
RUNTIME_DIRS = ("assets", "res")
RUNTIME_MANIFEST_SHA256 = "7f93d9afb920a49037de4435f7bb4b01d6ecefc35eaff61466f2e11ff8813722"
RUNTIME_MANIFEST_ENTRIES = 14


class PackageError(ValueError):
    pass


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(CHUNK_SIZE), b""):
            digest.update(block)
    return digest.hexdigest()


def _safe_regular_file(path: Path, root: Path) -> None:
    try:
        path.relative_to(root)
    except ValueError as exc:
        raise PackageError(f"path escapes its root: {path}") from exc
    if path.is_symlink():
        raise PackageError(f"symlink is not allowed: {path}")
    if not path.is_file():
        raise PackageError(f"expected a regular file: {path}")


def read_manifest(data_dir: Path, *, allow_runtime_terrain_gpu: bool = False) -> list[tuple[str, int, str]]:
    data_dir = data_dir.resolve(strict=True)
    manifest = data_dir / "manifest.txt"
    _safe_regular_file(manifest, data_dir)
    rows: list[tuple[str, int, str]] = []
    seen: set[str] = set()
    try:
        lines = manifest.read_text(encoding="utf-8").splitlines()
    except (OSError, UnicodeError) as exc:
        raise PackageError(f"cannot read UTF-8 manifest {manifest}: {exc}") from exc

    for number, line in enumerate(lines, 1):
        fields = line.split()
        if len(fields) != 3:
            raise PackageError(f"{manifest}:{number}: expected '<path> <size> <sha256>'")
        name, size_text, digest = fields
        pure = PurePosixPath(name)
        if ("\\" in name or pure.is_absolute() or not pure.parts
                or any(part in ("", ".", "..") for part in pure.parts)
                or pure.as_posix() != name or name != name.lower()
                or name.casefold() == "manifest.txt"
                or any(":" in part or part.endswith((".", " ")) for part in pure.parts)):
            raise PackageError(f"{manifest}:{number}: unsafe relative path {name!r}")
        key = name.casefold()
        if key in seen:
            raise PackageError(f"{manifest}:{number}: duplicate path after case folding: {name}")
        seen.add(key)
        if not re.fullmatch(r"[0-9]+", size_text):
            raise PackageError(f"{manifest}:{number}: invalid byte size {size_text!r}")
        if not re.fullmatch(r"[0-9a-fA-F]{64}", digest):
            raise PackageError(f"{manifest}:{number}: invalid SHA-256 digest")
        rows.append((name, int(size_text), digest.lower()))

    if not rows:
        raise PackageError(f"manifest has no entries: {manifest}")

    expected = {"manifest.txt"}
    for name, size, digest in rows:
        candidate = data_dir.joinpath(*PurePosixPath(name).parts)
        resolved = candidate.resolve(strict=True)
        _safe_regular_file(candidate, data_dir)
        try:
            resolved.relative_to(data_dir)
        except ValueError as exc:
            raise PackageError(f"manifest path resolves outside data directory: {name}") from exc
        if candidate.stat().st_size != size:
            raise PackageError(f"size mismatch for {name}: manifest {size}, file {candidate.stat().st_size}")
        actual = _sha256(candidate)
        if actual != digest:
            raise PackageError(f"SHA-256 mismatch for {name}: expected {digest}, got {actual}")
        expected.add(name)

    actual_files: set[str] = set()
    actual_keys: dict[str, str] = {}
    for current, dirs, files in os.walk(data_dir, followlinks=False):
        current_path = Path(current)
        for dirname in dirs:
            directory = current_path / dirname
            if directory.is_symlink():
                raise PackageError(f"symlink is not allowed in loose data: {directory}")
        for filename in files:
            path = current_path / filename
            _safe_regular_file(path, data_dir)
            if path.name.casefold() == "enations.dat":
                raise PackageError(f"container data must not be included in loose data: {path}")
            relative = path.relative_to(data_dir).as_posix()
            key = relative.casefold()
            if key in actual_keys and actual_keys[key] != relative:
                raise PackageError(f"loose data has case-colliding files: {actual_keys[key]} and {relative}")
            actual_keys[key] = relative
            actual_files.add(relative)
    runtime_files = {
        name for name in actual_files
        if allow_runtime_terrain_gpu and name.startswith("terrain_gpu/")
    }
    checked_files = actual_files - runtime_files
    if checked_files != expected:
        missing = sorted(expected - checked_files)
        extra = sorted(checked_files - expected)
        detail = []
        if missing:
            detail.append("missing: " + ", ".join(missing))
        if extra:
            detail.append("unlisted: " + ", ".join(extra))
        raise PackageError("loose data does not exactly match its manifest (" + "; ".join(detail) + ")")
    return rows


def verify_runtime_data(data_dir: Path) -> list[tuple[str, int, str]]:
    """Verify the pinned extracted game data, optionally including staged terrain GPU files."""
    data_dir = data_dir.resolve(strict=True)
    manifest = data_dir / "manifest.txt"
    _safe_regular_file(manifest, data_dir)
    actual_manifest_hash = _sha256(manifest)
    if actual_manifest_hash != RUNTIME_MANIFEST_SHA256:
        raise PackageError(
            f"runtime manifest SHA-256 mismatch: expected {RUNTIME_MANIFEST_SHA256}, "
            f"got {actual_manifest_hash}"
        )
    rows = read_manifest(data_dir, allow_runtime_terrain_gpu=True)
    if len(rows) != RUNTIME_MANIFEST_ENTRIES:
        raise PackageError(
            f"runtime manifest must list {RUNTIME_MANIFEST_ENTRIES} entries, got {len(rows)}"
        )
    return rows


def _copy_tree_checked(source: Path, destination: Path, root: Path) -> int:
    if source.is_symlink() or not source.is_dir():
        raise PackageError(f"expected a real directory: {source}")
    count = 0
    for current, dirs, files in os.walk(source, followlinks=False):
        current_path = Path(current)
        rel_dir = current_path.relative_to(root)
        target_dir = destination / rel_dir
        target_dir.mkdir(parents=True, exist_ok=True)
        for dirname in dirs:
            child = current_path / dirname
            if child.is_symlink():
                raise PackageError(f"symlink is not allowed: {child}")
        for filename in files:
            child = current_path / filename
            _safe_regular_file(child, root)
            if child.name.casefold() == "enations.dat":
                raise PackageError(f"container data must not be packaged: {child}")
            shutil.copy2(child, target_dir / filename)
            count += 1
    return count


def stage_runtime(runtime_dir: Path, stage_dir: Path) -> int:
    runtime_dir = runtime_dir.resolve(strict=True)
    if not runtime_dir.is_dir():
        raise PackageError(f"runtime path is not a directory: {runtime_dir}")
    count = 0
    for item in runtime_dir.iterdir():
        if item.name.casefold() == "enations.dat":
            raise PackageError(f"container data must not be packaged: {item}")
        if item.is_symlink():
            if item.name.casefold() in ROOT_FILES or item.suffix.casefold() in (".dll", ".so", ".dylib"):
                raise PackageError(f"symlink is not allowed: {item}")
            continue
        if item.is_dir():
            if item.name.casefold() in RUNTIME_DIRS:
                count += _copy_tree_checked(item, stage_dir, runtime_dir)
            elif item.name.casefold() == "data":
                terrain = item / "terrain_gpu"
                if terrain.exists():
                    count += _copy_tree_checked(terrain, stage_dir, runtime_dir)
            continue
        name = item.name.casefold()
        if name in ROOT_FILES or item.suffix.casefold() in (".dll", ".so", ".dylib"):
            _safe_regular_file(item, runtime_dir)
            if name == "enations.dat":
                raise PackageError(f"container data must not be packaged: {item}")
            shutil.copy2(item, stage_dir / item.name)
            count += 1

    if not any(p.name.casefold() in ("enations", "enations.exe") for p in stage_dir.iterdir() if p.is_file()):
        raise PackageError(f"no enations executable found in runtime directory: {runtime_dir}")
    if not (stage_dir / "data" / "terrain_gpu").is_dir():
        raise PackageError(f"runtime directory has no data/terrain_gpu: {runtime_dir}")
    return count


def create_package(runtime_dir: Path, data_dir: Path, output: Path,
                   vdmplay_ini: Path | None = None) -> tuple[int, int, int, int]:
    runtime_dir = runtime_dir.resolve(strict=True)
    data_dir = data_dir.resolve(strict=True)
    output = output.resolve()
    rows = verify_runtime_data(data_dir)
    if output.exists():
        raise PackageError(f"output already exists: {output}")
    for source in (runtime_dir, data_dir):
        try:
            output.relative_to(source)
        except ValueError:
            pass
        else:
            raise PackageError(f"output must be outside input directory: {output}")

    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="enations-package-") as temporary:
        stage_dir = Path(temporary) / "package"
        stage_dir.mkdir()
        runtime_count = stage_runtime(runtime_dir, stage_dir)
        if vdmplay_ini is not None:
            vdmplay_ini = vdmplay_ini.resolve(strict=True)
            _safe_regular_file(vdmplay_ini, vdmplay_ini.parent)
            shutil.copy2(vdmplay_ini, stage_dir / "vdmplay.ini")
        loose_count = 0
        for name, _size, _digest in rows:
            source = data_dir.joinpath(*PurePosixPath(name).parts)
            destination = stage_dir / "data" / Path(*PurePosixPath(name).parts)
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, destination)
            if destination.stat().st_size != _size or _sha256(destination) != _digest:
                raise PackageError(f"staged loose data failed manifest verification: {name}")
            loose_count += 1
        shutil.copy2(data_dir / "manifest.txt", stage_dir / "data" / "manifest.txt")

        entries = sorted(p for p in stage_dir.rglob("*") if p.is_file())
        for path in entries:
            if path.name.casefold() == "enations.dat":
                raise PackageError("refusing to include ENations.dat")
        uncompressed = sum(path.stat().st_size for path in entries)
        try:
            root_name = output.stem
            with zipfile.ZipFile(output, "x", compression=zipfile.ZIP_DEFLATED, compresslevel=1) as archive:
                for path in entries:
                    entry_name = (PurePosixPath(root_name) / path.relative_to(stage_dir).as_posix()).as_posix()
                    archive.write(path, entry_name)
            with zipfile.ZipFile(output, "r") as archive:
                bad_entry = archive.testzip()
                if bad_entry is not None:
                    raise PackageError(f"ZIP CRC check failed for {bad_entry}")
                for name, expected_size, expected_digest in rows:
                    entry_name = (PurePosixPath(root_name) / "data" / name).as_posix()
                    digest = hashlib.sha256()
                    actual_size = 0
                    with archive.open(entry_name) as stream:
                        for block in iter(lambda: stream.read(CHUNK_SIZE), b""):
                            digest.update(block)
                            actual_size += len(block)
                    if actual_size != expected_size or digest.hexdigest() != expected_digest:
                        raise PackageError(f"ZIP round-trip did not match manifest for {name}")
                if any(info.filename.casefold().endswith("/enations.dat") for info in archive.infolist()):
                    raise PackageError("ZIP contains ENations.dat")
        except Exception:
            output.unlink(missing_ok=True)
            raise
        return len(entries), loose_count, uncompressed, output.stat().st_size


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runtime-dir", type=Path,
                        help="built run directory containing the executable and staged runtime assets")
    parser.add_argument("--loose-data-dir", type=Path,
                        help="extractor output data/ directory, including manifest.txt")
    parser.add_argument("--output", type=Path, help="new ZIP path (must not already exist)")
    parser.add_argument("--vdmplay-ini", type=Path,
                        help="optional pristine vdmplay.ini to use instead of the run-dir copy")
    parser.add_argument("--verify-loose-data-dir", type=Path,
                        help="verify a loose-data directory and exit without packaging")
    parser.add_argument("--allow-runtime-terrain-gpu", action="store_true",
                        help="when verifying packaged data/, allow only its runtime terrain_gpu/ subtree")
    args = parser.parse_args(argv)
    try:
        if args.verify_loose_data_dir is not None:
            if args.allow_runtime_terrain_gpu:
                rows = verify_runtime_data(args.verify_loose_data_dir)
            else:
                rows = read_manifest(args.verify_loose_data_dir)
            print(f"verified {len(rows)} loose-data entries from {args.verify_loose_data_dir}")
            return 0
        if args.runtime_dir is None or args.loose_data_dir is None or args.output is None:
            parser.error("--runtime-dir, --loose-data-dir and --output are required for packaging")
        file_count, loose_count, unpacked, zipped = create_package(
            args.runtime_dir, args.loose_data_dir, args.output, args.vdmplay_ini)
    except (OSError, PackageError, zipfile.BadZipFile) as exc:
        print(f"package failed: {exc}", file=sys.stderr)
        return 1
    print(f"package: {args.output.resolve()}")
    print(f"files: {file_count}; loose data entries: {loose_count} verified")
    print(f"uncompressed bytes: {unpacked}; ZIP bytes: {zipped}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
