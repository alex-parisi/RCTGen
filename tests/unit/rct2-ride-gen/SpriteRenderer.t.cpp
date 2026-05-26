#include <gtest/gtest.h>

#include "Constants.hpp"
#include "SpriteRenderer.hpp"

using namespace RCTGen;

namespace {
    constexpr VehicleFlag kNoVehicleFlags = static_cast<VehicleFlag>(0);
    constexpr SpriteFlag kNoSpriteFlags = static_cast<SpriteFlag>(0);
} // namespace

TEST(CountSprites, NoFlagsZero) {
    EXPECT_EQ(countSprites(kNoSpriteFlags, kNoVehicleFlags), 0);
}

TEST(CountSprites, FlatSlope) {
    EXPECT_EQ(countSprites(SpriteFlag::flatSlope, kNoVehicleFlags), 32);
}

TEST(CountSprites, GentleSlope) {
    EXPECT_EQ(countSprites(SpriteFlag::gentleSlope, kNoVehicleFlags), 72);
}

TEST(CountSprites, SteepSlope) {
    EXPECT_EQ(countSprites(SpriteFlag::steepSlope, kNoVehicleFlags), 80);
}

TEST(CountSprites, VerticalSlope) {
    EXPECT_EQ(countSprites(SpriteFlag::verticalSlope, kNoVehicleFlags), 116);
}

TEST(CountSprites, DiagonalSlope) {
    EXPECT_EQ(countSprites(SpriteFlag::diagonalSlope, kNoVehicleFlags), 24);
}

TEST(CountSprites, Banking) {
    EXPECT_EQ(countSprites(SpriteFlag::banking, kNoVehicleFlags), 80);
}

TEST(CountSprites, InlineTwist) {
    EXPECT_EQ(countSprites(SpriteFlag::inlineTwist, kNoVehicleFlags), 40);
}

TEST(CountSprites, SlopeBankTransition) {
    EXPECT_EQ(countSprites(SpriteFlag::slopeBankTransition, kNoVehicleFlags), 128);
}

TEST(CountSprites, DiagonalBankTransition) {
    EXPECT_EQ(countSprites(SpriteFlag::diagonalBankTransition, kNoVehicleFlags), 16);
}

TEST(CountSprites, SlopedBankTransition) {
    EXPECT_EQ(countSprites(SpriteFlag::slopedBankTransition, kNoVehicleFlags), 16);
}

TEST(CountSprites, DiagonalSlopedBankTransition) {
    EXPECT_EQ(countSprites(SpriteFlag::diagonalSlopedBankTransition, kNoVehicleFlags), 48);
}

TEST(CountSprites, SlopedBankedTurn) {
    EXPECT_EQ(countSprites(SpriteFlag::slopedBankedTurn, kNoVehicleFlags), 128);
}

TEST(CountSprites, BankedSlopeTransition) {
    EXPECT_EQ(countSprites(SpriteFlag::bankedSlopeTransition, kNoVehicleFlags), 16);
}

TEST(CountSprites, Corkscrew) {
    EXPECT_EQ(countSprites(SpriteFlag::corkscrew, kNoVehicleFlags), 80);
}

TEST(CountSprites, ZeroGRoll) {
    EXPECT_EQ(countSprites(SpriteFlag::zeroGRoll, kNoVehicleFlags), 160);
}

TEST(CountSprites, DiveLoop) {
    EXPECT_EQ(countSprites(SpriteFlag::diveLoop, kNoVehicleFlags), 112);
}

TEST(CountSprites, RestraintAnimationContribution) {
    EXPECT_EQ(countSprites(kNoSpriteFlags, VehicleFlag::restraintAnimation), 12);
}

TEST(CountSprites, OtherVehicleFlagsDontAffectCount) {
    auto vf = VehicleFlag::secondaryRemap | VehicleFlag::tertiaryRemap |
              VehicleFlag::ridersScream;
    EXPECT_EQ(countSprites(kNoSpriteFlags, vf), 0);
    EXPECT_EQ(countSprites(SpriteFlag::flatSlope, vf), 32);
}

TEST(CountSprites, FlagsCombineAdditively) {
    auto sf = SpriteFlag::flatSlope | SpriteFlag::gentleSlope | SpriteFlag::banking;
    EXPECT_EQ(countSprites(sf, kNoVehicleFlags), 32 + 72 + 80);
}

TEST(CountSprites, RestraintAddsOnTopOfSpriteCount) {
    EXPECT_EQ(countSprites(SpriteFlag::flatSlope, VehicleFlag::restraintAnimation),
              32 + 12);
}

TEST(CountSprites, AllSpriteFlagsSum) {
    // Sum of every per-group value above.
    constexpr int kExpected =
            32 + 72 + 80 + 116 + 24 + 80 + 40 + 128 + 16 + 16 + 48 + 128 + 16 + 80 + 160 + 112;
    SpriteFlag all = SpriteFlag::flatSlope;
    for (auto f: {
             SpriteFlag::gentleSlope, SpriteFlag::steepSlope, SpriteFlag::verticalSlope,
             SpriteFlag::diagonalSlope, SpriteFlag::banking, SpriteFlag::inlineTwist,
             SpriteFlag::slopeBankTransition, SpriteFlag::diagonalBankTransition,
             SpriteFlag::slopedBankTransition, SpriteFlag::diagonalSlopedBankTransition,
             SpriteFlag::slopedBankedTurn, SpriteFlag::bankedSlopeTransition,
             SpriteFlag::corkscrew, SpriteFlag::zeroGRoll, SpriteFlag::diveLoop,
         }) {
        all |= f;
    }
    EXPECT_EQ(countSprites(all, kNoVehicleFlags), kExpected);
}