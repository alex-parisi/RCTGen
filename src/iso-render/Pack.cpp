#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <numeric>
#include <vector>

#include "Image.hpp"

namespace RCTGen {
    namespace {
        struct PackRect {
            int x;
            int y;
            int width;
            int height;
        };

        // We sort permutation indices with a tri-state qsort_r comparator instead of
        // std::sort + strict-weak predicate. The C-library sort's choice for equal
        // keys differs from libc++'s introsort, and the bin-packer is sensitive to
        // that choice: equal-area sprites swapped between runs would shuffle the
        // atlas layout and break byte-equivalence with historical output. Keeping
        // qsort_r locks in the legacy ordering that the goldens were captured with.
        using qsort_compare_fn = int (*)(const void *, const void *, void *);

#if defined(_WIN32) || defined(_WIN64) || defined(__WINDOWS__)
        struct qsort_thunk_t {
            void *arg;
            qsort_compare_fn cmp;
        };
        inline int qsort_thunk_swap(void *s, const void *a, const void *b) {
            auto *t = static_cast<qsort_thunk_t *>(s);
            return t->cmp(a, b, t->arg);
        }
        inline void qsort_r_compat(void *base, std::size_t n, std::size_t w, qsort_compare_fn cmp, void *arg) {
            qsort_thunk_t t{arg, cmp};
            qsort_s(base, n, w, &qsort_thunk_swap, &t);
        }
#elif defined(__APPLE__) || defined(__FreeBSD__)
        struct qsort_thunk_t {
            void *arg;
            qsort_compare_fn cmp;
        };
        inline int qsort_thunk_swap(void *s, const void *a, const void *b) {
            auto *t = static_cast<qsort_thunk_t *>(s);
            return t->cmp(a, b, t->arg);
        }
        inline void qsort_r_compat(void *base, std::size_t n, std::size_t w, qsort_compare_fn cmp, void *arg) {
            qsort_thunk_t t{arg, cmp};
            qsort_r(base, n, w, &t, &qsort_thunk_swap);
        }
#else // glibc-style: compar last
        inline void qsort_r_compat(void *base, std::size_t n, std::size_t w, qsort_compare_fn cmp, void *arg) {
            qsort_r(base, n, w, cmp, arg);
        }
#endif

        template<typename Project>
        int legacy_compare(const void *a, const void *b, void *arg) {
            auto *images = static_cast<Image *>(arg);
            Project proj{};
            auto ka = proj(images[*static_cast<const int *>(a)]);
            auto kb = proj(images[*static_cast<const int *>(b)]);
            if (ka == kb) return 0;
            return ka < kb ? 1 : -1; // descending
        }

        struct project_area {
            int operator()(const Image &i) const { return static_cast<int>(i.width) * i.height; }
        };

        struct project_perimeter {
            int operator()(const Image &i) const { return i.width + i.height; }
        };

        struct project_max_dim {
            int operator()(const Image &i) const { return std::max<int>(i.width, i.height); }
        };

        struct project_width {
            int operator()(const Image &i) const { return static_cast<int>(i.width); }
        };

        struct project_height {
            int operator()(const Image &i) const { return static_cast<int>(i.height); }
        };

        int pack_rects_fixed_with_comparator(Image *images, int num_images, int width, int height, int *x_coords,
                                             int *y_coords,
                                             qsort_compare_fn compare) {
            std::vector<int> permutation(num_images);
            std::iota(permutation.begin(), permutation.end(), 0);
            qsort_r_compat(permutation.data(), num_images, sizeof(int), compare, images);

            std::vector<PackRect> empty_spaces;
            empty_spaces.reserve(10000);
            empty_spaces.push_back({0, 0, width, height});

            int i;
            for (i = 0; i < num_images; ++i) {
                Image *image = images + permutation[i];
                int j;
                int num_created_rects = -1;
                PackRect created_rects[2];
                for (j = static_cast<int>(empty_spaces.size()) - 1; j >= 0; --j) {
                    const PackRect space = empty_spaces[j];
                    x_coords[permutation[i]] = space.x;
                    y_coords[permutation[i]] = space.y;
                    if (space.width > image->width && space.height > image->height) {
                        num_created_rects = 2;
                        if (space.height - image->height < space.width - image->width) {
                            created_rects[0] = {
                                space.x, space.y + image->height, space.width, space.height - image->height
                            };
                            created_rects[1] = {
                                space.x + image->width, space.y, space.width - image->width, image->height
                            };
                        } else {
                            created_rects[0] = {
                                space.x + image->width, space.y, space.width - image->width, space.height
                            };
                            created_rects[1] = {
                                space.x, space.y + image->height, image->width, space.height - image->height
                            };
                        }
                        break;
                    } else if (space.width == image->width && space.height > image->height) {
                        num_created_rects = 1;
                        created_rects[0] = {
                            space.x, space.y + image->height, space.width, space.height - image->height
                        };
                        break;
                    } else if (space.height == image->height && space.width > image->width) {
                        num_created_rects = 1;
                        created_rects[0] = {space.x + image->width, space.y, space.width - image->width, space.height};
                        break;
                    } else if (space.width == image->width && space.height == image->height) {
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

        int pack_rects_fixed(Image *images, int num_images, int width, int height, int *x_coords, int *y_coords) {
            if (pack_rects_fixed_with_comparator(images, num_images, width, height, x_coords, y_coords,
                                                 &legacy_compare<project_area>))
                return 1;
            if (pack_rects_fixed_with_comparator(images, num_images, width, height, x_coords, y_coords,
                                                 &legacy_compare<project_perimeter>))
                return 1;
            if (pack_rects_fixed_with_comparator(images, num_images, width, height, x_coords, y_coords,
                                                 &legacy_compare<project_max_dim>))
                return 1;
            if (pack_rects_fixed_with_comparator(images, num_images, width, height, x_coords, y_coords,
                                                 &legacy_compare<project_width>))
                return 1;
            if (pack_rects_fixed_with_comparator(images, num_images, width, height, x_coords, y_coords,
                                                 &legacy_compare<project_height>))
                return 1;
            return 0;
        }

        void pack_rects(Image *images, int num_images, int *width_ptr, int *height_ptr, int *x_coords, int *y_coords) {
            int size = 256;
            while (!pack_rects_fixed(images, num_images, size, size, x_coords, y_coords)) size *= 2;

            int lower_size = size / 2;
            int upper_size = size;
            while (upper_size - lower_size > 2) {
                const int mid_size = (upper_size + lower_size) / 2;
                if (pack_rects_fixed(images, num_images, mid_size, mid_size, x_coords, y_coords)) upper_size = mid_size;
                else lower_size = mid_size;
            }

            int upper_height = upper_size;
            int lower_height = 0;
            while (upper_height - lower_height > 2) {
                const int mid_height = (upper_height + lower_height) / 2;
                if (pack_rects_fixed(images, num_images, upper_size, mid_height, x_coords, y_coords))
                    upper_height = mid_height;
                else lower_height = mid_height;
            }

            int upper_width = upper_size;
            int lower_width = 0;
            while (upper_width - lower_width > 2) {
                const int mid_width = (upper_width + lower_width) / 2;
                if (pack_rects_fixed(images, num_images, mid_width, upper_height, x_coords, y_coords))
                    upper_width = mid_width;
                else lower_width = mid_width;
            }

            int width, height;
            if (upper_width < upper_height) {
                width = upper_width;
                height = upper_size;
            } else {
                width = upper_size;
                height = upper_height;
            }

            assert(pack_rects_fixed(images, num_images, width, height, x_coords, y_coords));
            *width_ptr = width;
            *height_ptr = height;
        }
    } // namespace

    void image_create_atlas(Image *output, Image *images, int num_images, int *x_coords, int *y_coords) {
        int width, height;
        pack_rects(images, num_images, &width, &height, x_coords, y_coords);

        image_new(output, static_cast<std::uint16_t>(width), static_cast<std::uint16_t>(height), 0, 0, 0);
        std::memset(output->pixels, 0, static_cast<std::size_t>(width) * height);
        int used_pixels = 0;
        for (int i = 0; i < num_images; ++i) {
            image_blit(output, images + i,
                       static_cast<std::int16_t>(x_coords[i] - images[i].x_offset),
                       static_cast<std::int16_t>(y_coords[i] - images[i].y_offset));
            used_pixels += images[i].width * images[i].height;
        }

        std::printf("Width %d Height %d\n", width, height);
        std::printf("Packing efficiency %.01f%%\n", (100.0 * used_pixels) / (width * height));
    }

    void image_create_grid(Image *output, Image *images, int num_images, int * /*x_coords*/, int * /*y_coords*/,
                           int columns) {
        int x_min = 0;
        int x_max = 0;
        int y_min = 0;
        int y_max = 0;

        for (int i = 0; i < num_images; ++i) {
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

        for (int i = 0; i < num_images; ++i) {
            const int row = i / columns;
            const int column = i - row * columns;
            image_blit(output, images + i,
                       static_cast<std::int16_t>(column_width * column - x_min),
                       static_cast<std::int16_t>(row_height * row - y_min));
        }

        std::printf("Width %d Height %d\n", width, height);
    }
}