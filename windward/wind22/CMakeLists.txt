
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
        src/flcctrl.cpp
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
        src/strext.cpp
        src/subclass.cpp
        src/threads.cpp
        src/wndbase.cpp
        dibwnd.cpp
        )

add_library(wind22 STATIC ${wind22_sources})

# according to wind22.mak this should be MD
# but maybe it was changed to MT in later versions?
# i still think win22 is older or incomplete somehow
#target_compile_options(wind22 PUBLIC
#    $<$<CONFIG:Release>:/MD /EHsc /O2 /Ob2 /EHsc>
#    $<$<CONFIG:Profile>:/MD /EHsc /O2 /Ob2 /EHsc>
#    $<$<CONFIG:Debug>:/MDd /EHsc /EHsc /ZI>
#)
target_compile_options(wind22 PUBLIC    
    $<$<CONFIG:Sanitize>:/MDd /EHsc /Od /Ob0 /fsanitize=address>
    $<$<CONFIG:Debug>:/MDd /EHsc /Od /Ob0> # ZI
    $<$<CONFIG:Profile>:/MD /EHsc /Od /Ob0> # ZI
    $<$<CONFIG:Release>:/MD /EHsc /O2 /Ob2 /Oi /GL /Gy /Gw /Ot /GF /arch:AVX2 /fp:fast /Zi>
)


set(CMAKE_MFC_FLAG 2)

#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_compile_definitions(wind22 PUBLIC
    _AFXDLL
    _WIN32_WINNT=_WIN32_WINNT_WIN7
    _WINDOWS
    _MBCS
    WIN32
    _X86_ # Hack! This should not be necessary.
    #LOGGINGON
    AI_THREADS_ENABLED
    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:Profile>:NDEBUG>    
    $<$<CONFIG:Debug>:_DEBUG>
)


# Precompiled header - specify the correct path
target_precompile_headers(wind22 PUBLIC include/stdafx.h)

#target_link_options(enations PUBLIC
#    $<$<CONFIG:Debug>:/DEBUG:FULL>
#    $<$<CONFIG:Release>:/DEBUG:FASTLINK>
#    /OPT:REF
#    /OPT:ICF
#    /INCREMENTAL:NO
#    /DYNAMICBASE
#    /NXCOMPAT
#    $<$<CONFIG:Release>:/LTCG>
#)
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
        )

target_include_directories(wind22 PUBLIC
        include
        ./include
        ../../tools
        )

target_link_directories(wind22 PUBLIC
        ../../cmakeBuild/tools/lib
        )
