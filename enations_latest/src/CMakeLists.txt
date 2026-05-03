message("Current CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")

set(enations_sources
#        advanced.cpp
        ai.cpp
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
        chat.cpp
        chatbar.cpp
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
        ipcchat.cpp
        ipccomm.cpp
        ipcmsg.cpp
        ipcplay.cpp
        ipcread.cpp
        ipcsend.cpp
        join.cpp
        lastplnt.cpp
        lastplnt.rc
        license.cpp
        main.cpp
        mainloop.cpp
        minerals.cpp
        movie.cpp
        netapi.cpp
        netcmd.cpp
        new_game.cpp
        new_unit.cpp
        newworld.cpp
        options.cpp
        pickrace.cpp
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
        GameWindow.cpp
        RenderingAdapter.cpp
        SDL2MainMenu.cpp
        SDL2UI.cpp
        SDL2Options.cpp
        SDL2Dialogs.cpp
        SDL2Video.cpp
        SDL2Panel.cpp
        SDL2Compositor.cpp
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

set(CMAKE_MFC_FLAG 2)

message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")


get_target_property(copts enations COMPILE_OPTIONS)

message("copts: ${copts}")

add_compile_definitions(
    $<$<CONFIG:Sanitize>:_DEBUG>
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:Profile>:NDEBUG> # disables asserts in profile builds
)


target_compile_definitions(enations PUBLIC
        _AFXDLL
        _WIN32
        _WIN32_WINNT=_WIN32_WINNT_WIN7
        _MBCS
        _X86_ # HACK! This should not need to be defined here!
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
        ddraw.lib
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

        # Disable SafeSEH for MSVC builds as some linked libraries do not support it
        # For Legacy 32-bit codebases. When we switch to 64 bit, remove this
if(MSVC)
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
        ../../tools/sdl2/SDL2_ttf-2.22.0/lib/x86
    )

# Copy required DLLs to output directory
add_custom_command(TARGET enations POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/../tools/sdl2/SDL2-2.30.12/lib/x86/SDL2.dll"
        $<TARGET_FILE_DIR:enations>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/../tools/sdl2/SDL2_mixer-2.8.1/lib/x86/SDL2_mixer.dll"
        $<TARGET_FILE_DIR:enations>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/tools/vdmplay/$<CONFIG>/vdmplay.dll"
        $<TARGET_FILE_DIR:enations>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/../tools/sdl2/SDL2_ttf-2.22.0/lib/x86/SDL2_ttf.dll"
        $<TARGET_FILE_DIR:enations>
    COMMENT "Copying SDL2.dll, SDL2_mixer.dll, SDL2_ttf.dll, and vdmplay.dll to output directory"
)

set_property(TARGET enations PROPERTY CXX_STANDARD 11)
