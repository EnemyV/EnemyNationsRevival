project(RIFConverter CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# RIF Converter executable
add_executable(rif_converter
    main.cpp
    rif_converter.cpp
    rif_converter.h
)

# Include directories
target_include_directories(rif_converter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Compiler flags
if(MSVC)
    target_compile_options(rif_converter PRIVATE /W4 /WX)
else()
    target_compile_options(rif_converter PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Output directory
set_target_properties(rif_converter PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
