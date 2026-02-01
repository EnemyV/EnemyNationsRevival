
###############################
# vdmplay.dll
###############################

set(vdmplay_sources
#        advanced.cpp
        advdlg.cpp
        astring.cpp
#        cstring.cpp
        commnet.cpp
        comstatd.cpp
        datalink.cpp
        datalog.cpp
        dialer.cpp
        dpnet.cpp
        dpengine.cpp
        frcoder.cpp
        mdmdrvr.cpp
        mdmnet.cpp
        nbnet.cpp
        smap.cpp
        stdafx.cpp
        tapiutil.cpp
        tapinet.cpp
        tcpnet.cpp
        vdmplay.cpp
#        vdmplay.def
        vdmplay.rc
        vpengine.cpp
        vpnet.cpp
        vputil.cpp
        vpwinsk.cpp
        w32comm.cpp
        wsipxnet.cpp
        )

add_library(vdmplay SHARED ${vdmplay_sources})

set(CMAKE_MFC_FLAG 2)

target_compile_definitions(vdmplay PUBLIC
        _AFXDLL
        STRICT
        _USRDLL
        _WINDLL
        _WINDOWS
        _MBCS
        WIN32
        _X86_
        VPSYSTEM
        _WIN32_WINNT=_WIN32_WINNT_WIN7
        $<$<CONFIG:Release>:NDEBUG>
        $<$<CONFIG:Profile>:NDEBUG>
        $<$<CONFIG:Sanitize>:_DEBUG>
        $<$<CONFIG:Debug>:_DEBUG>
        )

target_link_directories(vdmplay PUBLIC
        ./)

target_link_libraries(vdmplay PUBLIC
    netapi32
    winmm
    legacy_stdio_definitions
)


# according to vdmplay.mak this should be MT ...
target_compile_options(vdmplay PRIVATE
    $<$<CONFIG:DEBUG>:/MDd /ZI>
    $<$<CONFIG:Sanitize>:/MDd /ZI> # /DEBUG:FULL /OPT:NOREF /OPT:NOICF>
    $<$<CONFIG:Release>:/MD /EHsc /O2 /Ob2 /Oi /GL /Gy /Gw /Ot /GF /arch:AVX2 /fp:fast /Zi>
    $<$<CONFIG:Profile>:/MD>
)

target_link_options(vdmplay PUBLIC
        $<$<CONFIG:Sanitize>:/DEBUG:FULL /fsanitize=address>
        $<$<CONFIG:Release>:/DEBUG:FASTLINK /INCREMENTAL:NO /OPT:REF /OPT:ICF /LTCG>
        /NODEFAULTLIB:mtlgmt
        )

###############################
# iserve.exe
###############################

#add_subdirectory(ISERVE)

###############################
# vdmplui.dll
###############################

#add_subdirectory(VDMPLUI)

###############################
# vpdiag.exe
###############################

#add_subdirectory(VPDIAG)

message(STATUS "AFXDLL is defined: $ENV{_AFXDLL}")