# Headless tests. Registered by the top-level CMakeLists when BUILD_TESTING is
# on. These add no build targets: the cmake/ checks run everywhere, and the
# data/net suites compile shipped production bodies with the host compiler
# outside the game build (Windows only; they drive cl.exe directly).

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
