message("Current CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")

set(enations_sources
#        advanced.cpp
#        advdlg.cpp
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
        citizen.cpp
        cpathmap.cpp
        cpathmgr.cpp
        creatmul.cpp
        creatsin.cpp
        credits.cpp
        cutscene.cpp
#        dlgflic.cpp
        DlgMsg.cpp
        DlgReg.cpp
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
        loadtruk.cpp
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
        tstsnds.cpp
        ui.cpp
        unit.cpp
        unit_wnd.cpp
        vehicle.cpp
        vehmove.cpp
        vehoppo.cpp
        vpxfer.cpp
        world.cpp
        wrldinit.cpp mem_pool.h)


set(CMAKE_GENERATOR_TOOLSET "host=x86")

add_executable(enations WIN32 ${enations_sources})

#target_compile_options(enations INTERFACE
#        "$<$<CONFIG:RELEASE>:$<BUILD_INTERFACE:/MD;/EHsc;/Zi;/O2;/Ob2;/DEBUG>"
#        "$<$<CONFIG:DEBUG>:$<BUILD_INTERFACE:/MD;/EHsc>"
#        )
#target_compile_options(enations INTERFACE
#        "$<$<CONFIG:RELEASE>:$<BUILD_INTERFACE:/MT;/EHsc;/Zi;/O2;/Ob2>" # should be /NDEBUG??
#        "$<$<CONFIG:DEBUG>:$<BUILD_INTERFACE:/MTd;/EHsc;/Zi>"
#        )

target_compile_options(enations INTERFACE
    $<$<CONFIG:Release>:/MT /EHsc /O2 /Ob2 /Zi>
    $<$<CONFIG:Debug>:/MTd /EHsc /Zi>
)

set(CMAKE_MFC_FLAG 1)

#target_compile_options(enations PRIVATE
#    $<$<CONFIG:RELEASE>:/MD;/EHsc;/Zi;/O2;/Ob2>
#    $<$<CONFIG:DEBUG>:/MDd;/EHsc;/Zi>
#)
message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
set_target_properties(enations PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

get_target_property(copts enations COMPILE_OPTIONS)

message("copts: ${copts}")

add_compile_definitions(
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)


target_compile_definitions(enations PUBLIC
     #   _AFXDLL
        _WIN32
        _WIN32_WINNT=_WIN32_WINNT_WIN7
        _MBCS
        _X86_ # HACK! This should not need to be defined here!
        WIN32
        LOGGINGON        
        $<$<CONFIG:Release>:NDEBUG>
        $<$<CONFIG:Debug>:_DEBUG>
        )

target_precompile_headers(enations PRIVATE stdafx.h)


get_target_property(cdefs enations COMPILE_DEFINITIONS)
message("cdefs: ${cdefs}")

#arget_include_directories(enations PUBLIC
     #   .
     #   ./tools/
     #   ../../cmake-build-release/tools
       # ../../cmake-build-release/windward/wind22/Debug
      #  ../../windward/wind22/include)
        
target_include_directories(enations PUBLIC
        ../../tools/
       ../../cmake-build-release/tools
       ../../windward/wind22/include)


target_link_libraries(enations PUBLIC
        ddraw.lib
        mss32.lib
        wing32.lib
#        msvcrt.lib # was commented
#        shdw32m.lib # was commented
#        shlw32m.lib # was commented
        wind22.lib
        vdmplay.lib
        winmm.lib
        vfw32.lib
#        shmfc4m.lib # was commented
        Netapi32.lib
        version.lib
        )

target_link_options(enations PUBLIC
        /NODEFAULTLIB:nafxcw.lib
        /NODEFAULTLIB:libcmt.lib
        /DEBUG
        /OPT:REF # Remove unreferenced functions/data.
        /OPT:ICF # Identical COMDAT Folding / shrinks size
        /INCREMENTAL:NO
        )
        
        
        
#if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#    target_link_directories(enations PUBLIC
#        .
#        ../../tools/lib
#        ../../cmake-build-release/tools/vdmplay/Debug
#        ../../cmake-build-release/windward/wind22/Debug
#    )
#else()
  #  target_link_directories(enations PUBLIC
  #      .
  #      ../../tools/lib
  #      ../../cmake-build-release/tools/vdmplay/
  #      ../../cmake-build-release/windward/wind22/
 #   )
#endif()



    target_link_directories(enations PUBLIC        
      # "E:/Games/Enemy Nations Source/EnemyNations-master/tools/lib"
        ../../cmake-build-release/tools/lib
        ../../cmake-build-release/tools/vdmplay/$<CONFIG>
        ../../cmake-build-release/windward/wind22/$<CONFIG>
    )

set_property(TARGET enations PROPERTY CXX_STANDARD 11)
