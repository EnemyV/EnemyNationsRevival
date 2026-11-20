# Build guard (runs on every build of `enations`): macOS must always start fullscreen
# with the Dock and menu bar hidden. A leaked EN_WINDOWED / EN_FULLSCREEN /
# EN_MAC_USABLE_FULLSCREEN override has shipped or been mistaken for that release-blocking
# regression many times, so the Mac build no longer reads them. This fails the build if
# such a read comes back. See the always-fullscreen note in GameWindow.cpp.
# usage: cmake -DSRC=<enations_latest/src> -P tools/mac-fullscreen-guard.cmake
file(GLOB _srcs "${SRC}/*.cpp" "${SRC}/*.h")
set(_windowed_reads 0)
foreach(_f ${_srcs})
    file(READ "${_f}" _t)
    get_filename_component(_n "${_f}" NAME)
    if(_t MATCHES "getenv *\\( *\"EN_(MAC_USABLE_FULLSCREEN|FULLSCREEN)\"")
        message(FATAL_ERROR "mac-fullscreen-guard: ${_n} reads EN_${CMAKE_MATCH_1}. macOS must always be fullscreen; do not add display overrides back.")
    endif()
    string(REGEX MATCHALL "getenv *\\( *\"EN_WINDOWED\"" _m "${_t}")
    list(LENGTH _m _c)
    math(EXPR _windowed_reads "${_windowed_reads} + ${_c}")
    if(_c GREATER 0 AND NOT _n STREQUAL "GameWindow.cpp")
        message(FATAL_ERROR "mac-fullscreen-guard: ${_n} reads EN_WINDOWED. Only the Linux branch in GameWindow.cpp may.")
    endif()
endforeach()
file(READ "${SRC}/GameWindow.cpp" _gw)
string(FIND "${_gw}" "#ifdef __APPLE__\n    for (const char* v : { \"EN_WINDOWED\", \"EN_FULLSCREEN\", \"EN_MAC_USABLE_FULLSCREEN\" })" _a)
string(FIND "${_gw}" "    const bool wantFullscreen = true;\n#else\n    // Linux keeps the EN_WINDOWED=1 developer opt-in" _b)
if(_a LESS 0 OR _b LESS 0 OR _windowed_reads GREATER 1)
    message(FATAL_ERROR "mac-fullscreen-guard: GameWindow.cpp no longer forces macOS fullscreen (expected the Mac branch to ignore EN_WINDOWED and set wantFullscreen = true, with EN_WINDOWED read only in the Linux branch).")
endif()
