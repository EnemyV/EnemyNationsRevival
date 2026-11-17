###############################
# enterrain (static lib): the GPU terrain core shared by enations and terraintester.
# No game headers; depends only on SDL2 (+ the bundled stb_image).
###############################

add_library(enterrain STATIC
    EnTerrain.h
    EnTerrainTiles.cpp
    EnTerrainRender.cpp
    EnTerrainLayers.h
    EnTerrainLayers.cpp
    EnTerrainShore.h
    EnTerrainShore.cpp
    thirdparty/stb_image.h
)

target_include_directories(enterrain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (MSVC)
    # Match the game's per-config CRT / codegen exactly (same flags as wind22, which
    # also links into enations): a CRT or _ITERATOR_DEBUG_LEVEL mismatch fails the
    # link (LNK2038), and the float flags keep the shared shade math identical to
    # what SDL2Terrain.cpp computed in-TU before the move. /RTC1 rides in on CMake's
    # default debug flags and conflicts with /O2.
    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Ob0" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Od" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    target_compile_options(enterrain PRIVATE
        $<$<CONFIG:Sanitize>:/MDd /EHsc /Od /Ob0 /fsanitize=address>
        $<$<CONFIG:Debug>:/MDd /EHsc /O2 /Ob2>
        $<$<CONFIG:Profile>:/MD /EHsc /O2 /Ob2 /fp:fast>
        $<$<CONFIG:Release>:/MD /EHsc /O2 /Ob2 /Oi /GL /Gy /Gw /Ot /GF /fp:fast /Zi>
    )
    target_compile_definitions(enterrain PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
        $<$<CONFIG:Release>:NDEBUG>
        $<$<CONFIG:Profile>:NDEBUG>
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Sanitize>:_DEBUG>
    )
    target_include_directories(enterrain PUBLIC
        ${CMAKE_SOURCE_DIR}/tools/sdl2/SDL2-2.30.12/include)
    if (CMAKE_GENERATOR_PLATFORM STREQUAL "x64" OR CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ENTERRAIN_SDL2_ARCH x64)
    else ()
        set(ENTERRAIN_SDL2_ARCH x86)
    endif ()
    target_link_directories(enterrain PUBLIC
        ${CMAKE_SOURCE_DIR}/tools/sdl2/SDL2-2.30.12/lib/${ENTERRAIN_SDL2_ARCH})
    target_link_libraries(enterrain PUBLIC SDL2.lib)
    set_property(TARGET enterrain PROPERTY CXX_STANDARD 11)
else ()
    # -ffp-contract=off: same float semantics as the enations target (no FMA fusing),
    # so the moved shade/feather float math is bit-identical to the in-game TU.
    target_compile_options(enterrain PRIVATE
        -g -ffp-contract=off
        $<$<CONFIG:Release>:-O2>
        $<$<CONFIG:Profile>:-O2>
    )
    target_link_libraries(enterrain PUBLIC EnLinuxSDL2)
    set_property(TARGET enterrain PROPERTY CXX_STANDARD 17)
endif ()
