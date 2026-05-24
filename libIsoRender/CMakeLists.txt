add_library(IsoRender STATIC
    src/vectormath.cpp
    src/renderer.cpp
    src/model.cpp
    src/palette.cpp
    src/raytrace.cpp
    src/image.cpp
    src/pack.cpp
)

target_include_directories(IsoRender PUBLIC src)

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

add_executable(TestIsoRender src/main.cpp)
target_link_libraries(TestIsoRender PRIVATE IsoRender jansson::jansson)
