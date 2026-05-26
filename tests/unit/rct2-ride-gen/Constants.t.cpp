#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>

#include "Constants.hpp"

using namespace RCTGen;

TEST(Constants, SpriteFlagBitwiseOr) {
    constexpr auto combined = SpriteFlag::flatSlope | SpriteFlag::gentleSlope;
    using U = std::underlying_type_t<SpriteFlag>;
    EXPECT_EQ(static_cast<U>(combined),
              static_cast<U>(SpriteFlag::flatSlope) | static_cast<U>(SpriteFlag::gentleSlope));
}

TEST(Constants, SpriteFlagBitwiseAnd) {
    constexpr auto combined = SpriteFlag::flatSlope | SpriteFlag::gentleSlope;
    EXPECT_EQ(static_cast<std::uint32_t>(combined & SpriteFlag::flatSlope),
              static_cast<std::uint32_t>(SpriteFlag::flatSlope));
}

TEST(Constants, SpriteFlagBitwiseOrAssign) {
    SpriteFlag f = SpriteFlag::flatSlope;
    f |= SpriteFlag::banking;
    EXPECT_TRUE(has_flag(f, SpriteFlag::flatSlope));
    EXPECT_TRUE(has_flag(f, SpriteFlag::banking));
    EXPECT_FALSE(has_flag(f, SpriteFlag::gentleSlope));
}

TEST(Constants, HasFlagSpriteFlag) {
    constexpr auto set = SpriteFlag::flatSlope | SpriteFlag::corkscrew;
    EXPECT_TRUE(has_flag(set, SpriteFlag::flatSlope));
    EXPECT_TRUE(has_flag(set, SpriteFlag::corkscrew));
    EXPECT_FALSE(has_flag(set, SpriteFlag::diveLoop));
}

TEST(Constants, HasFlagRideFlag) {
    constexpr auto set = RideFlag::noCollisionCrashes;
    EXPECT_TRUE(has_flag(set, RideFlag::noCollisionCrashes));
    EXPECT_FALSE(has_flag(set, RideFlag::riderControlsSpeed));
}

TEST(Constants, HasFlagVehicleFlag) {
    auto set = VehicleFlag::ridersScream;
    set |= VehicleFlag::restraintAnimation;
    EXPECT_TRUE(has_flag(set, VehicleFlag::ridersScream));
    EXPECT_TRUE(has_flag(set, VehicleFlag::restraintAnimation));
    EXPECT_FALSE(has_flag(set, VehicleFlag::secondaryRemap));
    EXPECT_FALSE(has_flag(set, VehicleFlag::tertiaryRemap));
}

TEST(Constants, SpriteFlagValuesAreSingleBits) {
    using U = std::underlying_type_t<SpriteFlag>;
    auto flags = {
        SpriteFlag::flatSlope, SpriteFlag::gentleSlope, SpriteFlag::steepSlope,
        SpriteFlag::verticalSlope, SpriteFlag::diagonalSlope, SpriteFlag::banking,
        SpriteFlag::inlineTwist, SpriteFlag::slopeBankTransition,
        SpriteFlag::diagonalBankTransition, SpriteFlag::slopedBankTransition,
        SpriteFlag::slopedBankedTurn, SpriteFlag::bankedSlopeTransition,
        SpriteFlag::corkscrew, SpriteFlag::zeroGRoll,
        SpriteFlag::diagonalSlopedBankTransition, SpriteFlag::diveLoop,
    };
    for (auto f: flags) {
        U v = static_cast<U>(f);
        EXPECT_NE(v, 0u);
        EXPECT_EQ(v & (v - 1), 0u) << "Value " << v << " is not a single bit";
    }
}

TEST(Constants, NameTablesSized) {
    EXPECT_EQ(kSpriteGroupNames.size(), 16u);
    EXPECT_EQ(kRideFlagNames.size(), 2u);
    EXPECT_EQ(kVehicleFlagNames.size(), 4u);
    EXPECT_EQ(kRunningSoundNames.size(), 6u);
    EXPECT_EQ(kSecondarySoundNames.size(), 4u);
    EXPECT_EQ(kColorNames.size(), 32u);
    EXPECT_EQ(kCategoryNames.size(), 4u);
}

TEST(Constants, NameTablesNonEmpty) {
    for (auto n: kSpriteGroupNames) EXPECT_FALSE(n.empty());
    for (auto n: kColorNames) EXPECT_FALSE(n.empty());
    for (auto n: kRideFlagNames) EXPECT_FALSE(n.empty());
}

TEST(Constants, RunningSoundValuesMatchEnum) {
    ASSERT_EQ(kRunningSoundValues.size(), 6u);
    EXPECT_EQ(kRunningSoundValues[0], std::to_underlying(RunningSound::woodenOld));
    EXPECT_EQ(kRunningSoundValues[1], std::to_underlying(RunningSound::woodenModern));
    EXPECT_EQ(kRunningSoundValues[2], std::to_underlying(RunningSound::steel));
    EXPECT_EQ(kRunningSoundValues[3], std::to_underlying(RunningSound::steelSmooth));
    EXPECT_EQ(kRunningSoundValues[4], std::to_underlying(RunningSound::waterslide));
    EXPECT_EQ(kRunningSoundValues[5], std::to_underlying(RunningSound::train));
}

TEST(Constants, SentinelEnumValues) {
    EXPECT_EQ(std::to_underlying(RunningSound::none), 255u);
    EXPECT_EQ(std::to_underlying(SecondarySound::none), 255u);
}

TEST(Constants, TileSize) {
    EXPECT_FLOAT_EQ(kTileSize, 3.3f);
}