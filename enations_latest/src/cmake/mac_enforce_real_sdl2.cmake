# mac_enforce_real_sdl2.cmake — run at POST_BUILD via `cmake -P`.
#
# Homebrew's `sdl2` formula on macOS is now **sdl2-compat**, an SDL2 API on top of
# SDL3. Its blits compare the colour key against ALL 32 bits of a pixel — including the
# unused padding byte, which its format conversions fill with 0xFF while SDL_MapRGB
# builds keys with 0x00 there. So on any surface that went through
# SDL_ConvertSurfaceFormat (the game's DIB loaders), SDL_SetColorKey "succeeds"
# (rc=0, SDL_HasColorKey=yes, key reads back) and then never matches a single pixel.
# The game keys magenta (255,0,255) transparent when it builds surfaces from DIBs, so
# against the shim every sprite, icon and info-window thumbnail renders on a bright
# magenta background. (Beware: a naive fresh-surface colour-key self-test PASSES on
# the shim — FillRect writes 0x00 padding. Verified 2026-08-09, sdl2-compat 2.32.70.)
#
# Nothing in the build picks the real SDL2: pkg-config finds Homebrew's, so EVERY fresh
# mac build links the shim again. The fix has always been a post-build install_name_tool
# relink, applied by hand — and a fix you have to remember is a fix that gets skipped.
# It has now regressed this way more than once, each time reported as a release blocker,
# because a shim-linked binary looks completely normal until you actually see a sprite.
#
# So this runs on every build and either produces a real-SDL2 link or FAILS THE BUILD.
# It never leaves a silently-shim-linked binary behind.
#
# Inputs (set with -D by the caller):
#   EN_BINARY       the just-linked executable
#   EN_DONOR_DIR    directory holding a REAL, already-@loader_path-relinked SDL2 set
#
# macOS only; the caller guards on APPLE.

if (NOT EXISTS "${EN_BINARY}")
    message(FATAL_ERROR "mac SDL2 check: no such binary: ${EN_BINARY}")
endif ()

get_filename_component(_bindir "${EN_BINARY}" DIRECTORY)

# sdl2-compat is detected STRUCTURALLY, by the marker strings it carries (38 of them in
# the shim, 0 in real SDL2), not by a version number — a version threshold would be a
# guess about future releases, and the shim deliberately reports SDL2 versions.
# Every "cannot determine" here fails TOWARD repair/FATAL, never toward "real" — a false
# "real" is exactly the silent shim link this gate exists to prevent. A missing file, an
# unresolvable install-name, or a broken `strings` must not be read as a clean bill.
function (en_is_shim _path _out)
    set (${_out} TRUE PARENT_SCOPE)   # default: cannot prove real -> treat as shim
    if (NOT EXISTS "${_path}")
        return ()   # can't scan a file that isn't there -> stays "shim" (fail safe)
    endif ()
    execute_process(COMMAND strings "${_path}"
                    OUTPUT_VARIABLE _s RESULT_VARIABLE _rc ERROR_QUIET)
    if (NOT _rc EQUAL 0)
        message(FATAL_ERROR
            "mac SDL2 check: `strings` failed on ${_path} (rc=${_rc}); cannot verify the "
            "SDL2 link is not the sdl2-compat shim. Refusing to guess. Install binutils/"
            "cctools `strings` or fix PATH, then rebuild.")
    endif ()
    string(FIND "${_s}" "sdl2-compat" _hit)
    if (_hit EQUAL -1)
        set (${_out} FALSE PARENT_SCOPE)   # scanned, no marker -> genuinely real
    endif ()
endfunction ()

# Which libSDL2 does the binary actually load, and is it real?
function (en_linked_sdl2_is_real _binary _out _pathout)
    execute_process(COMMAND otool -L "${_binary}" OUTPUT_VARIABLE _otool ERROR_QUIET)
    string(REGEX MATCH "[^ \t\n]*libSDL2-2\\.0\\.0\\.dylib" _ref "${_otool}")
    set (${_pathout} "${_ref}" PARENT_SCOPE)
    if (_ref STREQUAL "")
        set (${_out} FALSE PARENT_SCOPE)   # links no SDL2 at all -> not real
        return ()
    endif ()
    # Resolve the install-name to a file we can scan. @loader_path and @executable_path
    # both resolve against the binary's own directory (it's a top-level executable). An
    # @rpath/... ref would need LC_RPATH resolution we don't do here — treat it as
    # unverifiable so it fails toward repair rather than passing blind.
    if (_ref MATCHES "^@rpath/")
        set (${_out} FALSE PARENT_SCOPE)   # cannot verify @rpath here -> not proven real
        return ()
    endif ()
    string(REPLACE "@loader_path" "${_bindir}" _resolved "${_ref}")
    string(REPLACE "@executable_path" "${_bindir}" _resolved "${_resolved}")
    en_is_shim("${_resolved}" _shim)
    if (_shim)
        set (${_out} FALSE PARENT_SCOPE)
    else ()
        set (${_out} TRUE PARENT_SCOPE)
    endif ()
endfunction ()

en_linked_sdl2_is_real("${EN_BINARY}" _real _ref)
if (_real)
    message(STATUS "SDL2 linkage OK (real SDL2 via ${_ref})")
    return ()
endif ()

# Shim-linked. Repair it from the donor set, which must itself be real.
if (NOT EXISTS "${EN_DONOR_DIR}/libSDL2-2.0.0.dylib")
    message(FATAL_ERROR
        "This build linked the sdl2-compat SHIM (via ${_ref}), which ignores the colour "
        "key and renders every sprite on magenta.\n"
        "No real SDL2 to repair it with: expected ${EN_DONOR_DIR}/libSDL2-2.0.0.dylib.\n"
        "Point -DEN_MAC_SDL2_DONOR_DIR at a directory holding a real SDL2 dylib set "
        "(the shipped release artifacts under release-artifacts/ contain one).")
endif ()
en_is_shim("${EN_DONOR_DIR}/libSDL2-2.0.0.dylib" _donor_shim)
if (_donor_shim)
    message(FATAL_ERROR
        "Donor SDL2 at ${EN_DONOR_DIR}/libSDL2-2.0.0.dylib is itself the sdl2-compat shim. "
        "Repairing from it would keep the magenta-sprite bug.")
endif ()

# Copy the donor's whole dylib set, not just libSDL2: libSDL2_ttf and libSDL2_mixer link
# SDL2 themselves, so relinking only the executable would leave TWO SDL2 copies loaded in
# one process. The donor set is already relinked to @loader_path, and its members pull
# their own dependencies (freetype, harfbuzz, ogg, ...) the same way, so the whole closure
# has to sit beside the binary for those references to resolve.
# ...but NEVER copy back a dylib this project itself builds. The usual donor dir is the
# run dir (run-mac), which also holds our own libvdmplay_posix.dylib. Copying that over
# the freshly linked one silently replaces NEW code with the previous run's binary while
# the build still reports success, and because copy_if_different then leaves the output
# newer than its inputs, EVERY later rebuild is a no-op and the stale dylib survives
# indefinitely. Observed 2026-08-23: a tree reset to lane tip kept shipping a retired
# local patch - object file clean, shipped dylib byte-identical to the old copy - which
# would have invalidated every verification run from this machine.
# Donors are third-party dependencies only; our own targets are always the build's output.
set(EN_PROJECT_DYLIBS libvdmplay_posix.dylib)
file(GLOB _donor_dylibs "${EN_DONOR_DIR}/*.dylib")
foreach (_d IN LISTS _donor_dylibs)
    get_filename_component(_n "${_d}" NAME)
    if (_n IN_LIST EN_PROJECT_DYLIBS)
        message(STATUS "Donor stage: SKIPPING ${_n} - built by this project, not a donor dependency")
        continue ()
    endif ()
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_d}" "${_bindir}/${_n}")
endforeach ()

# Repoint every absolute Homebrew SDL2 reference at the co-located copies.
execute_process(COMMAND otool -L "${EN_BINARY}" OUTPUT_VARIABLE _otool ERROR_QUIET)
foreach (_lib libSDL2-2.0.0.dylib libSDL2_ttf-2.0.0.dylib libSDL2_mixer-2.0.0.dylib)
    string(REGEX MATCH "/[^ \t\n]*${_lib}" _abs "${_otool}")
    if (NOT _abs STREQUAL "")
        execute_process(COMMAND install_name_tool -change "${_abs}" "@loader_path/${_lib}" "${EN_BINARY}")
    endif ()
endforeach ()

# The signature covers the load commands, so it must be renewed after rewriting them (an
# ad-hoc `-s -` signature — fine for a dev build; a real signing identity, if one is ever
# applied, must run AFTER this step). A stale/invalid signature is SIGKILL at launch on
# arm64, and the linkage re-verify below would still pass (it only reads load commands),
# so a failed codesign must fail the build here rather than ship a launch-dead binary.
execute_process(COMMAND codesign -f -s - "${EN_BINARY}"
                RESULT_VARIABLE _cs_rc OUTPUT_QUIET ERROR_VARIABLE _cs_err)
if (NOT _cs_rc EQUAL 0)
    message(FATAL_ERROR
        "mac SDL2 check: re-codesign failed after relink (rc=${_cs_rc}): ${_cs_err}\n"
        "The relinked binary would be SIGKILLed at launch (invalid signature).")
endif ()

# Prove it, rather than assuming the rewrite worked.
en_linked_sdl2_is_real("${EN_BINARY}" _real_now _ref_now)
if (NOT _real_now)
    message(FATAL_ERROR
        "Relink to real SDL2 FAILED — the binary still resolves ${_ref_now} to the "
        "sdl2-compat shim. Refusing to leave a build that renders every sprite magenta.")
endif ()
message(STATUS "SDL2 linkage REPAIRED from ${EN_DONOR_DIR} (now ${_ref_now})")
