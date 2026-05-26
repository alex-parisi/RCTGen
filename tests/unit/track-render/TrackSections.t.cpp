#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <set>
#include <string_view>

#include "Constants.hpp"
#include "Track.hpp"
#include "VectorMath.hpp"

using namespace RCTGen;

namespace {
    constexpr float kEps = 1e-4f;
} // namespace

TEST(TrackSectionTable, HasExpectedSize) {
    EXPECT_EQ(kTrackSections.size(), static_cast<std::size_t>(kNumTrackSections));
}

TEST(TrackSectionTable, EveryEntryHasNameAndCurve) {
    for (std::size_t i = 0; i < kTrackSections.size(); ++i) {
        const auto &s = kTrackSections[i];
        ASSERT_NE(s.name, nullptr) << "section " << i;
        EXPECT_NE(*s.name, '\0') << "section " << i << " has empty name";
        EXPECT_NE(s.curve, nullptr) << "section " << i << " missing curve fn";
        EXPECT_GT(s.length, 0.0f) << "section " << i << " (" << s.name << ") has non-positive length";
    }
}

TEST(TrackSectionTable, NamesAreUnique) {
    std::set<std::string_view> seen;
    for (const auto &s: kTrackSections) {
        auto [_, inserted] = seen.emplace(s.name);
        EXPECT_TRUE(inserted) << "duplicate section name: " << s.name;
    }
}

TEST(TrackSectionTable, FlatSectionHasExpectedShape) {
    // Section 0 is "flat" (see kTrackSections initializer order).
    const auto &flat = kTrackSections[0];
    EXPECT_STREQ(flat.name, "flat");
    EXPECT_FLOAT_EQ(flat.length, static_cast<float>(kTileSize));
    ASSERT_NE(flat.curve, nullptr);

    // Flat curve at distance 0 starts at origin pointing +z.
    TrackPoint start = flat.curve(0.0f);
    EXPECT_NEAR(start.position.x, 0.0f, kEps);
    EXPECT_NEAR(start.position.y, 0.0f, kEps);
    EXPECT_NEAR(start.position.z, 0.0f, kEps);
    EXPECT_NEAR(start.tangent.x, 0.0f, kEps);
    EXPECT_NEAR(start.tangent.y, 0.0f, kEps);
    EXPECT_NEAR(start.tangent.z, 1.0f, kEps);

    // At the end of the section, position has advanced by tile size along z.
    TrackPoint end = flat.curve(static_cast<float>(kTileSize));
    EXPECT_NEAR(end.position.x, 0.0f, kEps);
    EXPECT_NEAR(end.position.y, 0.0f, kEps);
    EXPECT_NEAR(end.position.z, static_cast<float>(kTileSize), kEps);
    EXPECT_NEAR(end.tangent.z, 1.0f, kEps);
}

TEST(TrackSectionTable, FlatTangentNormalBinormalAreOrthonormal) {
    const auto &flat = kTrackSections[0];
    TrackPoint p = flat.curve(0.5f * static_cast<float>(kTileSize));
    EXPECT_NEAR(vector3_norm(p.tangent), 1.0f, kEps);
    EXPECT_NEAR(vector3_norm(p.normal), 1.0f, kEps);
    EXPECT_NEAR(vector3_norm(p.binormal), 1.0f, kEps);
    EXPECT_NEAR(vector3_dot(p.tangent, p.normal), 0.0f, kEps);
    EXPECT_NEAR(vector3_dot(p.tangent, p.binormal), 0.0f, kEps);
    EXPECT_NEAR(vector3_dot(p.normal, p.binormal), 0.0f, kEps);
}

TEST(TrackSectionTable, BrakeSharesFlatCurve) {
    // brake is at index 2 (see kTrackSections initializer order).
    const auto &brake = kTrackSections[2];
    EXPECT_STREQ(brake.name, "brake");
    EXPECT_EQ(static_cast<std::uint32_t>(brake.flags & TrackFlag::specialMask),
              static_cast<std::uint32_t>(TrackFlag::specialBrake));
    EXPECT_FLOAT_EQ(brake.length, kTrackSections[0].length);
}

TEST(TrackSectionTable, SectionIdsResolveByOrder) {
    // The first 13 sections should follow the TrackSectionId initializer order
    // (flat..booster) -- guard against reordering of the kTrackSections array.
    EXPECT_STREQ(kTrackSections[static_cast<int>(TrackSectionId::flat)].name, "flat");
    EXPECT_STREQ(kTrackSections[static_cast<int>(TrackSectionId::brake)].name, "brake");
    EXPECT_STREQ(kTrackSections[static_cast<int>(TrackSectionId::booster)].name, "booster");
}