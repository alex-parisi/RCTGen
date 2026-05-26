add_library(IsoRender STATIC
        VectorMath.cpp
        Renderer.cpp
        Mesh.cpp
        Palette.cpp
        RayTrace.cpp
        Image.cpp
        Pack.cpp
)

target_include_directories(IsoRender PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Public deps leak through IsoRender's own headers: RayTrace.hpp pulls embree,
# Image.hpp pulls libpng. Everything else is an implementation detail.
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
