
project(makeriff VERSION 1.0
                 DESCRIPTION "Enemy Nations Riff Creation Tool"
                 LANGUAGES CXX)

set(makeriff_sources
        makeriff.cpp
        rifffile.cpp
        stdafx.cpp
        )



add_executable(makeriff ${makeriff_sources})

target_compile_options(makeriff INTERFACE
        "$<$<CONFIG:RELEASE>:$<BUILD_INTERFACE:/MD;/EHsc;/Zi;/O2;/Ob2>"
        "$<$<CONFIG:DEBUG>:$<BUILD_INTERFACE:/MD;/EHsc>"
        )

message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

get_target_property(copts makeriff COMPILE_OPTIONS)
message("copts: ${copts}")

target_compile_definitions(makeriff PUBLIC
        _AFXDLL
        _WIN32_WINNT=_WIN32_WINNT_WIN7
        )

get_target_property(cdefs makeriff COMPILE_DEFINITIONS)
message("cdefs: ${cdefs}")

target_include_directories(makeriff PUBLIC
        ../../../tools
        ../../windward/wind22/include/)


set_property(TARGET makeriff PROPERTY CXX_STANDARD 11)

