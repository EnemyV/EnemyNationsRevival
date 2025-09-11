message("Current CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
###############################
# vpdiag.exe
###############################

set(vpdiag_sources
        VPDIAG.CPP
        VPDIAGDL.CPP
        STDAFX.CPP
        vpdiag.rc
        ../vpwinsk.cpp
        NET.CPP
        NBDLG.CPP
        TCPDLG.CPP
        ../vputil.cpp
        )

set(CMAKE_GENERATOR_TOOLSET "host=x86")
        
add_executable(vpdiag WIN32 ${vpdiag_sources})

target_link_libraries(vpdiag PUBLIC
#        ddraw.lib
#        mss32.lib
#        wing32.lib
#        msvcrt.lib
#        shdw32m.lib
#        shlw32m.lib
#        wind22.lib
#        vdmplay.lib
#        winmm.lib
#        vfw32.lib
#        shmfc4m.lib
        Netapi32.lib
#        version.lib
        )

message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")


target_compile_definitions(vpdiag PUBLIC
        _AFXDLL
        _WIN32
        _WIN32_WINNT=_WIN32_WINNT_WIN7
        _MBCS
        _X86_ # HACK! This should not need to be defined here!
        WIN32
        LOGGINGON
        $<$<CONFIG:Release>:NDEBUG>
        $<$<CONFIG:Debug>:_DEBUG>
        )
        
target_compile_options(vpdiag INTERFACE
        "$<$<CONFIG:RELEASE>:$<BUILD_INTERFACE:/MT;/EHsc;/Zi;/O2;/Ob2;/DEBUG>"
        "$<$<CONFIG:DEBUG>:$<BUILD_INTERFACE:/MTd;/EHsc;/Zi>"
        )


set(CMAKE_MFC_FLAG 1)

target_include_directories(vpdiag PUBLIC
        ../
        )
        
set_property(TARGET vpdiag PROPERTY CXX_STANDARD 11)
