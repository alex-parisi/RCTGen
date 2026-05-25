add_library(rct2_ride_gen_lib STATIC
        Logging.cpp
        Json.cpp
        ProjectLoader.cpp
        SpriteRenderer.cpp
        ProjectExporter.cpp
)
target_include_directories(rct2_ride_gen_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(rct2_ride_gen_lib PUBLIC
        rctgen_common_flags
        IsoRender
        jansson::jansson
        libzip::zip
)

add_executable(makevehicle main.cpp)
target_link_libraries(makevehicle PRIVATE rct2_ride_gen_lib)

add_custom_command(TARGET makevehicle POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/examples"
        "$<TARGET_FILE_DIR:makevehicle>/examples"
        VERBATIM
)
