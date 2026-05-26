/// Image.hpp

#pragma once

#include <cstdint>
#include <cstdio>

#include <png.h>

namespace RCTGen {
    struct ImagePalette {
        int num_colors;
        int transparent_color;
        png_color colors[256];
    };

    struct Image {
        std::uint16_t width;
        std::uint16_t height;
        std::int16_t x_offset;
        std::int16_t y_offset;
        std::uint8_t *pixels;
    };

    void image_new(Image *image, std::uint16_t width, std::uint16_t height,
                   std::int16_t x_offset, std::int16_t y_offset, std::uint16_t flags);

    void image_copy(Image *src, Image *dst);

    void image_blit(Image *dst, Image *src, std::int16_t x_offset, std::int16_t y_offset);

    int image_read_png(Image * image, std::FILE * file);
    int image_write_png(Image * image, ImagePalette * palette, std::FILE * file);

    void image_create_atlas(Image *image, Image *images, int num_images, int *x_offsets, int *y_offsets);

    void image_create_grid(Image *image, Image *images, int num_images, int *x_offsets, int *y_offsets, int columns);

    void image_crop(Image *image);

    void image_destroy(Image *image);
}