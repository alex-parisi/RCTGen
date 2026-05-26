#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>

#include "Image.hpp"

using namespace RCTGen;

namespace {
    void fillSolid(Image &img, std::uint8_t value) {
        std::memset(img.pixels, value, static_cast<std::size_t>(img.width) * img.height);
    }
} // namespace

TEST(ImageNew, AllocatesAndZeroFills) {
    Image img;
    image_new(&img, 8, 4, -2, 3);
    EXPECT_EQ(img.width, 8);
    EXPECT_EQ(img.height, 4);
    EXPECT_EQ(img.x_offset, -2);
    EXPECT_EQ(img.y_offset, 3);
    ASSERT_NE(img.pixels, nullptr);
    for (int i = 0; i < 8 * 4; ++i)
        EXPECT_EQ(img.pixels[i], 0u);
    image_destroy(&img);
}

TEST(ImageNew, ZeroDimensionsDoesNotCrash) {
    Image img;
    image_new(&img, 0, 0, 0, 0);
    image_destroy(&img);
}

TEST(ImageCopy, DuplicatesPixelsAndMetadata) {
    Image src;
    image_new(&src, 4, 3, 1, -1);
    for (std::size_t i = 0; i < 4u * 3u; ++i)
        src.pixels[i] = static_cast<std::uint8_t>(i + 1);

    Image dst;
    image_copy(&src, &dst);
    EXPECT_EQ(dst.width, src.width);
    EXPECT_EQ(dst.height, src.height);
    EXPECT_EQ(dst.x_offset, src.x_offset);
    EXPECT_EQ(dst.y_offset, src.y_offset);
    ASSERT_NE(dst.pixels, nullptr);
    EXPECT_NE(dst.pixels, src.pixels); // distinct allocation
    for (std::size_t i = 0; i < 4u * 3u; ++i)
        EXPECT_EQ(dst.pixels[i], src.pixels[i]);

    // Mutating dst must not affect src.
    dst.pixels[0] = 99;
    EXPECT_EQ(src.pixels[0], 1u);

    image_destroy(&src);
    image_destroy(&dst);
}

TEST(ImageBlit, CopiesNonZeroPixelsAtOffset) {
    Image dst;
    image_new(&dst, 8, 8, 0, 0);

    Image src;
    image_new(&src, 2, 2, 0, 0);
    src.pixels[0] = 5;
    src.pixels[1] = 0; // transparent
    src.pixels[2] = 0; // transparent
    src.pixels[3] = 7;

    image_blit(&dst, &src, 3, 4);
    EXPECT_EQ(dst.pixels[4 * 8 + 3], 5u);
    EXPECT_EQ(dst.pixels[4 * 8 + 4], 0u);
    EXPECT_EQ(dst.pixels[5 * 8 + 3], 0u);
    EXPECT_EQ(dst.pixels[5 * 8 + 4], 7u);
    // Unaffected cell.
    EXPECT_EQ(dst.pixels[0], 0u);

    image_destroy(&src);
    image_destroy(&dst);
}

TEST(ImageBlit, ZeroPixelsDoNotOverwriteDst) {
    Image dst;
    image_new(&dst, 4, 1, 0, 0);
    fillSolid(dst, 42);

    Image src;
    image_new(&src, 4, 1, 0, 0);
    // All zero by default.

    image_blit(&dst, &src, 0, 0);
    for (int i = 0; i < 4; ++i)
        EXPECT_EQ(dst.pixels[i], 42u);

    image_destroy(&src);
    image_destroy(&dst);
}

TEST(ImageBlit, RespectsImageOffsets) {
    Image dst;
    image_new(&dst, 4, 4, 0, 0);

    // Source has its own offset (-1, -1); a (1,1) blit shifts target by that.
    Image src;
    image_new(&src, 1, 1, -1, -1);
    src.pixels[0] = 9;

    image_blit(&dst, &src, 2, 2);
    // x_offset = 2 + src.x_offset(-1) - dst.x_offset(0) = 1
    // y_offset = 2 + src.y_offset(-1) - dst.y_offset(0) = 1
    EXPECT_EQ(dst.pixels[1 * 4 + 1], 9u);

    image_destroy(&src);
    image_destroy(&dst);
}

TEST(ImageCrop, ShrinksToBoundingBox) {
    Image img;
    image_new(&img, 6, 5, 0, 0);
    // Place opaque pixels at (2,1) and (4,3) -> bbox 3x3.
    img.pixels[1 * 6 + 2] = 11;
    img.pixels[3 * 6 + 4] = 22;
    image_crop(&img);
    EXPECT_EQ(img.width, 3);
    EXPECT_EQ(img.height, 3);
    EXPECT_EQ(img.x_offset, 2);
    EXPECT_EQ(img.y_offset, 1);
    EXPECT_EQ(img.pixels[0 * 3 + 0], 11u);
    EXPECT_EQ(img.pixels[2 * 3 + 2], 22u);
    image_destroy(&img);
}

TEST(ImageCrop, EmptyImageCollapsesTo1x1) {
    Image img;
    image_new(&img, 4, 4, 5, 7);
    image_crop(&img);
    EXPECT_EQ(img.width, 1);
    EXPECT_EQ(img.height, 1);
    EXPECT_EQ(img.x_offset, 0);
    EXPECT_EQ(img.y_offset, 0);
    image_destroy(&img);
}

TEST(ImageCrop, PreservesExistingOffsetWhenNonEmpty) {
    Image img;
    image_new(&img, 4, 4, 10, -5);
    img.pixels[2 * 4 + 1] = 3;
    image_crop(&img);
    EXPECT_EQ(img.width, 1);
    EXPECT_EQ(img.height, 1);
    EXPECT_EQ(img.x_offset, 10 + 1);
    EXPECT_EQ(img.y_offset, -5 + 2);
    EXPECT_EQ(img.pixels[0], 3u);
    image_destroy(&img);
}

TEST(ImageCreateGrid, LaysOutSubimagesByColumn) {
    constexpr int kCount = 4;
    Image parts[kCount];
    for (int i = 0; i < kCount; ++i) {
        image_new(&parts[i], 2, 2, 0, 0);
        fillSolid(parts[i], static_cast<std::uint8_t>(i + 1));
    }
    Image grid;
    int xs[kCount] = {0, 0, 0, 0};
    int ys[kCount] = {0, 0, 0, 0};
    image_create_grid(&grid, parts, kCount, xs, ys, /*columns=*/2);
    // 2 columns x 2 rows of 2x2 tiles = 4x4 image.
    EXPECT_EQ(grid.width, 4);
    EXPECT_EQ(grid.height, 4);
    // Tile 0 (value 1) sits at the top-left corner.
    EXPECT_EQ(grid.pixels[0 * 4 + 0], 1u);
    // Tile 3 (value 4) sits at the bottom-right corner.
    EXPECT_EQ(grid.pixels[3 * 4 + 3], 4u);

    for (int i = 0; i < kCount; ++i) image_destroy(&parts[i]);
    image_destroy(&grid);
}