import hashlib
import importlib.util
import tempfile
import unittest
import zipfile
from unittest import mock
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[2] / "tools" / "package_release.py"
SPEC = importlib.util.spec_from_file_location("package_release", SCRIPT)
package_release = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(package_release)


class PackageReleaseTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.runtime = self.root / "runtime"
        self.runtime.mkdir()
        (self.runtime / "enations.exe").write_bytes(b"exe")
        (self.runtime / "SDL2.dll").write_bytes(b"dll")
        (self.runtime / "README.txt").write_text("release", encoding="utf-8")
        (self.runtime / "vdmplay.ini").write_text("runtime settings", encoding="utf-8")
        (self.runtime / "enations.pdb").write_bytes(b"symbols")
        (self.runtime / "run.log").write_text("log", encoding="utf-8")
        (self.runtime / "Save015.en").write_bytes(b"save")
        for folder in ("assets", "res", "data/terrain_gpu"):
            (self.runtime / folder).mkdir(parents=True, exist_ok=True)
        (self.runtime / "assets" / "icon.png").write_bytes(b"icon")
        (self.runtime / "res" / "cursor.cur").write_bytes(b"cursor")
        (self.runtime / "data" / "terrain_gpu" / "tile.png").write_bytes(b"terrain")
        self.data = self.root / "loose-data"
        (self.data / "units").mkdir(parents=True)
        (self.data / "units" / "units.rif").write_bytes(b"units test data")
        (self.data / "files").mkdir()
        (self.data / "files" / "stdgta.dat").write_bytes(b"std game data")
        rows = []
        for path in sorted(p for p in self.data.rglob("*") if p.is_file()):
            rel = path.relative_to(self.data).as_posix()
            blob = path.read_bytes()
            rows.append(f"{rel} {len(blob)} {hashlib.sha256(blob).hexdigest()}")
        (self.data / "manifest.txt").write_text("\n".join(rows) + "\n", encoding="utf-8")
        self.runtime_pin = mock.patch.object(
            package_release, "RUNTIME_MANIFEST_SHA256",
            hashlib.sha256((self.data / "manifest.txt").read_bytes()).hexdigest())
        self.runtime_count = mock.patch.object(package_release, "RUNTIME_MANIFEST_ENTRIES", 2)
        self.runtime_pin.start()
        self.runtime_count.start()
        self.addCleanup(self.runtime_pin.stop)
        self.addCleanup(self.runtime_count.stop)
        self.output = self.root / "release.zip"

    def tearDown(self):
        self.temp.cleanup()

    def test_builds_clean_package_and_keeps_loose_stdgta_dat(self):
        result = package_release.create_package(self.runtime, self.data, self.output)
        self.assertEqual(result[1], 2)
        with zipfile.ZipFile(self.output) as archive:
            names = set(archive.namelist())
            self.assertIn("release/data/units/units.rif", names)
            self.assertIn("release/data/files/stdgta.dat", names)
            self.assertIn("release/data/manifest.txt", names)
            self.assertIn("release/data/terrain_gpu/tile.png", names)
            self.assertNotIn("release/enations.pdb", names)
            self.assertNotIn("release/run.log", names)
            self.assertNotIn("release/Save015.en", names)
            self.assertFalse(any(name.casefold().endswith("/enations.dat") for name in names))
            self.assertEqual(archive.read("release/vdmplay.ini"), b"runtime settings")

    def test_rejects_tampered_entry_before_output(self):
        (self.data / "units" / "units.rif").write_bytes(b"changed")
        with self.assertRaises(package_release.PackageError):
            package_release.create_package(self.runtime, self.data, self.output)
        self.assertFalse(self.output.exists())

    def test_rejects_unsafe_manifest_path(self):
        (self.data / "manifest.txt").write_text(
            "../escape.rif 1 " + hashlib.sha256(b"x").hexdigest() + "\n", encoding="utf-8")
        with self.assertRaises(package_release.PackageError):
            package_release.read_manifest(self.data)

    def test_rejects_unlisted_file(self):
        (self.data / "extra.rif").write_bytes(b"not listed")
        with self.assertRaises(package_release.PackageError):
            package_release.read_manifest(self.data)

    def test_runtime_verifier_rejects_unpinned_manifest(self):
        with mock.patch.object(package_release, "RUNTIME_MANIFEST_SHA256", "0" * 64):
            with self.assertRaisesRegex(package_release.PackageError, "runtime manifest SHA-256 mismatch"):
                package_release.verify_runtime_data(self.data)

    def test_runtime_verifier_accepts_fixture_with_derived_pin(self):
        manifest_hash = hashlib.sha256((self.data / "manifest.txt").read_bytes()).hexdigest()
        with mock.patch.object(package_release, "RUNTIME_MANIFEST_SHA256", manifest_hash), \
             mock.patch.object(package_release, "RUNTIME_MANIFEST_ENTRIES", 2):
            rows = package_release.verify_runtime_data(self.data)
        self.assertEqual(len(rows), 2)

    def test_manifest_verification_only_allows_runtime_terrain_subtree(self):
        terrain = self.data / "terrain_gpu"
        terrain.mkdir()
        (terrain / "runtime.png").write_bytes(b"runtime terrain")
        rows = package_release.read_manifest(self.data, allow_runtime_terrain_gpu=True)
        self.assertEqual(len(rows), 2)
        (self.data / "unlisted.rif").write_bytes(b"unlisted")
        with self.assertRaises(package_release.PackageError):
            package_release.read_manifest(self.data, allow_runtime_terrain_gpu=True)

    def test_rejects_nested_container_in_runtime_terrain_subtree(self):
        terrain = self.data / "terrain_gpu"
        terrain.mkdir()
        (terrain / "ENATIONS.DAT").write_bytes(b"forbidden container")
        with self.assertRaisesRegex(package_release.PackageError, "container data must not be included"):
            package_release.read_manifest(self.data, allow_runtime_terrain_gpu=True)

    def test_rejects_container_in_runtime_directory(self):
        (self.runtime / "ENations.dat").write_bytes(b"container")
        with self.assertRaises(package_release.PackageError):
            package_release.create_package(self.runtime, self.data, self.output)
        self.assertFalse(self.output.exists())


if __name__ == "__main__":
    unittest.main()
