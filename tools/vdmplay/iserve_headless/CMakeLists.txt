# iserve_headless (MSVC) — Windows headless VDMPLAY registration server.
#
# The Windows half of the cross-platform headless ISERVE (the POSIX half is the
# `iserve` target wired in enations_latest/src/CMakeLists.txt's if(NOT MSVC) block).
# This target is added only from the root if(MSVC) block, AFTER add_subdirectory(
# tools/vdmplay) so the `vdmplay` engine target exists to link against.
#
# Console app (no WIN32/GUI subsystem), NO MFC — unlike the frozen legacy 32-bit MFC
# GUI in tools/vdmplay/ISERVE/. Reuses linux1's iserve_core.cpp (portable via #ifdef
# _WIN32) + a Windows-native iserve_main_win.cpp (Win32 message-loop pump, since
# vpPumpNet is POSIX-only). Built for the generator's arch (x64) — NOT /MACHINE:I386.

project(iserve_headless LANGUAGES CXX)

add_executable(iserve_headless
    iserve_core.cpp
    iserve_main_win.cpp)

set_target_properties(iserve_headless PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON)

target_include_directories(iserve_headless PRIVATE
    .                                       # iserve_core.h
    ..                                      # vdmplay.h, base.h (tools/vdmplay)
    ${CMAKE_SOURCE_DIR}/windward/wind22/include)

target_compile_definitions(iserve_headless PRIVATE
    WIN32 _CONSOLE _CRT_SECURE_NO_WARNINGS)

# Link the Windows VDMPLAY engine (the vp* registration API) + winsock. vdmplay is a
# SHARED target, so iserve_headless.exe needs vdmplay.dll co-located at runtime.
target_link_libraries(iserve_headless PRIVATE vdmplay ws2_32)
