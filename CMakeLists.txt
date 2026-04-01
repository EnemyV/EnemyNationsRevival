cmake_minimum_required(VERSION 3.11...3.26)

if (${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif ()

find_program(CCACHE_PROGRAM ccache
        PATHS D:\\ccache-4.8.1-windows-x86_64)
if (CCACHE_PROGRAM)
    message(STATUS "Found ccache: ${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
else ()
    message(STATUS "Could not find ccache.")
endif ()

# Ensure VS multi-config generator exports these solution configurations
if (CMAKE_GENERATOR MATCHES "Visual Studio")
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Profile;Sanitize" CACHE STRING "Available configurations" FORCE)
endif ()

project(Enations VERSION 1.0
                 DESCRIPTION "Enemy Nations"
                 LANGUAGES CXX)

###############################
# enations.exe
###############################

add_subdirectory(enations_latest/src)

###############################
# rif_converter.exe (SDL2 Phase 0 - Asset Conversion)
###############################

add_subdirectory(tools/rif_converter)

###############################
# makeriff.exe
###############################

add_subdirectory(tools/makeriff)

###############################
# sprite.exe
###############################

add_subdirectory(tools/sprite)

###############################
# wind22.lib
###############################

add_subdirectory(windward/wind22)

###############################
# cdf.exe
###############################

add_subdirectory(src/cdf)

###############################
# compit.exe
###############################

add_subdirectory(tools/compress)

###############################
# vdmplay.dll
###############################

add_subdirectory(tools/vdmplay)

###############################
# ISERVE.exe
###############################

add_subdirectory(tools/vdmplay/iserve)
