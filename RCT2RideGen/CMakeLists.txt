add_executable(makevehicle
    src/main.cpp
    src/project.cpp
)
target_link_libraries(makevehicle PRIVATE
    IsoRender
    jansson::jansson
    libzip::zip
)

# Mirror the examples directory next to the built binary.
add_custom_command(TARGET makevehicle POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/examples"
        "$<TARGET_FILE_DIR:makevehicle>/examples"
    VERBATIM
)
