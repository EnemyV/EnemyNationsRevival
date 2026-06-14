cmake_minimum_required(VERSION 3.11...3.26)

if (${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif ()

# On Windows look in the bundled ccache dir; elsewhere just use PATH.
if (WIN32)
    find_program(CCACHE_PROGRAM ccache PATHS D:\\ccache-4.8.1-windows-x86_64)
else ()
    find_program(CCACHE_PROGRAM ccache)
endif ()
if (CCACHE_PROGRAM)
    message(STATUS "Found ccache: ${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
else ()
    message(STATUS "Could not find ccache.")
endif ()

# Ensure VS multi-config generator exports these solution configurations
if (CMAKE_GENERATOR MATCHES "Visual Studio")
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Profile;Sanitize" CACHE STRING "Available configurations" FORCE)
endif ()

project(Enations VERSION 1.0
                 DESCRIPTION "Enemy Nations"
                 LANGUAGES CXX)

# Debug builds: DISABLE MSVC checked iterators (_ITERATOR_DEBUG_LEVEL=2 is the /MDd
# default). Every checked-iterator construct/copy/destroy — including const find(),
# vector push_back range-orphaning, any begin()/end() — takes std::_Lockit, ONE GLOBAL
# critical section for the entire process. With 12 AI threads + the main thread all
# running STL-heavy code, stack-sampling the 13-player save showed threads parked on
# that lock from independent directions (CMap::Lookup in AssertValid sweeps, sprite-
# capture vector push_backs, the dialog-list end() in PollEvents) — a lock convoy that
# took the game to ~1-4 fps. IDL=0 removes the proxies and the lock entirely; the CRT
# debug heap and game-side ASSERT/en_assert diagnostics remain. Set GLOBALLY (before
# all add_subdirectory) because IDL participates in the C++ ABI: mixed-IDL TUs fail to
# link (LNK2038), so every target must agree. The mfc_compat CMapSrw guard keys its
# read mode on IDL and automatically degrades to cheap SHARED reads at IDL=0.
#
# Linux note: _ITERATOR_DEBUG_LEVEL is MSVC-only. The libstdc++ analog is
# _GLIBCXX_DEBUG, which changes std container ABI and must match across every
# TU/lib or it silently corrupts the heap — and libstdc++ has no global _Lockit
# convoy to avoid in the first place. So define NOTHING here on Linux and leave
# libstdc++ at its default; gate the MSVC define under MSVC only.
if (MSVC)
    add_compile_definitions($<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>)
endif ()

# ---------------------------------------------------------------------------
# Linux: discover system SDL2 (libsdl2-dev / libsdl2-ttf-dev / libsdl2-mixer-dev)
# via pkg-config and expose it as one INTERFACE target both wind22 and enations
# link against. Replaces the bundled Windows tools/sdl2 import libs/DLLs.
# ---------------------------------------------------------------------------
if (NOT MSVC)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2       REQUIRED IMPORTED_TARGET sdl2)
    pkg_check_modules(SDL2_TTF   REQUIRED IMPORTED_TARGET SDL2_ttf)
    pkg_check_modules(SDL2_MIXER REQUIRED IMPORTED_TARGET SDL2_mixer)

    add_library(EnLinuxSDL2 INTERFACE)
    target_link_libraries(EnLinuxSDL2 INTERFACE
        PkgConfig::SDL2 PkgConfig::SDL2_TTF PkgConfig::SDL2_MIXER
        pthread dl m rt)
    # Linux-only shim dir: maps <windows.h>/<windef.h>/<windowsx.h>/... to the
    # Win32-on-POSIX shim, so the ~17 headers that include the SDK umbrella
    # headers compile unchanged. Carried transitively to wind22 and enations.
    target_include_directories(EnLinuxSDL2 INTERFACE
        ${CMAKE_SOURCE_DIR}/windward/wind22/include/winshim)
endif ()

###############################
# enations.exe   (game — built on every platform)
###############################

add_subdirectory(enations_latest/src)

###############################
# wind22.lib     (support library — built on every platform)
###############################

add_subdirectory(windward/wind22)

# ---------------------------------------------------------------------------
# Build-time tools + the vdmplay video DLL are MSVC/Windows-only for now.
# Linux scope is single-player game bring-up: the asset/build tools
# (rif_converter, makeriff, sprite, cdf, compit, iserve) are not needed to
# run the game, and vdmplay is VFW-backed (video playback is SDL2 now), so it
# is stubbed at link time on Linux instead of built. See the Linux port plan.
# ---------------------------------------------------------------------------
if (MSVC)
    ###############################
    # rif_converter.exe (SDL2 Phase 0 - Asset Conversion)
    ###############################
    add_subdirectory(tools/rif_converter)

    ###############################
    # makeriff.exe
    ###############################
    add_subdirectory(tools/makeriff)

    ###############################
    # sprite.exe
    ###############################
    add_subdirectory(tools/sprite)

    ###############################
    # cdf.exe
    ###############################
    add_subdirectory(src/cdf)

    ###############################
    # compit.exe
    ###############################
    add_subdirectory(tools/compress)

    ###############################
    # vdmplay.dll
    ###############################
    add_subdirectory(tools/vdmplay)

    ###############################
    # ISERVE.exe
    ###############################
    add_subdirectory(tools/vdmplay/iserve)
endif ()
