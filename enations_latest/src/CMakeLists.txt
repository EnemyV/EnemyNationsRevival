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
)


set(CMAKE_GENERATOR_TOOLSET "host=x86")

add_executable(enations WIN32 ${enations_sources})

# according to enations.mak this should be MT
# but i think it used to be MD?
# should be /Zi for release (no it shouldn't?)')
# /Z7 for profiling
# BUGBUGTODO change back to /Zi
target_compile_options(enations PRIVATE
    $<$<CONFIG:Release>:/MD;/EHsc;/O2;/Ob2;/Oi;/GL;/Gy;/Gw;/Ot;/GF;/arch:AVX2;/fp:fast;/Zi>
    $<$<CONFIG:Profile>:/MD;/EHsc;/O2;/Ob2;/Oi;/GL;/Gy;/Gw;/Ot;/GF;/arch:AVX2;/fp:fast;/Zi>
    $<$<CONFIG:Debug>:/MDd;/EHsc;/Zi;/Od;/Ob0>
    $<$<CONFIG:Sanitize>:/MDd;/EHsc;/Zi;/Od;/Ob0;/fsanitize=address>
)

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


# windows.h normally derives _X86_/_AMD64_ from the compiler's _M_* macros,
# but this codebase defined _X86_ by hand (see HACK note below). Pick the macro
# that matches the build platform so an x64 compile doesn't pull in the 32-bit
# SDK definitions.
if(CMAKE_GENERATOR_PLATFORM STREQUAL "x64" OR CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(EN_ARCH_DEFINE _AMD64_)
else()
    set(EN_ARCH_DEFINE _X86_)
endif()

target_compile_definitions(enations PUBLIC
        # Phase 1g step 2 (2026-05-21): _AFXDLL no longer required —
        # stdafx.h's afx*.h chain is gated out and mfc_compat.h provides
        # the MFC type surface (CString/CFile/CArchive/CMap/CDialog/CWnd/
        # CDC/CFont/CBrush/CPen/CBitmap/CList/CWinThread/CCriticalSection
        # etc.). MFC not linked: verified via ./mfc-status.ps1.
        # _AFXDLL
        _WIN32
        _WIN32_WINNT=_WIN32_WINNT_WIN7
        _MBCS
        ${EN_ARCH_DEFINE} # arch macro (_X86_ / _AMD64_), selected by platform above
        WIN32
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

target_precompile_headers(enations PRIVATE stdafx.h)


get_target_property(cdefs enations COMPILE_DEFINITIONS)
message("cdefs: ${cdefs}")
        
target_include_directories(enations PUBLIC
        ../../tools/
       ../../cmakeBuild/tools
       ./cmakeBuild/tools
       ../../windward/wind22/include
       ../../rendering/
       ../rendering/
       ../../tools/sdl2/SDL2-2.30.12/include
       ../../tools/sdl2/SDL2_ttf-2.22.0/include
       ../../tools/sdl2/glew-2.2.0/include
)


target_link_libraries(enations PUBLIC
        vdmplay
        wind22
        wing32.lib
#        msvcrt.lib # was commented
#        shdw32m.lib # was commented
#        shlw32m.lib # was commented

        vdmplay.lib
        winmm.lib
        vfw32.lib
#        shmfc4m.lib # was commented
        Netapi32.lib
        version.lib
        SDL2_ttf.lib
        )

# Pick the SDL2 import-lib arch to match the build platform. VS sets
# CMAKE_GENERATOR_PLATFORM (Win32/x64); fall back to pointer size otherwise.
if(CMAKE_GENERATOR_PLATFORM STREQUAL "x64" OR CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(SDL2_ARCH x64)
else()
    set(SDL2_ARCH x86)
endif()

        # Disable SafeSEH for MSVC builds as some linked libraries do not support it.
        # SAFESEH is an x86-only concept; the x64 linker rejects the flag, so guard it.
if(MSVC AND SDL2_ARCH STREQUAL "x86")
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
        ../../tools/sdl2/SDL2_ttf-2.22.0/lib/${SDL2_ARCH}
    )

# Copy required DLLs to output directory
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
    # Stage the baked GPU terrain PNG set next to the exe so the area map renders
    # regardless of the launch working directory (and in a standalone deploy).
    # The loader's first candidate is "data/terrain_gpu" relative to the exe.
    # copy_directory_if_different only writes changed files, so it's cheap on
    # incremental relinks.
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/../data/terrain_gpu"
        "$<TARGET_FILE_DIR:enations>/data/terrain_gpu"
    COMMENT "Copying runtime DLLs and baked terrain set (data/terrain_gpu) to output directory"
)

set_property(TARGET enations PROPERTY CXX_STANDARD 11)
