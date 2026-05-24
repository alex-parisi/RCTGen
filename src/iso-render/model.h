#pragma once

#include <cstddef>
#include <cstdint>
#include "color.h"
#include "vectormath.h"

struct texture_t
{
    std::uint16_t width;
    std::uint16_t height;
    vector3_t* pixels;
};

// Material flag bits. Kept as named constants (rather than enum class) because
// they are combined with bitwise OR throughout the codebase.
inline constexpr std::uint16_t MATERIAL_HAS_TEXTURE       = 1u   << 0;
inline constexpr std::uint16_t MATERIAL_IS_REMAPPABLE     = 1u   << 1;
inline constexpr std::uint16_t MATERIAL_IS_MASK           = 1u   << 2;
inline constexpr std::uint16_t MATERIAL_NO_AO             = 1u   << 3;
inline constexpr std::uint16_t MATERIAL_BACKGROUND_AA     = 1u   << 4;
inline constexpr std::uint16_t MATERIAL_BACKGROUND_AA_DARK = 1u  << 5;
inline constexpr std::uint16_t MATERIAL_IS_VISIBLE_MASK   = 1u   << 6;
inline constexpr std::uint16_t MATERIAL_NO_BLEED          = 1u   << 7;
inline constexpr std::uint16_t MATERIAL_IS_FLAT_SHADED    = 1u   << 8;

struct material_t
{
    std::uint16_t flags;
    std::uint8_t region;
    float specular_exponent;
    vector3_t specular_color;
    vector3_t ambient_color;
    union
    {
        texture_t texture;
        vector3_t color;
    };
};

struct face_t
{
    std::size_t material;
    std::size_t indices[3];
};

struct mesh_t
{
    vector3_t* vertices;
    vector3_t* normals;
    vector2_t* uvs;
    face_t* faces;
    material_t* materials;
    std::size_t num_vertices;
    std::size_t num_faces;
    std::size_t num_materials;
};

void texture_init(texture_t* texture, std::uint16_t width, std::uint16_t height);
int texture_load_png(texture_t* texture, const char* filename);
vector3_t texture_sample(texture_t* texture, vector2_t coord);
void texture_destroy(texture_t* texture);

material_t material_color(vector3_t color, vector3_t specular_color, float specular_exponent, std::uint8_t flags);
material_t material_texture(const char* filename, vector3_t specular_color, float specular_exponent, std::uint8_t flags);

int mesh_load(mesh_t* mesh, const char* filename);
int mesh_load_transform(mesh_t* output, const char* filename, matrix_t matrix);
void mesh_destroy(mesh_t* mesh);
