add_library(RctgenUtil STATIC
        Json.cpp
        Logging.cpp
)
target_include_directories(RctgenUtil PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# IsoRender is PUBLIC because Json.hpp exposes vectormath.h (vector3_t); jansson
# is PUBLIC because Json.hpp transitively includes <jansson.h>.
target_link_libraries(RctgenUtil PUBLIC
        rctgen_common_flags
        IsoRender
        jansson::jansson
)
