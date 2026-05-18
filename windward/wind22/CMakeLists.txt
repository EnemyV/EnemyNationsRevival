
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
        src/strext.cpp
        src/subclass.cpp
        src/threads.cpp
        src/w22_settings.cpp
        src/wndbase.cpp
        src/wndstub.cpp
        dibwnd.cpp
        )

add_library(wind22 STATIC ${wind22_sources})

# according to wind22.mak this should be MD
# but maybe it was changed to MT in later versions?
# i still think win22 is older or incomplete somehow
target_compile_options(wind22 PUBLIC    
    $<$<CONFIG:Sanitize>:/MDd /EHsc /Od /Ob0 /fsanitize=address>
    $<$<CONFIG:Debug>:/MDd /EHsc /Od /Ob0> # ZI
    $<$<CONFIG:Profile>:/MD /EHsc /Od /Ob0> # ZI
    $<$<CONFIG:Release>:/MD /EHsc /O2 /Ob2 /Oi /GL /Gy /Gw /Ot /GF /arch:AVX2 /fp:fast /Zi>
)

set(CMAKE_MFC_FLAG 2)

target_compile_definitions(wind22 PUBLIC
    _AFXDLL
    _WIN32_WINNT=_WIN32_WINNT_WIN7
    _WINDOWS
    _MBCS
    WIN32
    _X86_ # Hack! This should not be necessary.
    #LOGGINGON
    AI_THREADS_ENABLED
    # ENATIONS_USE_STUB_WND  # Phase 1: flip this on to swap CWndBase to non-MFC CWndStub
    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:Profile>:NDEBUG>
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Sanitize>:_DEBUG>
)


# Precompiled header - specify the correct path
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
        )

target_include_directories(wind22 PUBLIC
        include
        ./include
        ../../tools
        ../../tools/sdl2/SDL2-2.30.12/include
        ../../tools/sdl2/SDL2_mixer-2.8.1/include
        )

target_link_directories(wind22 PUBLIC
        ../../cmakeBuild/tools/lib
        ../../tools/sdl2/SDL2-2.30.12/lib/x86
        ../../tools/sdl2/SDL2_mixer-2.8.1/lib/x86
        )
