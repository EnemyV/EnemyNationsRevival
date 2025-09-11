
set(sprite_sources
        bd.cpp
        bitbuffe.cpp
        bmpfile.cpp
        boxsplit.cpp
        bpecodec.cpp
        cdirdlg.cpp
        codec.cpp
        drawdib.cpp
        holdtime.cpp
        huffmanc.cpp
        image.cpp
        lzsscode.cpp
        lzwcodec.cpp
        mainfrm.cpp
        medcut.cpp
        minvarq.cpp
        multprog.cpp
        namngdlg.cpp
        paldoc.cpp
        palview.cpp
        progdlg.cpp
        rbuild.cpp
        rgbcolor.cpp
        sp.cpp
        splitwnd.cpp
        spprop.cpp
        sprcnvrt.cpp
        sprd.cpp
        sprite.cpp
        spritvw.cpp
        sptrctrl.cpp
        statusdlg.cpp
        stdafx.cpp
        stucki.cpp
        td.cpp
        terrain.cpp
        tgafile.cpp
        treeview.cpp
        v.cpp
        varcut.cpp
        vd.cpp
        sprite.rc)

add_executable(sprite WIN32 ${sprite_sources})

target_compile_options(sprite INTERFACE
        "$<$<CONFIG:RELEASE>:$<BUILD_INTERFACE:/MD;/GX;/Zi;/O2;/Ob2;/EHsc>"
        "$<$<CONFIG:DEBUG>:$<BUILD_INTERFACE:/MD;/GX;/EHsc>"
        )

message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

get_target_property(copts sprite COMPILE_OPTIONS)
message("copts: ${copts}")


        

target_compile_definitions(sprite PUBLIC
        _AFXDLL
        _WIN32_WINNT=_WIN32_WINNT_WIN7
        _WINDOWS
        _MBCS
        WIN32
        $<$<CONFIG:Release>:NDEBUG>
        $<$<CONFIG:Debug>:_DEBUG>
        )
        
set(CMAKE_MFC_FLAG 1)

target_link_libraries(sprite PUBLIC
        winmm.lib)

#target_link_options(sprite PUBLIC)

get_target_property(cdefs sprite COMPILE_DEFINITIONS)
message("cdefs: ${cdefs}")

target_include_directories(sprite PUBLIC
        ../../../tools
        ../../windward/wind22/include/)


set_property(TARGET sprite PROPERTY CXX_STANDARD 11)
