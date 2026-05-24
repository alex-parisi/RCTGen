#pragma once

#include <array>
#include <cstdint>
#include "color.h"
#include "vectormath.h"

inline constexpr std::size_t NUM_REGIONS = 8;
inline constexpr std::size_t NUM_SUBREGIONS = 4;

struct region_t
{
    std::uint8_t subregions;
    std::array<std::uint8_t, NUM_SUBREGIONS> start_indices;
    std::array<std::uint8_t, NUM_SUBREGIONS> end_indices;
    bool remap;
};

struct palette_t
{
    std::uint8_t transparent_index;
    std::array<region_t, NUM_REGIONS> regions;
    std::array<color_t, 256> colors;
    std::array<color_t, 16> remap_colors;
};

constexpr color_t color(std::uint8_t r, std::uint8_t g, std::uint8_t b) noexcept { return {r, g, b}; }

vector3_t vector_from_color(color_t c);
color_t color_from_vector(vector3_t vec);

// ASSUME: region < MAX_REGION
std::uint8_t palette_get_nearest(palette_t* palette, std::uint8_t region, vector3_t color, vector3_t* error);

palette_t palette_rct2();
