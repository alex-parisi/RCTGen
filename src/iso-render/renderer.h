#pragma once

#include <array>
#include <cstdint>
#include "image.h"
#include "model.h"
#include "palette.h"
#include "raytrace.h"
#include "vectormath.h"

inline constexpr int RENDER_WIDTH    = 255;
inline constexpr int RENDER_HEIGHT   = 256;
inline constexpr int RENDER_PIXELS   = RENDER_WIDTH * RENDER_HEIGHT;
inline constexpr int UNITS_PER_TILE  = 4096;
inline constexpr int UNITS_PER_PIXEL = 128;
inline constexpr std::uint8_t FRAGMENT_UNUSED = 255;
inline constexpr std::uint8_t REGION_MASK     = 0x7;
inline constexpr std::size_t  MAX_REGIONS     = 8;

struct rect_t
{
    std::int32_t x_lower;
    std::int32_t y_lower;
    std::int32_t x_upper;
    std::int32_t y_upper;
};

struct fragment_t
{
    vector3_t color;
    float depth;
    float ghost_depth;
    std::uint8_t flags;
    std::uint8_t region;
};

enum light_type : std::uint16_t
{
    LIGHT_HEMI,
    LIGHT_DIFFUSE,
    LIGHT_SPECULAR,
};

struct light_t
{
    std::uint16_t type;
    std::uint16_t shadow;
    vector3_t direction;
    float intensity;
};

struct framebuffer_t
{
    std::uint16_t width;
    std::uint16_t height;
    vector2_t offset;
    fragment_t* fragments;
};

struct context_t
{
    std::uint32_t num_lights;
    std::uint32_t dither;
    light_t* lights;
    matrix_t projection;
    device_t rt_device;
    scene_t rt_scene;
    palette_t palette;
};

extern std::array<matrix_t, 4> views;

void context_init(context_t* context, light_t* lights, std::uint32_t num_lights, std::uint32_t dither, palette_t palette, float upt);
void context_destroy(context_t* context);
void context_begin_render(context_t* context);
void context_add_model(context_t* context, mesh_t* mesh, transform_t transform, int mask);
void context_add_model_transformed(context_t* context, mesh_t* mesh, vertex_t (*transform)(vector3_t, vector3_t, bool, void*), void* data, int mask);
void context_render_view(context_t* context, matrix_t view_matrix, image_t* image);
void context_render_silhouette(context_t* context, matrix_t view, image_t* image);
void context_finalize_render(context_t* context);
void context_end_render(context_t* context);
