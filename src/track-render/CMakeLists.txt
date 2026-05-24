# Sources shared between maketrack and subposition; compiled once.
add_library(TrackCommon OBJECT
        sprites.cpp
        track_sections.cpp
)
target_include_directories(TrackCommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(TrackCommon PUBLIC IsoRender)

add_executable(maketrack
        main.cpp
        track.cpp
        mask.cpp
)
target_link_libraries(maketrack PRIVATE
    TrackCommon
    jansson::jansson
    libzip::zip
)

add_executable(subposition subposition.cpp)
target_link_libraries(subposition PRIVATE
    TrackCommon
    jansson::jansson
    libzip::zip
)
