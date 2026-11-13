# Headless tests. Registered by the top-level CMakeLists when BUILD_TESTING is
# on. The zoom and listener checks build small headless executables; the
# data/net suites compile production bodies separately on Windows.

add_test(NAME fitsbuffer_coverage
    COMMAND ${CMAKE_COMMAND}
        -DEN_SOURCE_DIR=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_fitsbuffer_coverage.cmake)

add_test(NAME posix_strings_complete
    COMMAND ${CMAKE_COMMAND}
        -DEN_SOURCE_DIR=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_posix_strings.cmake)

if (WIN32)
    add_test(NAME data_suites
        COMMAND powershell -NoProfile -ExecutionPolicy Bypass
            -File ${CMAKE_CURRENT_SOURCE_DIR}/data/run-data-tests.ps1
            -OutDir ${CMAKE_BINARY_DIR}/test-out/datatests)
    add_test(NAME net_suites
        COMMAND powershell -NoProfile -ExecutionPolicy Bypass
            -File ${CMAKE_CURRENT_SOURCE_DIR}/net/run-net-tests.ps1)
    set_tests_properties(data_suites net_suites PROPERTIES TIMEOUT 1200)
endif ()

# These exercise the same interpolation/parser code and loopback listener used
# by the game. The listener stub replaces game callbacks; it is not a gameplay test.
add_executable(zoom_interpolation_checks zoom_interpolation_test.cpp)
target_compile_features(zoom_interpolation_checks PRIVATE cxx_std_11)
add_test(NAME zoom_interpolation COMMAND zoom_interpolation_checks)

find_package(Python3 COMPONENTS Interpreter QUIET)
if (Python3_Interpreter_FOUND)
    find_package(Threads REQUIRED)
    add_executable(p1_listener_stub perf/listener_stub.cpp ../enations_latest/src/PerfHarness.cpp)
    target_compile_features(p1_listener_stub PRIVATE cxx_std_11)
    target_link_libraries(p1_listener_stub PRIVATE Threads::Threads)
    if (WIN32)
        target_link_libraries(p1_listener_stub PRIVATE ws2_32)
    endif ()
    add_test(NAME p1_listener_protocol
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/perf/test_listener.py $<TARGET_FILE:p1_listener_stub>)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/listener-tmp")
    set_tests_properties(p1_listener_protocol PROPERTIES TIMEOUT 30
        ENVIRONMENT "TMP=${CMAKE_CURRENT_BINARY_DIR}/listener-tmp;TEMP=${CMAKE_CURRENT_BINARY_DIR}/listener-tmp;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/listener-tmp")
endif ()

add_executable(dib_surface_checks dib_surface_test.cpp)
target_compile_features(dib_surface_checks PRIVATE cxx_std_11)
if (MSVC)
    target_include_directories(dib_surface_checks PRIVATE "${CMAKE_SOURCE_DIR}/tools/sdl2/SDL2-2.30.12/include")
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(EN_TEST_SDL_ARCH x64)
    else ()
        set(EN_TEST_SDL_ARCH x86)
    endif ()
    set(EN_TEST_SDL_DIR "${CMAKE_SOURCE_DIR}/tools/sdl2/SDL2-2.30.12/lib/${EN_TEST_SDL_ARCH}")
    target_link_libraries(dib_surface_checks PRIVATE "${EN_TEST_SDL_DIR}/SDL2.lib")
    add_custom_command(TARGET dib_surface_checks POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${EN_TEST_SDL_DIR}/SDL2.dll" "$<TARGET_FILE_DIR:dib_surface_checks>")
else ()
    target_link_libraries(dib_surface_checks PRIVATE EnLinuxSDL2)
endif ()
add_test(NAME dib_surface_pixels COMMAND dib_surface_checks)

add_executable(controller_zoom_checks controller_zoom_test.cpp)
target_compile_features(controller_zoom_checks PRIVATE cxx_std_11)
if (MSVC)
    target_include_directories(controller_zoom_checks PRIVATE "${CMAKE_SOURCE_DIR}/tools/sdl2/SDL2-2.30.12/include")
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(EN_CONTROLLER_TEST_SDL_ARCH x64)
    else ()
        set(EN_CONTROLLER_TEST_SDL_ARCH x86)
    endif ()
    set(EN_CONTROLLER_TEST_SDL_DIR "${CMAKE_SOURCE_DIR}/tools/sdl2/SDL2-2.30.12/lib/${EN_CONTROLLER_TEST_SDL_ARCH}")
    target_link_libraries(controller_zoom_checks PRIVATE "${EN_CONTROLLER_TEST_SDL_DIR}/SDL2.lib")
    add_custom_command(TARGET controller_zoom_checks POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${EN_CONTROLLER_TEST_SDL_DIR}/SDL2.dll" "$<TARGET_FILE_DIR:controller_zoom_checks>")
else ()
    target_link_libraries(controller_zoom_checks PRIVATE EnLinuxSDL2)
endif ()
add_test(NAME controller_zoom COMMAND controller_zoom_checks)

add_executable(controller_zoom_focus_checks controller_zoom_focus_test.cpp)
target_compile_features(controller_zoom_focus_checks PRIVATE cxx_std_11)
add_test(NAME controller_zoom_focus COMMAND controller_zoom_focus_checks)
