#include "image.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <numeric>
#include <vector>

namespace
{

struct rect_t
{
    int x;
    int y;
    int width;
    int height;
};

template <typename Project>
auto make_image_comparator(image_t* images, Project proj)
{
    return [images, proj](int a, int b) {
        return proj(images[a]) > proj(images[b]);
    };
}

int pack_rects_fixed_with_comparator(image_t* images, int num_images, int width, int height, int* x_coords, int* y_coords,
                                     auto&& compare)
{
    std::vector<int> permutation(num_images);
    std::iota(permutation.begin(), permutation.end(), 0);
    std::sort(permutation.begin(), permutation.end(), compare);

    std::vector<rect_t> empty_spaces;
    empty_spaces.reserve(10000);
    empty_spaces.push_back({0, 0, width, height});

    int i;
    for (i = 0; i < num_images; ++i)
    {
        image_t* image = images + permutation[i];
        int j;
        int num_created_rects = -1;
        rect_t created_rects[2];
        for (j = static_cast<int>(empty_spaces.size()) - 1; j >= 0; --j)
        {
            const rect_t space = empty_spaces[j];
            x_coords[permutation[i]] = space.x;
            y_coords[permutation[i]] = space.y;
            if (space.width > image->width && space.height > image->height)
            {
                num_created_rects = 2;
                if (space.height - image->height < space.width - image->width)
                {
                    created_rects[0] = {space.x, space.y + image->height, space.width, space.height - image->height};
                    created_rects[1] = {space.x + image->width, space.y, space.width - image->width, image->height};
                }
                else
                {
                    created_rects[0] = {space.x + image->width, space.y, space.width - image->width, space.height};
                    created_rects[1] = {space.x, space.y + image->height, image->width, space.height - image->height};
                }
                break;
            }
            else if (space.width == image->width && space.height > image->height)
            {
                num_created_rects = 1;
                created_rects[0] = {space.x, space.y + image->height, space.width, space.height - image->height};
                break;
            }
            else if (space.height == image->height && space.width > image->width)
            {
                num_created_rects = 1;
                created_rects[0] = {space.x + image->width, space.y, space.width - image->width, space.height};
                break;
            }
            else if (space.width == image->width && space.height == image->height)
            {
                num_created_rects = 0;
                break;
            }
        }
        if (num_created_rects < 0) return 0;

        empty_spaces.erase(empty_spaces.begin() + j);
        for (int k = 0; k < num_created_rects; ++k) empty_spaces.push_back(created_rects[k]);
    }

    return 1;
}

int pack_rects_fixed(image_t* images, int num_images, int width, int height, int* x_coords, int* y_coords)
{
    auto area      = [](const image_t& i) { return static_cast<int>(i.width) * i.height; };
    auto perimeter = [](const image_t& i) { return i.width + i.height; };
    auto max_dim   = [](const image_t& i) { return std::max<int>(i.width, i.height); };
    auto width_p   = [](const image_t& i) { return static_cast<int>(i.width); };
    auto height_p  = [](const image_t& i) { return static_cast<int>(i.height); };

    if (pack_rects_fixed_with_comparator(images, num_images, width, height, x_coords, y_coords, make_image_comparator(images, area))) return 1;
    if (pack_rects_fixed_with_comparator(images, num_images, width, height, x_coords, y_coords, make_image_comparator(images, perimeter))) return 1;
    if (pack_rects_fixed_with_comparator(images, num_images, width, height, x_coords, y_coords, make_image_comparator(images, max_dim))) return 1;
    if (pack_rects_fixed_with_comparator(images, num_images, width, height, x_coords, y_coords, make_image_comparator(images, width_p))) return 1;
    if (pack_rects_fixed_with_comparator(images, num_images, width, height, x_coords, y_coords, make_image_comparator(images, height_p))) return 1;
    return 0;
}

void pack_rects(image_t* images, int num_images, int* width_ptr, int* height_ptr, int* x_coords, int* y_coords)
{
    int size = 256;
    while (!pack_rects_fixed(images, num_images, size, size, x_coords, y_coords)) size *= 2;

    int lower_size = size / 2;
    int upper_size = size;
    while (upper_size - lower_size > 2)
    {
        const int mid_size = (upper_size + lower_size) / 2;
        if (pack_rects_fixed(images, num_images, mid_size, mid_size, x_coords, y_coords)) upper_size = mid_size;
        else lower_size = mid_size;
    }

    int upper_height = upper_size;
    int lower_height = 0;
    while (upper_height - lower_height > 2)
    {
        const int mid_height = (upper_height + lower_height) / 2;
        if (pack_rects_fixed(images, num_images, upper_size, mid_height, x_coords, y_coords)) upper_height = mid_height;
        else lower_height = mid_height;
    }

    int upper_width = upper_size;
    int lower_width = 0;
    while (upper_width - lower_width > 2)
    {
        const int mid_width = (upper_width + lower_width) / 2;
        if (pack_rects_fixed(images, num_images, mid_width, upper_height, x_coords, y_coords)) upper_width = mid_width;
        else lower_width = mid_width;
    }

    int width, height;
    if (upper_width < upper_height)
    {
        width = upper_width;
        height = upper_size;
    }
    else
    {
        width = upper_size;
        height = upper_height;
    }

    assert(pack_rects_fixed(images, num_images, width, height, x_coords, y_coords));
    *width_ptr = width;
    *height_ptr = height;
}

} // namespace

void image_create_atlas(image_t* output, image_t* images, int num_images, int* x_coords, int* y_coords)
{
    int width, height;
    pack_rects(images, num_images, &width, &height, x_coords, y_coords);

    image_new(output, static_cast<std::uint16_t>(width), static_cast<std::uint16_t>(height), 0, 0, 0);
    std::memset(output->pixels, 0, static_cast<std::size_t>(width) * height);
    int used_pixels = 0;
    for (int i = 0; i < num_images; ++i)
    {
        image_blit(output, images + i,
                   static_cast<std::int16_t>(x_coords[i] - images[i].x_offset),
                   static_cast<std::int16_t>(y_coords[i] - images[i].y_offset));
        used_pixels += images[i].width * images[i].height;
    }

    std::printf("Width %d Height %d\n", width, height);
    std::printf("Packing efficiency %.01f%%\n", (100.0 * used_pixels) / (width * height));
}

void image_create_grid(image_t* output, image_t* images, int num_images, int* /*x_coords*/, int* /*y_coords*/, int columns)
{
    int x_min = 0;
    int x_max = 0;
    int y_min = 0;
    int y_max = 0;

    for (int i = 0; i < num_images; ++i)
    {
        x_min = std::min<int>(x_min, images[i].x_offset);
        x_max = std::max<int>(x_max, images[i].width + images[i].x_offset);
        y_min = std::min<int>(y_min, images[i].y_offset);
        y_max = std::max<int>(y_max, images[i].height + images[i].y_offset);
    }

    const int column_width = x_max - x_min;
    const int row_height = y_max - y_min;

    int rows = num_images / columns;
    if (num_images % columns != 0) rows++;

    const int width = column_width * columns;
    const int height = row_height * rows;

    image_new(output, static_cast<std::uint16_t>(width), static_cast<std::uint16_t>(height), 0, 0, 0);
    std::memset(output->pixels, 0, static_cast<std::size_t>(width) * height);

    for (int i = 0; i < num_images; ++i)
    {
        const int row = i / columns;
        const int column = i - row * columns;
        image_blit(output, images + i,
                   static_cast<std::int16_t>(column_width * column - x_min),
                   static_cast<std::int16_t>(row_height * row - y_min));
    }

    std::printf("Width %d Height %d\n", width, height);
}
