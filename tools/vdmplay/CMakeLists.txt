
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

set(CMAKE_MFC_FLAG 1)

#target_compile_definitions(vdmplay PUBLIC
   #     _AFXDLL
   ##     _DLL
        # _MT
   #     _USRDLL
   #     _WINDLL
   #     _WIN32_WINNT=_WIN32_WINNT_WIN7
   #     _WINDOWS
   #     _MBCS
   #     VPSYSTEM
   #     WIN32
# #       USE_MFC
  #      _X86_ # Hack! This should not be necessary.
  #      )


target_compile_definitions(vdmplay PUBLIC
#        _AFXDLL
#        _DLL
#        _AFX_STATIC
        STRICT
        _USRDLL
        _WINDLL
#        NOMFC
        _WINDOWS
        _MBCS
        WIN32
        _X86_
        VPSYSTEM
        _WIN32_WINNT=_WIN32_WINNT_WIN7
        $<$<CONFIG:Release>:NDEBUG>
        $<$<CONFIG:Debug>:_DEBUG>
        )

target_link_directories(vdmplay PUBLIC
        ./)

#target_link_libraries(vdmplay PUBLIC
##        mtlgmt
    #    netapi32
    #    winmm
    #    ucrt
    #    vcruntime
    #    mfcs140
    #    msvcprt
      #  msvcrt
##        mtlmt
      #  legacy_stdio_definitions
#)
target_link_libraries(vdmplay PUBLIC
    netapi32
    winmm
#    mtlmt.lib
    nafxcwd.lib 
    legacy_stdio_definitions
#    $<$<CONFIG:Debug>:nafxcwd.lib>       # Only for Debug
#    $<$<CONFIG:Release>:nafxcw.lib>      # Only for Release
)


#target_include_directories(vdmplay PUBLIC )

#target_compile_options(vdmplay INTERFACE
#        "$<$<CONFIG:RELEASE>:$<BUILD_INTERFACE:/MD>"
#        "$<$<CONFIG:DEBUG>:$<BUILD_INTERFACE:/MDd>"
#        )

# according to vdmplay.mak this should be MT ...
target_compile_options(vdmplay PRIVATE
    $<$<CONFIG:DEBUG>:/MTd>
    $<$<CONFIG:RELEASE>:/MT>
)
#target_compile_options(vdmplay PRIVATE
#        "$<$<CONFIG:RELEASE>:$<BUILD_INTERFACE:/MD;/GX;/Zi;/Ob2;/EHsc>"
#        "$<$<CONFIG:DEBUG>:$<BUILD_INTERFACE:/MDd;/GX;/EHsc;/Zi>"
#        )
        

target_link_options(vdmplay PUBLIC
       # /NODEFAULTLIB:libcmt
       # /NODEFAULTLIB:LIBCMTD
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