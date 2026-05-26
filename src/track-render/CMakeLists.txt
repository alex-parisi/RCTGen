# Sources shared between maketrack and subposition; compiled once.
add_library(TrackCommon OBJECT
        Json.cpp
        Logging.cpp
        Sprites.cpp
        TrackSections.cpp
)
target_include_directories(TrackCommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(TrackCommon PUBLIC
        rctgen_common_flags
        IsoRender
        jansson::jansson
)

add_executable(maketrack
        main.cpp
        Mask.cpp
        Track.cpp
)
target_link_libraries(maketrack PRIVATE
        TrackCommon
        libzip::zip
)

add_executable(subposition Subposition.cpp)
target_link_libraries(subposition PRIVATE
        TrackCommon
        libzip::zip
)
