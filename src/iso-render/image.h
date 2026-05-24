#pragma once

#include <cstdint>
#include <cstdio>
#include <png.h>

struct image_palette_t
{
    int num_colors;
    int transparent_color;
    png_color colors[256];
};

struct image_t
{
    std::uint16_t width;
    std::uint16_t height;
    std::int16_t x_offset;
    std::int16_t y_offset;
    std::uint8_t* pixels;
};

void image_new(image_t* image, std::uint16_t width, std::uint16_t height, std::int16_t x_offset, std::int16_t y_offset, std::uint16_t flags);
void image_copy(image_t* src, image_t* dst);
void image_blit(image_t* dst, image_t* src, std::int16_t x_offset, std::int16_t y_offset);
int image_read_png(image_t* image, std::FILE* file);
int image_write_png(image_t* image, image_palette_t* palette, std::FILE* file);
void image_create_atlas(image_t* image, image_t* images, int num_images, int* x_offsets, int* y_offsets);
void image_create_grid(image_t* image, image_t* images, int num_images, int* x_offsets, int* y_offsets, int columns);
void image_crop(image_t* image);
void image_destroy(image_t* image);
