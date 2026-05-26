add_library(rct2_ride_gen_lib STATIC
        ProjectLoader.cpp
        SpriteRenderer.cpp
        ProjectExporter.cpp
)
target_include_directories(rct2_ride_gen_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(rct2_ride_gen_lib PUBLIC
        rctgen_common_flags
        RctgenUtil
        # libzip must come before IsoRender on the link line: assimp (a transitive
        # dep of IsoRender) ships its own zip_open / zip_close symbols from a
        # bundled kuba--/zip library with an incompatible signature. With assimp
        # earlier in the link order, the macOS two-level-namespace linker binds
        # our zip_open calls to assimp's stub, which silently returns nullptr.
        libzip::zip
        IsoRender
        jansson::jansson
)

add_executable(makevehicle main.cpp)
target_link_libraries(makevehicle PRIVATE rct2_ride_gen_lib)

add_custom_command(TARGET makevehicle POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/examples"
        "$<TARGET_FILE_DIR:makevehicle>/examples"
        VERBATIM
)
