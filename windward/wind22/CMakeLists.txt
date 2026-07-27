
set(wind22_sources
        src/acmutil.cpp
        src/apppalet.cpp
        src/bitbuffe.cpp
        src/blt.cpp
        src/bpecodec.cpp
        src/btree.cpp
        src/codec.cpp
        src/datafile.cpp
        src/davidinl.cpp
        src/dib.cpp
#        src/dircore.cpp
        src/dlgmsg.cpp
        src/fixpoint.cpp
        src/global.cpp
        src/huffmanc.cpp
        src/init.cpp
        src/logging.cpp
        src/lzsscode.cpp
        src/lzwcodec.cpp
        src/mmio.cpp
        src/msg_box.cpp
        src/music.cpp
        src/rand.cpp
        src/scanlist.cpp
        src/stdafx.cpp
        src/mfc_compat_text.cpp
        src/strext.cpp
        src/subclass.cpp
        src/threads.cpp
        src/w22_settings.cpp
        src/wndbase.cpp
        src/winappstub.cpp
        src/wndstub.cpp
        dibwnd.cpp
        )

# ---------------------------------------------------------------------------
# Linux: exclude the legacy WinG/GDI blit path and the ACM audio decompressor.
# These are dead at runtime on the SDL2 branch (rendering is SDL_Renderer,
# audio is SDL_mixer) and bottom out on Win32-only APIs (WinG function ptrs,
# acmStream*/WAVEFORMATEX). Per the project rule we EXCLUDE, not delete; any
# symbol another TU still references gets a stub in win32_compat.cpp.
# Also add the Win32-on-POSIX shim implementation.
if (NOT MSVC)
    list(REMOVE_ITEM wind22_sources
        src/acmutil.cpp     # ACM audio decompression (SDL_mixer now)
        src/apppalet.cpp    # WinG halftone palette
        src/blt.cpp         # WinG BitBlt path (dynamically-loaded pfnWinG*)
        src/msg_box.cpp     # legacy MFC ExtMsgBox (_WINDOWS-gated; no live callers — SDL2 dialogs)
    )
    list(APPEND wind22_sources
        src/win32_compat.cpp        # Win32-on-POSIX shim implementation (Linux only)
        src/win32_legacy_stubs.cpp  # stubs for excluded WinG/palette/blt symbols
    )
endif ()

add_library(wind22 STATIC ${wind22_sources})

# according to wind22.mak this should be MD
# but maybe it was changed to MT in later versions?
# i still think win22 is older or incomplete somehow
# Fast-debug (2026-06-11): same flag surgery as enations_latest — keep
# _DEBUG/asserts/symbols/debug-CRT, drop /Od /Ob0 /RTC1 + checked iterators.
# /RTC1 rides in on CMake's default debug flags and conflicts with /O2.
if (MSVC)
    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Ob0" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Od" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    target_compile_options(wind22 PUBLIC
        $<$<CONFIG:Sanitize>:/MDd /EHsc /Od /Ob0 /fsanitize=address>
        $<$<CONFIG:Debug>:/MDd /EHsc /O2 /Ob2> # ZI
        $<$<CONFIG:Profile>:/MD /EHsc /Od /Ob0> # ZI
        $<$<CONFIG:Release>:/MD /EHsc /O2 /Ob2 /Oi /GL /Gy /Gw /Ot /GF /arch:AVX2 /fp:fast /Zi>
    )
else ()
    # Linux/gcc: -fms-extensions accepts MSVC anonymous-struct/union semantics;
    # -fsigned-char matches MSVC's signed char (the codec TUs index tables by
    # byte). -Wno-unknown-pragmas swallows #pragma warning/comment/intrinsic.
    target_compile_options(wind22 PUBLIC
        -g -fms-extensions -fsigned-char -fpermissive -Wno-unknown-pragmas -Wno-format -Wno-write-strings -Wno-narrowing
        $<$<CONFIG:Release>:-O2>
        $<$<CONFIG:Profile>:-O2>
    )
    # Force-include stdafx.h (PRIVATE so it doesn't leak to dependents that have
    # their own stdafx.h). This mirrors MSVC's /Yu forced-PCH include: many
    # legacy TUs include windward.h/thielen.h before stdafx.h and rely on the
    # PCH having defined CString/ASSERT_VALID first. Not a PCH — just an implicit
    # #include at the top of every TU, so no .gch flag-matching fragility.
    target_compile_options(wind22 PRIVATE -include stdafx.h)
    # scanlist.cpp uses AVX2 intrinsics (_mm256_*). MSVC emits them without
    # /arch; gcc requires -mavx2 to use the intrinsics. Per-file so the rest of
    # the library stays baseline-ISA. ARM (Apple Silicon) has no AVX2 — the file
    # guards the intrinsics behind __AVX2__ and falls back to scalar, so only add
    # the x86 flags on x86 targets.
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i686|i386")
        set_source_files_properties(src/scanlist.cpp PROPERTIES COMPILE_OPTIONS "-mavx2;-mfma")
    endif ()
endif ()

# Phase 1g experiment: drop CMAKE_MFC_FLAG. CMAKE_MFC_FLAG=2 auto-links
# mfcsXX.lib (static MFC entry point shim — provides _AfxWinMain etc.).
# Our gate-on path supplies its own WinMain (WinMain.cpp), so we don't
# need the auto-entry shim. _AFXDLL is kept so MFC types from afxwin.h
# (CRect, CString, CFile, CWnd, CDialog) still link via the DLL.
# set(CMAKE_MFC_FLAG 2)

# Arch macro: select _X86_ / _AMD64_ to match the build platform (see the
# matching note in enations_latest/src/CMakeLists.txt).
if(CMAKE_GENERATOR_PLATFORM STREQUAL "x64" OR CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(EN_ARCH_DEFINE _AMD64_)
else()
    set(EN_ARCH_DEFINE _X86_)
endif()

target_compile_definitions(wind22 PUBLIC
    #LOGGINGON
    AI_THREADS_ENABLED
    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:Profile>:NDEBUG>
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Sanitize>:_DEBUG>
)

if (MSVC)
    target_compile_definitions(wind22 PUBLIC
        # Phase 1g step 2 (2026-05-21): _AFXDLL no longer required —
        # stdafx.h's afx*.h chain is gated out and mfc_compat.h provides the
        # MFC type surface. MFC not linked into enations.exe (mfc-status.ps1).
        # _AFXDLL
        _WIN32_WINNT=_WIN32_WINNT_WIN7
        _WINDOWS
        _MBCS
        WIN32
        ${EN_ARCH_DEFINE} # arch macro (_X86_ / _AMD64_), selected by platform above
        # Fast-debug: no checked iterators in Debug. PUBLIC so every dependent
        # (enations) sees the same std container layouts — mixing IDL levels
        # across the link is an ODR violation. MSVC-only (see root CMakeLists).
        $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
    )
endif ()


if (MSVC)
    # Precompiled header - specify the correct path. Disabled on Linux for the
    # bring-up: gcc PCH flag-matching is finicky and masks real porting errors.
    target_precompile_headers(wind22 PUBLIC include/stdafx.h)

    target_link_options(wind22 PUBLIC
        $<$<CONFIG:Sanitize>:/DEBUG:FULL /OPT:NOREF /OPT:NOICF /fsanitize=address>
        $<$<CONFIG:Debug>:/DEBUG:FULL /OPT:NOREF /OPT:NOICF>
        $<$<CONFIG:Profile>:/DEBUG:FULL /INCREMENTAL:NO /OPT:NOREF /OPT:NOICF>
        $<$<CONFIG:Release>:/DEBUG:FASTLINK /INCREMENTAL:NO /OPT:REF /OPT:ICF /LTCG>
        /DYNAMICBASE
        /NXCOMPAT
    )

    target_link_libraries(wind22 PUBLIC
            winmm.lib
            version.lib
            SDL2.lib
            SDL2_mixer.lib
            SDL2_ttf.lib
            )

    target_include_directories(wind22 PUBLIC
            include
            ./include
            ../../tools
            ../../tools/sdl2/SDL2-2.30.12/include
            ../../tools/sdl2/SDL2_mixer-2.8.1/include
            ../../tools/sdl2/SDL2_ttf-2.22.0/include
            )

    target_link_directories(wind22 PUBLIC
            ../../cmakeBuild/tools/lib
            ../../tools/sdl2/SDL2-2.30.12/lib/${SDL2_ARCH}
            ../../tools/sdl2/SDL2_mixer-2.8.1/lib/${SDL2_ARCH}
            ../../tools/sdl2/SDL2_ttf-2.22.0/lib/${SDL2_ARCH}
            )
else ()
    # Linux: system SDL2 (libsdl2-dev / -ttf / -mixer) via the EnLinuxSDL2
    # interface target defined in the root CMakeLists; project headers only.
    target_include_directories(wind22 PUBLIC
            include
            ./include
            ../../tools
            )
    target_link_libraries(wind22 PUBLIC EnLinuxSDL2)
endif ()
