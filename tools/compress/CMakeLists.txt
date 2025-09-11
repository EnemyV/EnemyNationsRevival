
project(compit VERSION 1.0
        DESCRIPTION "Enemy Nations Riff Creation Tool"
        LANGUAGES CXX)

set(compit_sources
        lzwcodec.cpp
        main.cpp
        stdafx.cpp
        )


add_executable(compit ${compit_sources})

target_compile_options(compit INTERFACE
        "$<$<CONFIG:RELEASE>:$<BUILD_INTERFACE:/GX;/Zi;/O2;/Ob2>"
        "$<$<CONFIG:DEBUG>:$<BUILD_INTERFACE:/GX>"
        )

#message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

#get_target_property(copts cdf COMPILE_OPTIONS)
#message("copts: ${copts}")

target_compile_definitions(compit PUBLIC
        _AFXDLL
        _WIN32_WINNT=_WIN32_WINNT_WIN7
        $<$<CONFIG:Debug>:_DEBUG>
        )

set(CMAKE_MFC_FLAG 1)

#get_target_property(cdefs cdf COMPILE_DEFINITIONS)
#message("cdefs: ${cdefs}")
#
target_include_directories(compit PUBLIC
        ..
        ../../windward/wind22/include)


target_link_libraries(compit PUBLIC
        wind22.lib)

#target_link_directories(compit PUBLIC
#        ../../cmake-build-release/windward/wind22)
        

    target_link_directories(compit PUBLIC
        ../../cmake-build-release/windward/wind22/
    )

set_property(TARGET compit PROPERTY CXX_STANDARD 11)

