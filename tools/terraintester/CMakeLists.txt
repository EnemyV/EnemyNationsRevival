###############################
# terraintester: standalone terrain render tool (does not start the game).
# Renders tools/terraintester/maps/*.txt with the same enterrain library as enations.
###############################

add_executable(terraintester main.cpp)
target_link_libraries(terraintester PRIVATE enterrain)
# Default tile set: the repo's baked PNGs (override with --data / EN_TERRAIN_DATA).
target_compile_definitions(terraintester PRIVATE
    ENTERRAIN_DEFAULT_DATA="${CMAKE_SOURCE_DIR}/enations_latest/data/terrain_gpu")

if (MSVC)
    # Console app with its own main() (SDL_MAIN_HANDLED); same CRT as enterrain.
    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Ob0" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Od" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    target_compile_options(terraintester PRIVATE
        $<$<CONFIG:Sanitize>:/MDd /EHsc /Od /Ob0 /fsanitize=address>
        $<$<CONFIG:Debug>:/MDd /EHsc /O2 /Ob2>
        $<$<CONFIG:Profile>:/MD /EHsc /O2>
        $<$<CONFIG:Release>:/MD /EHsc /O2>
    )
    target_compile_definitions(terraintester PRIVATE
        _CRT_SECURE_NO_WARNINGS
        $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>)
    set_property(TARGET terraintester PROPERTY CXX_STANDARD 11)
    target_link_options(terraintester PRIVATE $<$<CONFIG:Release>:/LTCG>)   # enterrain Release is /GL
    if (CMAKE_GENERATOR_PLATFORM STREQUAL "x64" OR CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(TT_SDL2_ARCH x64)
    else ()
        set(TT_SDL2_ARCH x86)
    endif ()
    add_custom_command(TARGET terraintester POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/tools/sdl2/SDL2-2.30.12/lib/${TT_SDL2_ARCH}/SDL2.dll"
            $<TARGET_FILE_DIR:terraintester>
        COMMENT "Copying SDL2.dll next to terraintester")
else ()
    target_compile_options(terraintester PRIVATE -g)
    set_property(TARGET terraintester PROPERTY CXX_STANDARD 17)
endif ()
