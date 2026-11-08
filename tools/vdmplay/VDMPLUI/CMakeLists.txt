
###############################
# vdmplui
###############################

set(vdmplui_sources
        STDAFX.CPP
        VDMPLUI.CPP
        VDMPLUI.RC
        )

add_library(vdmplui SHARED ${vdmplui_sources})

target_include_directories(vdmplui PUBLIC
        ../
        )

        
# Use MFC in shared DLL for Debug/Release
target_compile_definitions(vdmplui PUBLIC
        _AFXDLL
        _WINDOWS
        WIN32
        _MBCS
        _WIN32_WINNT=_WIN32_WINNT_WIN7
        )

set(CMAKE_MFC_FLAG 1)

target_compile_options(vdmplui PRIVATE
        $<$<CONFIG:DEBUG>:/MDd>
        $<$<CONFIG:RELEASE>:/MD>
        )