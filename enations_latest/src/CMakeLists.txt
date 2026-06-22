message("Current CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")

set(enations_sources
#        advanced.cpp
        ai.cpp
        aisnap.cpp   # Tier-B AI world snapshot (lock-free AI reads)
        area.cpp
        bmbutton.cpp
        bridge.cpp
        caicopy.cpp
        caidata.cpp
        caigmgr.cpp
        caigoal.cpp
        caihex.cpp
        caiinit.cpp
        cailog.cpp
        caimap.cpp
        caimaput.cpp
        caimgr.cpp
        caimsg.cpp
        caiopfor.cpp
        caiqueue.cpp
        cairoute.cpp
        caisavld.cpp
        caistart.cpp
        caitask.cpp
        caitmgr.cpp
        caiunit.cpp
        CdLoc.cpp
        # chat.cpp excluded (Phase 2d) — CDlgChatAll replaced by SDL2ChatWindow
        # chatbar.cpp excluded (Phase 2d-cont, 2026-05-11) — CChatBar dead
        chproute.cpp
#        citizen.cpp # looks unused, i think an old ai or something?
        cpathmap.cpp
        cpathmgr.cpp
        creatmul.cpp
        creatsin.cpp
        credits.cpp
        cutscene.cpp
        DlgMsg.cpp
        DlgReg.cpp  # holds the GetDefaultApp helper used by ExitInstance
        dxfer.cpp
        event.cpp
        icons.cpp
        # ipcchat.cpp excluded (Phase 2d-cont) — CChatWnd dead at runtime
        # ipccomm.cpp excluded (Phase 2d-cont) — CWndComm replaced by ChatStub in ipccomm.h
        ipcmsg.cpp
        ipcplay.cpp
        # ipcread.cpp excluded (Phase 2d-cont) — CWndMailRead dead at runtime
        # ipcsend.cpp excluded (Phase 2d-cont) — CDlgCompose dead at runtime
        join.cpp
        lastplnt.cpp
        lastplnt.rc
        WinMain.cpp
        license.cpp
        main.cpp
        mainloop.cpp
        minerals.cpp
        # movie.cpp excluded (Phase 2d) — CWndMovie replaced by SDL2VideoPlayer
        netapi.cpp
        netcmd.cpp
        wire_layout_assert.cpp   # compile-time size pins for the wire structs (cross-platform MP regression guard)
        new_game.cpp
        new_unit.cpp
        newworld.cpp
        player.cpp
        PlyrList.cpp
        projbase.cpp
        racedata.cpp
        relation.cpp
        research.cpp
        resource.h
        scenario.cpp
        sprite.cpp
        sprtinit.cpp
        stdafx.cpp
        terrain.cpp
        toolbar.cpp
        ui.cpp
        unit.cpp
        unit_wnd.cpp
        vehicle.cpp
        vehmove.cpp
        vehoppo.cpp
        vpxfer.cpp
        world.cpp
        wrldinit.cpp
        wrldgen.cpp
        mem_pool.h
        cpuspeed.cpp
        EnSettings.cpp
        Perf.cpp
        Perf.h
        GameWindow.cpp
        RenderingAdapter.cpp
        SDL2MainMenu.cpp
        SDL2UI.cpp
        SDL2Options.cpp
        SDL2Dialogs.cpp
        SDL2Video.cpp
        SDL2Panel.cpp
        SDL2Compositor.cpp
        SDL2Terrain.cpp
        SDL2Sprites.cpp
        SDL2AreaBar.cpp
        SDL2BuildingWindow.cpp
        SDL2BuildStructure.cpp
        SDL2BuildTransport.cpp
        SDL2CreateStatus.cpp
        SDL2FileDialog.cpp
        SDL2GameDialogs.cpp
        SDL2MFCPanel.cpp
        SDL2RouteWindow.cpp
        SDL2FileBrowser.cpp
        SDL2SaveDialog.cpp
        SDL2Toolbar.cpp
        SDL2UnitList.cpp
        ../../rendering/SDLButton.cpp
        ../../rendering/SDLButtonManager.cpp
        ../../rendering/StatusBar.cpp
        ../../input/InputHandler.cpp
        ../../input/UIButtonListener.cpp
        network/davenet.cpp
        network/sockets.cpp
)

# ---------------------------------------------------------------------------
# Linux source-list adjustments:
#  - lastplnt.rc is an MFC .RC resource script; gcc/CMake cannot compile it.
#    Drop it and provide its runtime STRINGTABLE via win32_resources_linux.cpp.
#  - linux_main.cpp supplies main() -> WinMain (the Win32 entry has no main()).
# ---------------------------------------------------------------------------
if (NOT MSVC)
    list(REMOVE_ITEM enations_sources lastplnt.rc)
    list(APPEND enations_sources
        linux_main.cpp
        win32_resources_linux.cpp
        ../../harness/control_socket.cpp   # in-process LLM-driving harness (Debug)
    )

    # -----------------------------------------------------------------------
    # MP POSIX port (plan §7b): the proven vp* (VDMPLAY) TCP engine, ported
    # winsock->BSD via tools/vdmplay/vp_sock_posix.h. Built as ITS OWN static
    # lib (not folded into enations_sources) so it does NOT inherit the game's
    # force-included stdafx.h (-include stdafx.h, line ~191) — the vp* TUs carry
    # their own tools/vdmplay/stdafx.h, which pulls win32_compat.h + the BSD
    # socket shim on the POSIX branch. Replaces the 45-line vdmplay_stubs.cpp
    # once the real vp* exports link. IPX/NetBIOS/COMM/modem/Win16 transports
    # are #ifdef'd out on POSIX (TCP only).
    set(vdmplay_posix_sources
        ../../tools/vdmplay/vdmplay.cpp
        ../../tools/vdmplay/vpengine.cpp
        ../../tools/vdmplay/tcpnet.cpp
        ../../tools/vdmplay/vpwinsk.cpp
        ../../tools/vdmplay/smap.cpp
        ../../tools/vdmplay/vputil.cpp
        ../../tools/vdmplay/vpnet.cpp
        ../../tools/vdmplay/vp_netpump_posix.cpp   # step 5: select() pump (WSAAsyncSelect repl.)
    )
    # SHARED (not STATIC): the vp* engine has C++ classes (CPlayer/CSession/…)
    # whose names collide with the game's same-named classes. The old Windows
    # build isolated them behind the vdmplay.dll boundary; we reproduce that with
    # a .so built -fvisibility=hidden — only the extern "C" vp* API (VPAPI ->
    # visibility default) is exported; the internal classes stay private, so no
    # multiple-definition clash at the exe link.
    add_library(vdmplay_posix SHARED ${vdmplay_posix_sources})
    target_include_directories(vdmplay_posix PRIVATE
        ../../tools/vdmplay
        ../../windward/wind22/include
        ../../windward/wind22/include/winshim
        ../../tools/
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/network
    )
    target_compile_definitions(vdmplay_posix PRIVATE VPSYSTEM)
    target_compile_options(vdmplay_posix PRIVATE
        -g -fms-extensions -fsigned-char -fpermissive
        -fvisibility=hidden -fvisibility-inlines-hidden
        -Wno-unknown-pragmas -Wno-format -Wno-write-strings -Wno-narrowing)
    # Mirror MSVC's PCH: every vp* TU gets stdafx.h (-> win32_compat.h + BSD
    # socket shim) force-included, so TUs that include vputil.h/etc. before
    # stdafx.h (e.g. vputil.cpp) still see BOOL/LPCSTR/CRITICAL_SECTION. The
    # tools/vdmplay include dir is first, so this resolves to the vp* stdafx.h.
    target_compile_options(vdmplay_posix PRIVATE -include stdafx.h)
    # Bind the engine's own internal refs to its own definitions (don't let the
    # exe's same-named symbols interpose at load time) and ship the .so next to
    # the binary ($ORIGIN rpath) so it's found without LD_LIBRARY_PATH.
    target_link_options(vdmplay_posix PRIVATE -Wl,-Bsymbolic)
    set_target_properties(vdmplay_posix PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif ()

# MSVC-only host toolset selection (32-bit host tools for the VS generator).
if (MSVC)
    set(CMAKE_GENERATOR_TOOLSET "host=x86")
endif ()

# The WIN32 keyword is a no-op on non-Windows (CMake ignores it), so it is
# harmless to leave for the Linux build.
add_executable(enations WIN32 ${enations_sources})

# network/ TUs do winsock2-before-windows.h ordering themselves; skip the
# force-included PCH (stdafx.h->windows.h pulls winsock1 and breaks ws2tcpip.h).
set_source_files_properties(
    network/davenet.cpp
    network/sockets.cpp
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

# according to enations.mak this should be MT
# but i think it used to be MD?
# should be /Zi for release (no it shouldn't?)')
# /Z7 for profiling
# BUGBUGTODO change back to /Zi
# Fast-debug (2026-06-11): Debug keeps _DEBUG/asserts/symbols/debug-CRT but
# drops the pure perf-tax knobs — /Od (no-opt), /Ob0 (no inlining at all;
# brutal in this accessor-heavy codebase), /RTC1 (stack checks, seen live in
# profile stacks), and checked iterators (_ITERATOR_DEBUG_LEVEL=0 below).
# Live 13-player profiling showed operV=729ms/s at 16us per CVehicle::Operate
# under the old flags. /RTC1 comes from CMake's default debug flags and is
# incompatible with /O2 — strip it (and the /Od;/Ob0 defaults ours override).
if (MSVC)
    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Ob0" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Od" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    target_compile_options(enations PRIVATE
        $<$<CONFIG:Release>:/MD;/EHsc;/O2;/Ob2;/Oi;/GL;/Gy;/Gw;/Ot;/GF;/arch:AVX2;/fp:fast;/Zi>
        $<$<CONFIG:Profile>:/MD;/EHsc;/O2;/Ob2;/Oi;/GL;/Gy;/Gw;/Ot;/GF;/arch:AVX2;/fp:fast;/Zi>
        $<$<CONFIG:Debug>:/MDd;/EHsc;/Zi;/O2;/Ob2>
        $<$<CONFIG:Sanitize>:/MDd;/EHsc;/Zi;/Od;/Ob0;/fsanitize=address>
    )
else ()
    # Linux/gcc. -fms-extensions + -fsigned-char match MSVC semantics the code
    # relies on; -Wno-unknown-pragmas swallows #pragma warning/comment/intrinsic.
    target_compile_options(enations PRIVATE
        -g -fms-extensions -fsigned-char -fpermissive -Wno-unknown-pragmas -Wno-format -Wno-write-strings -Wno-narrowing
        $<$<CONFIG:Release>:-O2>
        $<$<CONFIG:Profile>:-O2>
    )
    # Force-include stdafx.h — mirrors MSVC's /Yu forced-PCH include so TUs that
    # include game headers before stdafx.h still get the precompiled surface.
    target_compile_options(enations PRIVATE -include stdafx.h)
endif ()

# Phase 1g step 1: drop CMAKE_MFC_FLAG. Our own WinMain (WinMain.cpp)
# replaces MFC's auto-generated AfxWinMain entry shim. The MFC types from
# afx*.h still link via _AFXDLL until Phase 1g step 2 (header purge).
# set(CMAKE_MFC_FLAG 2)

message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")


get_target_property(copts enations COMPILE_OPTIONS)

message("copts: ${copts}")

add_compile_definitions(
    $<$<CONFIG:Sanitize>:_DEBUG>
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:Profile>:NDEBUG> # disables asserts in profile builds
)


# Cross-platform game-logic defines + per-config debug/release macros.
target_compile_definitions(enations PUBLIC
        #LOGGINGON
        #_LOGOUT
        # STRICTER_ASSERTS ## these prob all need to be fixed
        AI_THREADS_ENABLED # here's hoping!!
        OPTIMIZE_BLIT_ORDER
        # AI_SPECTATE
        # _CHEAT
        $<$<CONFIG:Release>:NDEBUG>
        $<$<CONFIG:Profile>:NDEBUG>
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Sanitize>:_DEBUG>
        )

if (MSVC)
    # windows.h normally derives _X86_/_AMD64_ from the compiler's _M_* macros,
    # but this codebase defined _X86_ by hand. Pick the macro that matches the
    # build platform so an x64 compile doesn't pull in the 32-bit SDK defs.
    if(CMAKE_GENERATOR_PLATFORM STREQUAL "x64" OR CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(EN_ARCH_DEFINE _AMD64_)
    else()
        set(EN_ARCH_DEFINE _X86_)
    endif()

    target_compile_definitions(enations PUBLIC
            # mfc_compat.h provides the MFC type surface; MFC not linked.
            _WIN32
            _WIN32_WINNT=_WIN32_WINNT_WIN7
            # NOMINMAX: stop <windows.h> defining max/min as textual MACROS. The
            # cross-platform port uses ::max(...)/::min(...) (binds to the global
            # templates in stdafx.h, mirroring the Linux win32_compat.h design); the
            # macros would expand ::max(a,b) -> ::(...) and break the build. Legacy
            # unqualified max(a,b)/min(a,b) sites bind to those templates instead.
            NOMINMAX
            _MBCS
            ${EN_ARCH_DEFINE} # arch macro (_X86_ / _AMD64_)
            WIN32
            # Fast-debug: no checked iterators in Debug. MUST match wind22 (std
            # container layouts change with this) — both targets define it.
            $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
            )

    # PCH disabled on Linux for the bring-up (gcc PCH flag-matching masks errors).
    target_precompile_headers(enations PRIVATE stdafx.h)
endif ()


get_target_property(cdefs enations COMPILE_DEFINITIONS)
message("cdefs: ${cdefs}")
        
# Project include dirs (cross-platform). SDL2/glew bundled headers are added
# only for MSVC; Linux gets SDL2 headers transitively via EnLinuxSDL2.
target_include_directories(enations PUBLIC
        ../../tools/
       ../../cmakeBuild/tools
       ./cmakeBuild/tools
       ../../windward/wind22/include
       ../../rendering/
       ../rendering/
       ../../harness/
)

if (MSVC)
    target_include_directories(enations PUBLIC
           ../../tools/sdl2/SDL2-2.30.12/include
           ../../tools/sdl2/SDL2_ttf-2.22.0/include
           ../../tools/sdl2/glew-2.2.0/include
    )

    target_link_libraries(enations PUBLIC
            vdmplay
            wind22
            wing32.lib
            vdmplay.lib
            winmm.lib
            vfw32.lib
            Netapi32.lib
            ws2_32.lib
            version.lib
            SDL2_ttf.lib
            )
else ()
    # Linux: wind22 (carries EnLinuxSDL2 transitively) + system SDL2. vdmplay
    # (VFW video) is not built on Linux — video playback is SDL2 — and NetBIOS
    # multiplayer is stubbed, so neither vdmplay nor Netapi32 is linked.
    target_link_libraries(enations PUBLIC
            wind22
            vdmplay_posix   # ported vp* TCP MP engine (replaces vdmplay_stubs.cpp)
            EnLinuxSDL2
            )
    # libvdmplay_posix.so sits next to the enations binary; $ORIGIN lets the
    # loader find it without LD_LIBRARY_PATH.
    set_target_properties(enations PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN")
endif ()

# Pick the SDL2 import-lib arch to match the build platform. VS sets
# CMAKE_GENERATOR_PLATFORM (Win32/x64); fall back to pointer size otherwise.
if(CMAKE_GENERATOR_PLATFORM STREQUAL "x64" OR CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(SDL2_ARCH x64)
else()
    set(SDL2_ARCH x86)
endif()

if (MSVC)
        # Disable SafeSEH for MSVC builds as some linked libraries do not support it.
        # SAFESEH is an x86-only concept; the x64 linker rejects the flag, so guard it.
    if(SDL2_ARCH STREQUAL "x86")
        target_link_options(enations PRIVATE "/SAFESEH:NO")
    endif()

    target_link_options(enations PUBLIC
        /NODEFAULTLIB:nafxcw.lib
        /NODEFAULTLIB:libcmt.lib

        $<$<CONFIG:Sanitize>:/DEBUG:FULL /OPT:NOREF /OPT:NOICF /fsanitize=address /LARGEADDRESSAWARE>
        $<$<CONFIG:Debug>:/DEBUG:FULL /OPT:NOREF /OPT:NOICF /LARGEADDRESSAWARE>
        $<$<CONFIG:Profile>:/DEBUG:FULL /INCREMENTAL:NO /OPT:NOREF /OPT:NOICF /LARGEADDRESSAWARE>
        $<$<CONFIG:Release>:/DEBUG:FASTLINK /INCREMENTAL:NO /OPT:REF /OPT:ICF /LTCG /LARGEADDRESSAWARE>

        /DYNAMICBASE
        /NXCOMPAT
    )

    target_link_directories(enations PUBLIC
        ../../tools/lib
        ../../cmakeBuild/tools/vdmplay/$<CONFIG>
        ../../cmakeBuild/windward/wind22/$<CONFIG>
        ./cmakeBuild/tools/vdmplay/$<CONFIG>
        ./cmakeBuild/windward/wind22/$<CONFIG>
        ../../tools/sdl2/SDL2-2.30.12/lib/${SDL2_ARCH}        # SDL2.lib (pulled via SDL2_ttf/_mixer /DEFAULTLIB)
        ../../tools/sdl2/SDL2_ttf-2.22.0/lib/${SDL2_ARCH}
        ../../tools/sdl2/SDL2_mixer-2.8.1/lib/${SDL2_ARCH}    # SDL2_mixer.lib
    )

    # Copy required DLLs to output directory (Windows only — Linux uses system .so).
    add_custom_command(TARGET enations POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/../tools/sdl2/SDL2-2.30.12/lib/${SDL2_ARCH}/SDL2.dll"
            $<TARGET_FILE_DIR:enations>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/../tools/sdl2/SDL2_mixer-2.8.1/lib/${SDL2_ARCH}/SDL2_mixer.dll"
            $<TARGET_FILE_DIR:enations>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/tools/vdmplay/$<CONFIG>/vdmplay.dll"
            $<TARGET_FILE_DIR:enations>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/../tools/sdl2/SDL2_ttf-2.22.0/lib/${SDL2_ARCH}/SDL2_ttf.dll"
            $<TARGET_FILE_DIR:enations>
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/../data/terrain_gpu"
            "$<TARGET_FILE_DIR:enations>/data/terrain_gpu"
        COMMENT "Copying runtime DLLs and baked terrain set (data/terrain_gpu) to output directory"
    )
else ()
    # Linux: no DLLs to stage, but still place the baked GPU terrain PNG set next
    # to the binary so the area map renders regardless of launch directory.
    add_custom_command(TARGET enations POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/../data/terrain_gpu"
            "$<TARGET_FILE_DIR:enations>/data/terrain_gpu"
        COMMENT "Staging baked terrain set (data/terrain_gpu) next to the binary"
    )
endif ()

# MSVC is lax about C++ standard (defaults to C++14+ even when 11 is requested),
# so the codebase uses std::make_unique (C++14) and relaxed constexpr. gcc with
# -std=c++11 rejects both — build Linux as C++17 (gcc 13's default anyway).
if (MSVC)
    set_property(TARGET enations PROPERTY CXX_STANDARD 11)
else ()
    set_property(TARGET enations PROPERTY CXX_STANDARD 17)
endif ()
