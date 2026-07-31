message("Current CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")

# Dev cheat surface. OFF by default: a debug build is NOT a cheat build.
# Turn on deliberately with -DEN_CHEAT=ON (needed for the in-dev End-key
# spectator mode). See the _CHEAT note in target_compile_definitions below.
option(EN_CHEAT "Compile the dev cheat surface (_CHEAT): map reveal, research grants, Max*, AI unit grants, End-key spectator, debug overlays" OFF)

set(enations_sources
#        advanced.cpp
        ai.cpp
        aisnap.cpp   # Tier-B AI world snapshot (lock-free AI reads)
        altoutput.cpp   # reusable alternate-output toggle system (BioFuel, Coal Liquefaction, ...)
        area.cpp
        bmbutton.cpp
        edicts.cpp   # Edicts v1 static catalog (data model; see docs/plans/edicts-feature-plan.md)
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
        cpufeatures.cpp   # runtime ISA detection (GH #8) — the binary is baseline x86-64; anything wider dispatches through EnCpu
        EnSettings.cpp
        Perf.cpp
        Perf.h
        GameWindow.cpp
        framecap.cpp
        framecap.h
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
        x11_transient_linux.cpp            # X11 WM_TRANSIENT_FOR owner relationship for detached panels (replaces ALWAYS_ON_TOP)
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
    # -Bsymbolic is GNU ld only; the macOS linker (ld64) rejects it as an unknown
    # option ("ld: unknown options: -Bsymbolic"). On macOS the same isolation is
    # the default: two-level namespace binds each dylib's refs to its own defs and
    # prevents the exe's same-named symbols from interposing — so no flag needed.
    if (APPLE)
        # macOS: the vp* engine references win32_compat shim symbols (Critical-
        # Section, GetTickCount, PostMessage, …) that live in the main exe. ELF
        # resolves such .so->exe refs at load by default; the macOS linker does
        # not — it needs -undefined dynamic_lookup to defer them to load-time
        # lookup in the host process (where the exe's default-visibility shim
        # symbols satisfy them). -fvisibility=hidden still keeps the dylib's own
        # CPlayer/CSession internals private, so no collision with the exe.
        target_link_options(vdmplay_posix PRIVATE -undefined dynamic_lookup)
    else ()
        target_link_options(vdmplay_posix PRIVATE -Wl,-Bsymbolic)
    endif ()
    set_target_properties(vdmplay_posix PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

    # ---- ISERVE headless registration server (cross-platform, no MFC) ----------
    # Standalone CLI daemon: hosts register their game, clients SenumREQ the list
    # (VDMPLAY registration server, TCP 1707). OPTIONAL — the game works with or
    # without it (broadcast + direct Join unchanged). Linux-verified listening on
    # 1707. win32_compat.cpp is compiled IN so the vdmplay_posix .so's shim refs
    # (PostMessage/CRITICAL_SECTION/timeGetTime/…) resolve from this exe at load
    # (--export-dynamic). The legacy MFC GUI (tools/vdmplay/ISERVE/) stays frozen.
    add_executable(iserve
        ../../tools/vdmplay/iserve_headless/iserve_core.cpp
        ../../tools/vdmplay/iserve_headless/iserve_main.cpp
        ../../windward/wind22/src/win32_compat.cpp)
    target_include_directories(iserve PRIVATE
        ../../tools/vdmplay
        ../../windward/wind22/include
        ../../windward/wind22/include/winshim
        ../../tools/)
    target_compile_options(iserve PRIVATE
        -g -fms-extensions -fsigned-char -fpermissive
        -Wno-unknown-pragmas -Wno-format -Wno-write-strings -Wno-narrowing)
    target_link_libraries(iserve PRIVATE vdmplay_posix pthread ${CMAKE_DL_LIBS})
    if (APPLE)
        target_link_options(iserve PRIVATE -Wl,-export_dynamic)
    else ()
        target_link_libraries(iserve PRIVATE rt)
        target_link_options(iserve PRIVATE -Wl,--export-dynamic)
    endif ()
    # Co-locate libvdmplay_posix.{dylib,so} next to the binary and find it via the
    # platform's co-located-rpath token: $ORIGIN on ELF/Linux, @loader_path on macOS.
    # ($ORIGIN is INERT on Darwin — dyld does not expand it — so a portable mac build
    # needs @loader_path, else @rpath/libvdmplay_posix.dylib only resolves via the abs
    # build path and breaks once the build is relocated/shipped. mac2, 2026-06-26.)
    set_target_properties(iserve PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        BUILD_RPATH   "$<IF:$<PLATFORM_ID:Darwin>,@loader_path,$ORIGIN>"
        INSTALL_RPATH "$<IF:$<PLATFORM_ID:Darwin>,@loader_path,$ORIGIN>")
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
        # NO /arch: flag => baseline x86-64 (SSE2, guaranteed on every x64 CPU).
        # /arch:AVX2 used to be here. It arrived in @66195ab9 "tweak release compile
        # flags" with no benchmark and no rationale, and it made the shipped Windows
        # build REFUSE TO START on any CPU older than Intel Haswell (2013) / AMD
        # Excavator (2015): MSVC emitted AVX2 throughout (2723 ymm instructions, 179
        # AVX2-specific like vpermq/vpbroadcastd, the first at the very start of .text),
        # so an older CPU takes an illegal-instruction fault during init and the process
        # disappears with no dialog and no log — reported as "a brief 'Loading Second
        # Chance...' then it closes" (GH #8). Self-compiling did not help, because the
        # flag lives here. Any AVX2 use must now be RUNTIME-DISPATCHED via EnCpu (see
        # cpufeatures.h) so the baseline binary keeps running everywhere.
        $<$<CONFIG:Release>:/MD;/EHsc;/O2;/Ob2;/Oi;/GL;/Gy;/Gw;/Ot;/GF;/fp:fast;/Zi>
        $<$<CONFIG:Profile>:/MD;/EHsc;/O2;/Ob2;/Oi;/GL;/Gy;/Gw;/Ot;/GF;/fp:fast;/Zi>
        $<$<CONFIG:Debug>:/MDd;/EHsc;/Zi;/O2;/Ob2>
        $<$<CONFIG:Sanitize>:/MDd;/EHsc;/Zi;/Od;/Ob0;/fsanitize=address>
    )
else ()
    # Linux/gcc. -fms-extensions + -fsigned-char match MSVC semantics the code
    # relies on; -Wno-unknown-pragmas swallows #pragma warning/comment/intrinsic.
    # -ffp-contract=off: no FMA fusing — MP world-gen parity requires float
    # expressions to round identically everywhere; clang on Apple ARM64 fuses
    # a*b+c by default (x86 SSE2 and MSVC don't), silently diverging mac maps.
    target_compile_options(enations PRIVATE
        -g -fms-extensions -fsigned-char -fpermissive -ffp-contract=off -Wno-unknown-pragmas -Wno-format -Wno-write-strings -Wno-narrowing
        $<$<CONFIG:Release>:-O2>
        $<$<CONFIG:Profile>:-O2>
    )
    # Force-include stdafx.h — mirrors MSVC's /Yu forced-PCH include so TUs that
    # include game headers before stdafx.h still get the precompiled surface.
    target_compile_options(enations PRIVATE -include stdafx.h)
endif ()

# Cross-platform world-gen FP determinism (MP RAND MISMATCH, 2026-07-03):
# NOTE 2026-07-30: /arch:AVX2 has since been REMOVED from this target (GH #8 —
# it made the build unstartable on pre-2013 CPUs). Without it MSVC has no FMA to
# contract into, so the primary divergence source described below is gone. The
# /fp:precise pin is KEPT anyway: /fp:fast alone still permits reassociation and
# reciprocal substitution, which can diverge from POSIX -ffp-contract=off on its
# own. Do not remove it without re-running the 3-node [wg] seed diff.
# the enations target compiled Release/Profile with /fp:fast /arch:AVX2, which
# let MSVC CONTRACT a*b+c into a fused multiply-add (single rounding) and
# reassociate float expressions in the terrain-altitude math. POSIX builds use
# -ffp-contract=off, so gcc(Linux) and clang(mac) agree and MSVC is the lone
# outlier: on some wtype=4 seeds the terrain stage computes a different altitude
# (map hash diverges while the RNG fingerprint is still identical), which later
# gates a different RandNum draw count -> CmdPlay RAND MISMATCH, client dropped.
# Confirmed by the 3-node [wg] diff on seed 384bf615 (blocks identical; terrain
# rand-equal/map-differs = the pure float-value signature). Scope ONLY the two
# world-gen TUs to /fp:precise (kills contraction AND /fp:fast's reassociation/
# reciprocal tricks) — appended AFTER the target's /fp:fast so cl.exe's last-wins
# rule makes it override. Rest of the game keeps /fp:fast (perf). Gen-time only.
if (MSVC)
    set_source_files_properties(
        wrldinit.cpp
        wrldgen.cpp
        PROPERTIES COMPILE_OPTIONS "/fp:precise")
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
        # _CHEAT = the dev cheat surface (map reveal, KnowItAll/GrantResearch, Max*, the AI unit
        # grants, the in-dev End-key spectator, the debug overlays). It used to be defined for
        # EVERY Debug/Sanitize build, on the argument that each cheat is separately opt-in via the
        # [Cheat] registry section so it was "safe by construction". It is not: a stale
        # [Cheat]SeeAll=1 left in the registry silently revealed the whole map in every debug
        # build (operator hit this while testing Training Grounds, 2026-07-25). Now an explicit
        # opt-in, default OFF:  cmake -B cmakeBuild-x64 -A x64 -DEN_CHEAT=ON
        # Still fenced to Debug/Sanitize so a cheat can never reach a Release artifact even if
        # someone turns the option on.
        $<$<AND:$<BOOL:${EN_CHEAT}>,$<OR:$<CONFIG:Debug>,$<CONFIG:Sanitize>>>:_CHEAT>
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
            # NOTE: no bare vdmplay.lib here — the `vdmplay` target above already
            # links the import lib from THIS build tree. The bare name only ever
            # resolved via the hardcoded ../../cmakeBuild/ link dir (the OLD build
            # dir name), i.e. a stale lib: fresh clones/worktrees have no
            # cmakeBuild/ and died with LNK1181.
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
    if (NOT APPLE)
        # libX11 for the X11 WM_TRANSIENT_FOR owner relationship used by
        # x11_transient_linux.cpp (panels owned-by, not above-everything). Linux only.
        find_package(X11 REQUIRED)
        target_link_libraries(enations PUBLIC ${X11_LIBRARIES})
    else ()
        # libobjc for the CoreAnimation transaction flush in SDL2CreateStatus.cpp
        # (keeps the creating-world dialog painting while the load loop is busy).
        target_link_libraries(enations PUBLIC objc)
    endif ()
    # libvdmplay_posix.{dylib,so} sits next to the enations binary; the loader finds
    # it via the platform's co-located-rpath token: $ORIGIN on ELF/Linux, @loader_path
    # on macOS ($ORIGIN is inert on Darwin — see the iserve rpath note above).
    set_target_properties(enations PROPERTIES
        BUILD_RPATH "$<IF:$<PLATFORM_ID:Darwin>,@loader_path,$ORIGIN>"
        INSTALL_RPATH "$<IF:$<PLATFORM_ID:Darwin>,@loader_path,$ORIGIN>")
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
        # (removed the hardcoded ../../cmakeBuild/ + ./cmakeBuild/ vdmplay/wind22
        # link dirs: both are linked as CMake TARGETS above, which supplies the
        # import libs from the CURRENT build tree; the old-name dirs only ever
        # served stale artifacts and broke fresh clones/other build-dir names)
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

# Stage the original .CUR cursor art next to the binary. On POSIX the Win32 GDI
# cursor pipeline (GetIconInfo/GetDIBits) is stubbed, so CWndArea loads the .cur
# files directly from a res/ dir found relative to the binary (area.cpp
# FindCursorDir / LoadCurFromFile). Without these the custom in-game cursors —
# select reticle, attack target, road, goto/move — silently fall back to the
# arrow. Windows compiles the cursors into the exe via lastplnt.rc, so this is a
# no-op there, but harmless (copy_if_different). Cursors-only, not the whole res/
# (skips the .rc/.dib/.bmp source art we don't need at runtime).
file(GLOB EN_CURSOR_FILES "${CMAKE_CURRENT_SOURCE_DIR}/res/*.cur"
                          "${CMAKE_CURRENT_SOURCE_DIR}/res/*.CUR")
# T-0073: the bundled UI font must ship NEXT TO THE BINARY too. EnResolveFontPath()
# tries "<exedir>/res/DejaVuSans.ttf" FIRST so the game does not depend on the host
# distribution's font layout (Fedora/Arch/openSUSE keep DejaVu somewhere else, and a
# bare system may carry no font at all -> the game rendered NO TEXT). Without this
# glob the file sits in source only, the first two candidates always miss, and the
# last-resort safety net silently never ships. Licence text rides along with it.
file(GLOB EN_FONT_FILES   "${CMAKE_CURRENT_SOURCE_DIR}/res/*.ttf"
                          "${CMAKE_CURRENT_SOURCE_DIR}/res/*.TTF"
                          "${CMAKE_CURRENT_SOURCE_DIR}/res/DejaVuSans.LICENSE.txt")
add_custom_command(TARGET enations POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:enations>/res"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EN_CURSOR_FILES} ${EN_FONT_FILES}
        "$<TARGET_FILE_DIR:enations>/res"
    COMMENT "Staging .cur cursor art + the bundled UI font (res/) next to the binary"
)

# Stage the intro/logo videos the menu's PlayVideo loads at runtime (assets/videos/*.mpg).
# NOT in ENATIONS.DAT — PlayVideo reads loose files (CWD then exe dir) and silently no-ops if
# missing, which was bug #53 "Replay Introduction does nothing". The real clips are the *_44k.mpg
# in enations/disk/avi (the bare intro.mpg/logo.mpg there are 0-byte stubs). copy_if_different so
# the 18MB intro only copies once. Cross-platform (POSIX needs them too once paths are forward-slash).
add_custom_command(TARGET enations POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:enations>/assets/videos"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/../../enations/disk/avi/intro_44k.mpg"
        "$<TARGET_FILE_DIR:enations>/assets/videos/intro.mpg"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/../../enations/disk/avi/logo_44k.mpg"
        "$<TARGET_FILE_DIR:enations>/assets/videos/logo.mpg"
    # App icon for the macOS Dock (res/main.ico's 32x32 converted to PNG; a bare
    # Mach-O binary otherwise gets the generic exec icon — GameWindow sets it at
    # runtime via NSApplication). Harmless on other platforms.
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/res/appicon.png"
        "$<TARGET_FILE_DIR:enations>/assets/appicon.png"
    COMMENT "Staging intro/logo videos to assets/videos (PlayVideo runtime assets, bug #53)"
)

# Generated README.txt for the release zips. These used to be hand-written per
# platform under three different names (README-FIRST.txt / README.txt / none on
# mac) so they drifted independently: the 3.00.009 Linux zip shipped 3.00.000-era
# text stamped "release 3.00.008" that still told users Linux had no multiplayer.
# Stamped from VER_STRING so it cannot go stale, same filename on all three.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/version.h" EN_VERSION_H)
string(REGEX MATCH "VER_STRING[ \t]+\"([0-9]+\\.[0-9]+\\.[0-9]+)\"" _en_ver_match "${EN_VERSION_H}")
set(EN_VERSION "${CMAKE_MATCH_1}")
if (NOT EN_VERSION)
    message(FATAL_ERROR "packaging: could not parse VER_STRING from version.h")
endif ()

if (WIN32)
    set(EN_PLATFORM "Windows x64")
    set(EN_RUN "enations.exe")
    set(EN_REQS "  - 64-bit Windows")
    set(EN_CONTENTS "  enations.exe   - the game binary (MSVC x64 Release)\n  SDL2*.dll      - SDL2 runtime libraries\n  vdmplay.dll    - video playback\n  ENations.dat   - bundled game data (~516 MB), no separate download needed\n  data/          - baked terrain set\n  res/           - cursor art\n  assets/        - intro/logo videos")
elseif (APPLE)
    set(EN_PLATFORM "macOS ARM64")
    set(EN_RUN "./enations")
    set(EN_REQS "  - macOS on Apple Silicon (ARM64)")
    set(EN_CONTENTS "  enations       - the game binary (clang ARM64 Release)\n  *.dylib        - bundled SDL2 and codec libraries\n  ENations.dat   - bundled game data (~516 MB), no separate download needed\n  data/          - baked terrain set\n  res/           - cursor art\n  assets/        - intro/logo videos")
else ()
    set(EN_PLATFORM "Linux x64")
    set(EN_RUN "./enations")
    set(EN_REQS "  - 64-bit Linux (x86-64), glibc\n  - System libs: SDL2, SDL2_ttf, SDL2_mixer\n      Debian/Ubuntu:  sudo apt install libsdl2-2.0-0 libsdl2-ttf-2.0-0 libsdl2-mixer-2.0-0")
    set(EN_CONTENTS "  enations       - the game binary (gcc x64 Release)\n  ENations.dat   - bundled game data (~516 MB), no separate download needed\n  data/          - baked terrain set\n  res/           - cursor art\n  assets/        - intro/logo videos")
endif ()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../../packaging/README.txt.in"
               "${CMAKE_CURRENT_BINARY_DIR}/README.txt" @ONLY)

add_custom_command(TARGET enations POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_BINARY_DIR}/README.txt"
        "$<TARGET_FILE_DIR:enations>/README.txt"
    COMMENT "Staging generated README.txt (${EN_VERSION}, ${EN_PLATFORM}) next to the binary"
)

# MSVC is lax about C++ standard (defaults to C++14+ even when 11 is requested),
# so the codebase uses std::make_unique (C++14) and relaxed constexpr. gcc with
# -std=c++11 rejects both — build Linux as C++17 (gcc 13's default anyway).
if (MSVC)
    set_property(TARGET enations PROPERTY CXX_STANDARD 11)
else ()
    set_property(TARGET enations PROPERTY CXX_STANDARD 17)
endif ()
