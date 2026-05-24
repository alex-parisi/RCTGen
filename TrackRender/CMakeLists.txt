# Sources shared between maketrack and subposition; compiled once.
add_library(TrackCommon OBJECT
    src/sprites.cpp
    src/track_sections.cpp
)
target_include_directories(TrackCommon PUBLIC src)
target_link_libraries(TrackCommon PUBLIC IsoRender)

add_executable(maketrack
    src/main.cpp
    src/track.cpp
    src/mask.cpp
)
target_link_libraries(maketrack PRIVATE
    TrackCommon
    jansson::jansson
    libzip::zip
)

add_executable(subposition src/subposition.cpp)
target_link_libraries(subposition PRIVATE
    TrackCommon
    jansson::jansson
    libzip::zip
)
