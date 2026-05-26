#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>

#include "Constants.hpp"

using namespace RCTGen;

TEST(TrackConstants, TrackFlagBitwiseOr) {
    constexpr auto kCombined = TrackFlag::diagonal | TrackFlag::vertical;
    using U = std::underlying_type_t<TrackFlag>;
    EXPECT_EQ(static_cast<U>(kCombined),
              static_cast<U>(TrackFlag::diagonal) | static_cast<U>(TrackFlag::vertical));
}

TEST(TrackConstants, TrackFlagBitwiseAnd) {
    constexpr auto kCombined = TrackFlag::entryBankLeft | TrackFlag::exitBankLeft;
    EXPECT_EQ(static_cast<std::uint32_t>(kCombined & TrackFlag::entryBankLeft),
              static_cast<std::uint32_t>(TrackFlag::entryBankLeft));
}

TEST(TrackConstants, TrackFlagOrAssign) {
    TrackFlag f = TrackFlag::diagonal;
    f |= TrackFlag::noSupports;
    EXPECT_TRUE(has_flag(f, TrackFlag::diagonal));
    EXPECT_TRUE(has_flag(f, TrackFlag::noSupports));
    EXPECT_FALSE(has_flag(f, TrackFlag::vertical));
}

TEST(TrackConstants, BankLeftIsEntryPlusExit) {
    using U = std::underlying_type_t<TrackFlag>;
    EXPECT_EQ(static_cast<U>(TrackFlag::bankLeft),
              static_cast<U>(TrackFlag::entryBankLeft) | static_cast<U>(TrackFlag::exitBankLeft));
    EXPECT_EQ(static_cast<U>(TrackFlag::bankRight),
              static_cast<U>(TrackFlag::entryBankRight) | static_cast<U>(TrackFlag::exitBankRight));
}

TEST(TrackConstants, Exit90DegCombinesLeftRight) {
    using U = std::underlying_type_t<TrackFlag>;
    EXPECT_EQ(static_cast<U>(TrackFlag::exit90Deg),
              static_cast<U>(TrackFlag::exit90DegLeft) | static_cast<U>(TrackFlag::exit90DegRight));
}

TEST(TrackConstants, SpecialMaskIsolatesSpecialBits) {
    using U = std::underlying_type_t<TrackFlag>;
    const auto mixed = TrackFlag::diagonal | TrackFlag::specialBrake;
    EXPECT_EQ(static_cast<U>(mixed & TrackFlag::specialMask),
              static_cast<U>(TrackFlag::specialBrake));
}

TEST(TrackConstants, AnyOfDetectsOverlap) {
    constexpr auto kSet = TrackFlag::diagonal | TrackFlag::entryBankLeft;
    EXPECT_TRUE(any_of(kSet, TrackFlag::entryBankLeft));
    EXPECT_TRUE(any_of(kSet, TrackFlag::bankLeft)); // bankLeft includes entryBankLeft
    EXPECT_FALSE(any_of(kSet, TrackFlag::vertical));
}

TEST(TrackConstants, TrackTypeFlagOps) {
    auto f = TrackTypeFlag::hasLift | TrackTypeFlag::hasSupports;
    EXPECT_TRUE(has_flag(f, TrackTypeFlag::hasLift));
    EXPECT_TRUE(has_flag(f, TrackTypeFlag::hasSupports));
    EXPECT_FALSE(has_flag(f, TrackTypeFlag::separateTie));
}

TEST(TrackConstants, ViewFlagOps) {
    auto f = ViewFlag::needsTrackMask | ViewFlag::extrudeBehind;
    EXPECT_TRUE(has_flag(f, ViewFlag::needsTrackMask));
    EXPECT_TRUE(has_flag(f, ViewFlag::extrudeBehind));
    EXPECT_FALSE(has_flag(f, ViewFlag::extrudeInFront));
    EXPECT_FALSE(has_flag(f, ViewFlag::maskEnd));
}

TEST(TrackConstants, TrackGroupOps) {
    auto g = TrackGroup::flat | TrackGroup::brakes | TrackGroup::diveLoops;
    EXPECT_TRUE(has_flag(g, TrackGroup::flat));
    EXPECT_TRUE(has_flag(g, TrackGroup::brakes));
    EXPECT_TRUE(has_flag(g, TrackGroup::diveLoops));
    EXPECT_FALSE(has_flag(g, TrackGroup::corkscrews));
}

TEST(TrackConstants, TrackGroupHighBitsFitInUint64) {
    using U = std::underlying_type_t<TrackGroup>;
    static_assert(std::is_same_v<U, std::uint64_t>);
    EXPECT_EQ(static_cast<U>(TrackGroup::bankedZeroGRolls), 1ull << 42);
}

TEST(TrackConstants, TileSizeIsThreePointThree) {
    EXPECT_DOUBLE_EQ(kTileSize, 3.3);
}

TEST(TrackConstants, ClearanceHeightDerivedFromTileSize) {
    EXPECT_DOUBLE_EQ(kClearanceHeight, 0.20412414523 * kTileSize);
}

TEST(TrackConstants, TrackSectionIdCountMatchesNumSections) {
    EXPECT_EQ(static_cast<int>(TrackSectionId::count_), kNumTrackSections);
}

TEST(TrackConstants, NumModelsMatchesEnumSize) {
    EXPECT_EQ(static_cast<int>(TrackModel::specialLargeZeroGRoll) + 1, kNumModels);
}