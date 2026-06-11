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

# Debug builds: DISABLE MSVC checked iterators (_ITERATOR_DEBUG_LEVEL=2 is the /MDd
# default). Every checked-iterator construct/copy/destroy — including const find(),
# vector push_back range-orphaning, any begin()/end() — takes std::_Lockit, ONE GLOBAL
# critical section for the entire process. With 12 AI threads + the main thread all
# running STL-heavy code, stack-sampling the 13-player save showed threads parked on
# that lock from independent directions (CMap::Lookup in AssertValid sweeps, sprite-
# capture vector push_backs, the dialog-list end() in PollEvents) — a lock convoy that
# took the game to ~1-4 fps. IDL=0 removes the proxies and the lock entirely; the CRT
# debug heap and game-side ASSERT/en_assert diagnostics remain. Set GLOBALLY (before
# all add_subdirectory) because IDL participates in the C++ ABI: mixed-IDL TUs fail to
# link (LNK2038), so every target must agree. The mfc_compat CMapSrw guard keys its
# read mode on IDL and automatically degrades to cheap SHARED reads at IDL=0.
add_compile_definitions($<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>)

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
