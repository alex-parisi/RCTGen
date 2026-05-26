#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>

#include "Color.hpp"
#include "Palette.hpp"
#include "VectorMath.hpp"

using namespace RCTGen;

namespace {
    constexpr float kEps = 1e-4f;
} // namespace

TEST(ColorCtor, BuildsExpectedValues) {
    constexpr Color kC = color(10, 20, 30);
    EXPECT_EQ(kC.r, 10);
    EXPECT_EQ(kC.g, 20);
    EXPECT_EQ(kC.b, 30);
}

TEST(VectorFromColor, BlackIsZero) {
    Vector3 v = vector_from_color(color(0, 0, 0));
    EXPECT_NEAR(v.x, 0.0f, kEps);
    EXPECT_NEAR(v.y, 0.0f, kEps);
    EXPECT_NEAR(v.z, 0.0f, kEps);
}

TEST(VectorFromColor, WhiteIsOne) {
    Vector3 v = vector_from_color(color(255, 255, 255));
    EXPECT_NEAR(v.x, 1.0f, kEps);
    EXPECT_NEAR(v.y, 1.0f, kEps);
    EXPECT_NEAR(v.z, 1.0f, kEps);
}

TEST(VectorFromColor, MonotonicInChannel) {
    Vector3 lo = vector_from_color(color(64, 0, 0));
    Vector3 hi = vector_from_color(color(192, 0, 0));
    EXPECT_LT(lo.x, hi.x);
    EXPECT_FLOAT_EQ(lo.y, 0.0f);
    EXPECT_FLOAT_EQ(lo.z, 0.0f);
}

TEST(ColorFromVector, ZeroVectorIsBlack) {
    Color c = color_from_vector(vector3(0, 0, 0));
    EXPECT_EQ(c.r, 0);
    EXPECT_EQ(c.g, 0);
    EXPECT_EQ(c.b, 0);
}

TEST(ColorFromVector, OneVectorIsWhite) {
    Color c = color_from_vector(vector3(1, 1, 1));
    EXPECT_EQ(c.r, 255);
    EXPECT_EQ(c.g, 255);
    EXPECT_EQ(c.b, 255);
}

TEST(ColorFromVector, ClampsAbove) {
    Color c = color_from_vector(vector3(2.0f, 5.0f, 10.0f));
    EXPECT_EQ(c.r, 255);
    EXPECT_EQ(c.g, 255);
    EXPECT_EQ(c.b, 255);
}

TEST(ColorFromVector, ClampsBelow) {
    Color c = color_from_vector(vector3(-1.0f, -100.0f, -0.5f));
    EXPECT_EQ(c.r, 0);
    EXPECT_EQ(c.g, 0);
    EXPECT_EQ(c.b, 0);
}

TEST(ColorRoundTrip, RecoversCloseValue) {
    // sRGB->linear->sRGB should round-trip within 1 LSB.
    for (std::uint8_t v: {std::uint8_t{17}, std::uint8_t{64}, std::uint8_t{128}, std::uint8_t{200}}) {
        Color in = color(v, v, v);
        Color out = color_from_vector(vector_from_color(in));
        EXPECT_LE(std::abs(int{out.r} - int{in.r}), 1);
        EXPECT_LE(std::abs(int{out.g} - int{in.g}), 1);
        EXPECT_LE(std::abs(int{out.b} - int{in.b}), 1);
    }
}

TEST(PaletteRct2, BuildsAndHasTransparentIndexZero) {
    Palette p = palette_rct2();
    EXPECT_EQ(p.transparent_index, 0);
}

TEST(PaletteRct2, RegionsConfigured) {
    Palette p = palette_rct2();
    // Region 0 is the grey base (3 subregions covering indices 10..202).
    EXPECT_EQ(p.regions[0].subregions, 3u);
    EXPECT_EQ(p.regions[0].remap, false);
    // Region 1 is the primary remap region.
    EXPECT_EQ(p.regions[1].remap, true);
}

TEST(PaletteGetNearest, ExactGreyMatchesPaletteIndex) {
    Palette p = palette_rct2();
    // Index 10 is {23, 35, 35}, the start of region 0's first subregion.
    Vector3 target = vector_from_color(p.colors[10]);
    Vector3 err{};
    std::uint8_t idx = palette_get_nearest(&p, 0, target, &err);
    EXPECT_EQ(idx, 10);
    EXPECT_NEAR(vector3_norm(err), 0.0f, kEps);
}

TEST(PaletteGetNearest, NullErrorPointerOk) {
    Palette p = palette_rct2();
    Vector3 target = vector_from_color(p.colors[12]);
    std::uint8_t idx = palette_get_nearest(&p, 0, target, nullptr);
    EXPECT_EQ(idx, 12);
}

TEST(PaletteGetNearest, ReturnsIndexInOneOfRegionSubregions) {
    Palette p = palette_rct2();
    // Some arbitrary mid-gray.
    Vector3 target = vector_from_color(color(100, 100, 100));
    std::uint8_t idx = palette_get_nearest(&p, 0, target, nullptr);
    // The returned index must fall inside one of region 0's subregions.
    const auto &reg = p.regions[0];
    bool in_some_subregion = false;
    for (int s = 0; s < reg.subregions; ++s) {
        if (idx >= reg.start_indices[s] && idx < reg.end_indices[s]) {
            in_some_subregion = true;
            break;
        }
    }
    EXPECT_TRUE(in_some_subregion) << "idx " << int{idx} << " outside all subregions";
}