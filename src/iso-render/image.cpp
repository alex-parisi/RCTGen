#include "image.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <png.h>
#include <vector>

namespace
{

image_palette_t rct2_palette{256, 0, {
    {0, 0, 0}, {1, 1, 1}, {2, 2, 2}, {3, 3, 3}, {4, 4, 4},
    {5, 5, 5}, {6, 6, 6}, {7, 7, 7}, {8, 8, 8}, {9, 9, 9},
    {23, 35, 35}, {35, 51, 51}, {47, 67, 67}, {63, 83, 83}, {75, 99, 99},
    {91, 115, 115}, {111, 131, 131}, {131, 151, 151}, {159, 175, 175}, {183, 195, 195},
    {211, 219, 219}, {239, 243, 243}, {51, 47, 0}, {63, 59, 0}, {79, 75, 11},
    {91, 91, 19}, {107, 107, 31}, {119, 123, 47}, {135, 139, 59}, {151, 155, 79},
    {167, 175, 95}, {187, 191, 115}, {203, 207, 139}, {223, 227, 163}, {67, 43, 7},
    {87, 59, 11}, {111, 75, 23}, {127, 87, 31}, {143, 99, 39}, {159, 115, 51},
    {179, 131, 67}, {191, 151, 87}, {203, 175, 111}, {219, 199, 135}, {231, 219, 163},
    {247, 239, 195}, {71, 27, 0}, {95, 43, 0}, {119, 63, 0}, {143, 83, 7},
    {167, 111, 7}, {191, 139, 15}, {215, 167, 19}, {243, 203, 27}, {255, 231, 47},
    {255, 243, 95}, {255, 251, 143}, {255, 255, 195}, {35, 0, 0}, {79, 0, 0},
    {95, 7, 7}, {111, 15, 15}, {127, 27, 27}, {143, 39, 39}, {163, 59, 59},
    {179, 79, 79}, {199, 103, 103}, {215, 127, 127}, {235, 159, 159}, {255, 191, 191},
    {27, 51, 19}, {35, 63, 23}, {47, 79, 31}, {59, 95, 39}, {71, 111, 43},
    {87, 127, 51}, {99, 143, 59}, {115, 155, 67}, {131, 171, 75}, {147, 187, 83},
    {163, 203, 95}, {183, 219, 103}, {31, 55, 27}, {47, 71, 35}, {59, 83, 43},
    {75, 99, 55}, {91, 111, 67}, {111, 135, 79}, {135, 159, 95}, {159, 183, 111},
    {183, 207, 127}, {195, 219, 147}, {207, 231, 167}, {223, 247, 191}, {15, 63, 0},
    {19, 83, 0}, {23, 103, 0}, {31, 123, 0}, {39, 143, 7}, {55, 159, 23},
    {71, 175, 39}, {91, 191, 63}, {111, 207, 87}, {139, 223, 115}, {163, 239, 143},
    {195, 255, 179}, {79, 43, 19}, {99, 55, 27}, {119, 71, 43}, {139, 87, 59},
    {167, 99, 67}, {187, 115, 83}, {207, 131, 99}, {215, 151, 115}, {227, 171, 131},
    {239, 191, 151}, {247, 207, 171}, {255, 227, 195}, {15, 19, 55}, {39, 43, 87},
    {51, 55, 103}, {63, 67, 119}, {83, 83, 139}, {99, 99, 155}, {119, 119, 175},
    {139, 139, 191}, {159, 159, 207}, {183, 183, 223}, {211, 211, 239}, {239, 239, 255},
    {0, 27, 111}, {0, 39, 151}, {7, 51, 167}, {15, 67, 187}, {27, 83, 203},
    {43, 103, 223}, {67, 135, 227}, {91, 163, 231}, {119, 187, 239}, {143, 211, 243},
    {175, 231, 251}, {215, 247, 255}, {11, 43, 15}, {15, 55, 23}, {23, 71, 31},
    {35, 83, 43}, {47, 99, 59}, {59, 115, 75}, {79, 135, 95}, {99, 155, 119},
    {123, 175, 139}, {147, 199, 167}, {175, 219, 195}, {207, 243, 223}, {63, 0, 95},
    {75, 7, 115}, {83, 15, 127}, {95, 31, 143}, {107, 43, 155}, {123, 63, 171},
    {135, 83, 187}, {155, 103, 199}, {171, 127, 215}, {191, 155, 231}, {215, 195, 243},
    {243, 235, 255}, {63, 0, 0}, {87, 0, 0}, {115, 0, 0}, {143, 0, 0},
    {171, 0, 0}, {199, 0, 0}, {227, 7, 0}, {255, 7, 0}, {255, 79, 67},
    {255, 123, 115}, {255, 171, 163}, {255, 219, 215}, {79, 39, 0}, {111, 51, 0},
    {147, 63, 0}, {183, 71, 0}, {219, 79, 0}, {255, 83, 0}, {255, 111, 23},
    {255, 139, 51}, {255, 163, 79}, {255, 183, 107}, {255, 203, 135}, {255, 219, 163},
    {0, 51, 47}, {0, 63, 55}, {0, 75, 67}, {0, 87, 79}, {7, 107, 99},
    {23, 127, 119}, {43, 147, 143}, {71, 167, 163}, {99, 187, 187}, {131, 207, 207},
    {171, 231, 231}, {207, 255, 255}, {63, 0, 27}, {103, 0, 51}, {123, 11, 63},
    {143, 23, 79}, {163, 31, 95}, {183, 39, 111}, {219, 59, 143}, {239, 91, 171},
    {243, 119, 187}, {247, 151, 203}, {251, 183, 223}, {255, 215, 239}, {39, 19, 0},
    {55, 31, 7}, {71, 47, 15}, {91, 63, 31}, {107, 83, 51}, {123, 103, 75},
    {143, 127, 107}, {163, 147, 127}, {187, 171, 147}, {207, 195, 171}, {231, 219, 195},
    {255, 243, 223}, {55, 75, 75}, {255, 183, 0}, {255, 219, 0}, {255, 255, 0},
    {39, 143, 135}, {7, 107, 99}, {7, 107, 99}, {7, 107, 99}, {27, 131, 123},
    {155, 227, 227}, {55, 155, 151}, {55, 155, 151}, {55, 155, 151}, {115, 203, 203},
    {67, 91, 91}, {83, 107, 107}, {99, 123, 123}, {111, 51, 47}, {131, 55, 47},
    {151, 63, 51}, {171, 67, 51}, {191, 75, 47}, {211, 79, 43}, {231, 87, 35},
    {255, 95, 31}, {255, 127, 39}, {255, 155, 51}, {255, 183, 63}, {255, 207, 75},
}};



} // namespace

void image_new(image_t* image, std::uint16_t width, std::uint16_t height, std::int16_t x_offset, std::int16_t y_offset, std::uint16_t /*flags*/)
{
    image->width = width;
    image->height = height;
    image->x_offset = x_offset;
    image->y_offset = y_offset;
    image->pixels = static_cast<std::uint8_t*>(std::calloc(static_cast<std::size_t>(width) * height, sizeof(std::uint8_t)));
}

void image_copy(image_t* src, image_t* dst)
{
    dst->width = src->width;
    dst->height = src->height;
    dst->x_offset = src->x_offset;
    dst->y_offset = src->y_offset;
    const std::size_t bytes = static_cast<std::size_t>(src->width) * src->height;
    dst->pixels = static_cast<std::uint8_t*>(std::calloc(bytes, sizeof(std::uint8_t)));
    std::memmove(dst->pixels, src->pixels, bytes);
}

// TODO prevent writing outside image
void image_blit(image_t* dst, image_t* src, std::int16_t x_offset, std::int16_t y_offset)
{
    x_offset += src->x_offset - dst->x_offset;
    y_offset += src->y_offset - dst->y_offset;

    for (int y = 0; y < src->height; ++y)
        for (int x = 0; x < src->width; ++x)
        {
            const int dst_x = x_offset + x;
            const int dst_y = y_offset + y;

            if (src->pixels[y * src->width + x] && dst_x >= 0 && dst_y >= 0 && dst_x < dst->width && dst_y < dst->height)
                dst->pixels[dst_y * dst->width + dst_x] = src->pixels[y * src->width + x];
        }
}

int image_read_png(image_t* image, std::FILE* file)
{
    if (!file) return 1;

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) { std::fclose(file); return 1; }

    png_infop info = png_create_info_struct(png);
    if (!info) { std::fclose(file); return 1; }

    if (setjmp(png_jmpbuf(png))) return 1;

    png_init_io(png, file);
    png_read_info(png, info);

    image->width = static_cast<std::uint16_t>(png_get_image_width(png, info));
    image->height = static_cast<std::uint16_t>(png_get_image_height(png, info));
    image->x_offset = 0;
    image->y_offset = 0;

    const png_byte color_type = png_get_color_type(png, info);

    if (color_type != PNG_COLOR_TYPE_PALETTE)
    {
        std::fclose(file);
        return 1;
    }
    png_read_update_info(png, info);

    std::vector<std::vector<png_byte>> row_storage(image->height);
    std::vector<png_bytep> row_pointers(image->height);
    const std::size_t rowbytes = png_get_rowbytes(png, info);
    for (int y = 0; y < image->height; ++y)
    {
        row_storage[y].resize(rowbytes);
        row_pointers[y] = row_storage[y].data();
    }

    png_read_image(png, row_pointers.data());

    image->pixels = static_cast<std::uint8_t*>(std::malloc(static_cast<std::size_t>(image->width) * image->height));

    for (int y = 0; y < image->height; ++y)
        for (int x = 0; x < image->width; ++x)
            image->pixels[x + y * image->width] = static_cast<std::uint8_t>(row_pointers[y][x]);

    return 0;
}

int image_write_png(image_t* image, image_palette_t* palette, std::FILE* file)
{
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (png_ptr == nullptr) return 1;

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == nullptr)
    {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return 1;
    }

    png_set_IHDR(png_ptr, info_ptr, image->width, image->height, 8, PNG_COLOR_TYPE_PALETTE,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    if (palette == nullptr) palette = &rct2_palette;
    png_set_PLTE(png_ptr, info_ptr, palette->colors, palette->num_colors);

    if (palette->transparent_color >= 0)
    {
        png_byte transparency = static_cast<png_byte>(palette->transparent_color);
        png_set_tRNS(png_ptr, info_ptr, &transparency, 1, nullptr);
    }

    std::vector<png_byte*> row_pointers(image->height);
    for (std::size_t y = 0; y < image->height; ++y)
        row_pointers[y] = image->pixels + y * image->width;

    png_init_io(png_ptr, file);
    png_set_rows(png_ptr, info_ptr, row_pointers.data());
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return 0;
}

void image_crop(image_t* image)
{
    int x_min = INT32_MAX;
    int x_max = INT32_MIN;
    int y_min = INT32_MAX;
    int y_max = INT32_MIN;

    for (int y = 0; y < image->height; ++y)
        for (int x = 0; x < image->width; ++x)
        {
            if (image->pixels[x + y * image->width] > 0)
            {
                x_min = std::min(x_min, x);
                x_max = std::max(x_max, x);
                y_min = std::min(y_min, y);
                y_max = std::max(y_max, y);
            }
        }

    if (x_max < x_min)
    {
        image->x_offset = 0;
        image->y_offset = 0;
        image->width = 1;
        image->height = 1;
    }
    else
    {
        const int stride = image->width;
        image->x_offset += static_cast<std::int16_t>(x_min);
        image->y_offset += static_cast<std::int16_t>(y_min);
        image->width = static_cast<std::uint16_t>(x_max - x_min + 1);
        image->height = static_cast<std::uint16_t>(y_max - y_min + 1);

        for (int y = 0; y < image->height; ++y)
            for (int x = 0; x < image->width; ++x)
                image->pixels[x + y * image->width] = image->pixels[(x + x_min) + (y + y_min) * stride];
    }
}

void image_destroy(image_t* image)
{
    std::free(image->pixels);
}
