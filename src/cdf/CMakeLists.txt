
project(cdf VERSION 1.0
        DESCRIPTION "Enemy Nations Riff Creation Tool"
        LANGUAGES CXX)

set(cdf_sources
        cdf.cpp
        )


add_executable(cdf ${cdf_sources})

#target_compile_options(cdf INTERFACE
#        "$<$<CONFIG:RELEASE>:$<BUILD_INTERFACE:/GX;/Zi;/O2;/Ob2>"
#        "$<$<CONFIG:DEBUG>:$<BUILD_INTERFACE:/GX>"
#        )
target_compile_options(cdf PRIVATE
    $<$<CONFIG:RELEASE>:/GX;/Zi;/O2;/Ob2>
    $<$<CONFIG:DEBUG>:/GX>
)
#message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

#get_target_property(copts cdf COMPILE_OPTIONS)
#message("copts: ${copts}")

        
target_compile_definitions(cdf PUBLIC
        _AFXDLL
        _WIN32_WINNT=_WIN32_WINNT_WIN7
        $<$<CONFIG:Debug>:_DEBUG>
        )

set(CMAKE_MFC_FLAG 1)

#get_target_property(cdefs cdf COMPILE_DEFINITIONS)
#message("cdefs: ${cdefs}")
#
#target_include_directories(cdf PUBLIC
#        ../../tools/
#        ../../windward/wind22/include/)


set_property(TARGET cdf PROPERTY CXX_STANDARD 11)

