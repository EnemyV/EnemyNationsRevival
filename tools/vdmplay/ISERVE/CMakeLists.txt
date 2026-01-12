cmake_minimum_required(VERSION 3.10)
project(iserve LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define source files
set(iserve_sources
    STDAFX.CPP
    ISERVE.CPP
    ISERVDLG.CPP
    ISERVE.RC
)

# Add executable
add_executable(iserve WIN32 ${iserve_sources})

# Include directories
target_include_directories(iserve PUBLIC
    .
    ..
)

# Find MFC
find_package(MFC REQUIRED)

# Link libraries
target_link_libraries(iserve PRIVATE
    ${CMAKE_SOURCE_DIR}/cmakeBuild/tools/vdmplay/$<CONFIG>/vdmplay.lib
    wsock32.lib
)

# Define preprocessor definitions
target_compile_definitions(iserve PRIVATE
    WIN32
    _WINDOWS
    _MBCS
    _AFXDLL
)

# Configuration-specific settings
target_compile_definitions(iserve PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

target_compile_options(iserve PRIVATE
    $<$<CONFIG:Debug>:/MD /W3 /GX /Zi /Od>
    $<$<CONFIG:Release>:/MD /W3 /GX /O2>
)

target_link_options(iserve PRIVATE
    $<$<CONFIG:Debug>:/SUBSYSTEM:WINDOWS /DEBUG /MACHINE:I386 /INCREMENTAL:YES>
    $<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS /INCREMENTAL:NO>
)

set_target_properties(iserve PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/WinDebug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/WinRel"
    PDB_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/WinDebug"
    PDB_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/WinRel"
)

# Precompiled headers
target_precompile_headers(iserve PRIVATE stdafx.h)

# Resource compilation
set_source_files_properties(ISERVE.RC PROPERTIES
    COMPILE_FLAGS "/l 0x409"  # Language ID for English
)

# Generate .bsc file (source browser information)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(bsc_files
        ${CMAKE_BINARY_DIR}/WinDebug/STDAFX.sbr
        ${CMAKE_BINARY_DIR}/WinDebug/ISERVE.sbr
        ${CMAKE_BINARY_DIR}/WinDebug/ISERVDLG.sbr
    )
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/WinDebug/ISERVE.bsc
        COMMAND bscmake /nologo /o${CMAKE_BINARY_DIR}/WinDebug/ISERVE.bsc ${bsc_files}
        DEPENDS ${iserve_sources}
        COMMENT "Generating ISERVE.bsc for Debug"
    )
    add_custom_target(iserve_bsc ALL DEPENDS ${CMAKE_BINARY_DIR}/WinDebug/ISERVE.bsc)
else()
    set(bsc_files
        ${CMAKE_BINARY_DIR}/WinRel/STDAFX.sbr
        ${CMAKE_BINARY_DIR}/WinRel/ISERVE.sbr
        ${CMAKE_BINARY_DIR}/WinRel/ISERVDLG.sbr
    )
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/WinRel/ISERVE.bsc
        COMMAND bscmake /nologo /o${CMAKE_BINARY_DIR}/WinRel/ISERVE.bsc ${bsc_files}
        DEPENDS ${iserve_sources}
        COMMENT "Generating ISERVE.bsc for Release"
    )
    add_custom_target(iserve_bsc ALL DEPENDS ${CMAKE_BINARY_DIR}/WinRel/ISERVE.bsc)
endif()

# Create output directories
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/WinDebug)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/WinRel)