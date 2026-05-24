add_library(IsoRender STATIC
        vectormath.cpp
        renderer.cpp
        model.cpp
        palette.cpp
        raytrace.cpp
        image.cpp
        pack.cpp
)

target_include_directories(IsoRender PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Public deps leak through IsoRender's own headers: raytrace.h pulls embree,
# image.h pulls libpng. Everything else is an implementation detail.
target_link_libraries(IsoRender
    PUBLIC
        rctgen_common_flags
        PNG::PNG
        embree
    PRIVATE
        ZLIB::ZLIB
        assimp::assimp
)

add_executable(TestIsoRender main.cpp)
target_link_libraries(TestIsoRender PRIVATE IsoRender jansson::jansson)
