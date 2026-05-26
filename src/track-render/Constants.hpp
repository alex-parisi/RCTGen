/// Constants.hpp

#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace RCTGen {
    // Kept at double precision to match master's macro-based #define TILE_SIZE 3.3
    // semantics: expressions like `0.5f * kTileSize` then promote `0.5f` to
    // double, evaluate as double, and only truncate to float at the final assign.
    // A float kTileSize breaks byte-equivalence with the goldens by ~70 sprites.
    inline constexpr double kTileSize = 3.3;
    inline constexpr double kClearanceHeight = 0.20412414523 * kTileSize;

    inline constexpr int kNumTrackSections = 170;
    inline constexpr int kNumModels = 23;

    // Per-section flag bits, combined with bitwise OR.
    enum class TrackFlag : std::uint32_t {
        none = 0,
        diagonal = 1u << 0,
        vertical = 1u << 1,
        entryBankLeft = 1u << 3,
        entryBankRight = 1u << 4,
        exitBankLeft = 1u << 5,
        exitBankRight = 1u << 6,
        exit45DegLeft = 1u << 7,
        exit45DegRight = 1u << 8,
        exit90DegLeft = 1u << 9,
        exit90DegRight = 1u << 10,
        exit180Deg = 1u << 11,
        altPreferOdd = 1u << 12,
        altInvert = 1u << 13,
        noSupports = 1u << 14,
        offsetSpriteMask = 1u << 15,
        supportBase = 1u << 16,
        diagonal2 = 1u << 17,
        specialSteepToVertical = 0x01000000,
        specialVerticalToSteep = 0x02000000,
        specialVertical = 0x03000000,
        specialVerticalTwistLeft = 0x04000000,
        specialVerticalTwistRight = 0x05000000,
        specialBarrelRollLeft = 0x06000000,
        specialBarrelRollRight = 0x07000000,
        specialHalfLoop = 0x08000000,
        specialQuarterLoop = 0x09000000,
        specialCorkscrewLeft = 0x0A000000,
        specialCorkscrewRight = 0x0B000000,
        specialZeroGRollLeft = 0x0C000000,
        specialZeroGRollRight = 0x0D000000,
        specialLargeZeroGRollLeft = 0x0E000000,
        specialLargeZeroGRollRight = 0x0F000000,
        specialBrake = 0x10000000,
        specialBlockBrake = 0x11000000,
        specialBooster = 0x12000000,
        specialMagneticBrake = 0x13000000,
        specialLaunchedLift = 0x14000000,
        specialVerticalBooster = 0x15000000,
        specialMask = 0xFF000000,

        exit90Deg = exit90DegLeft | exit90DegRight,
        exit45Deg = exit45DegLeft | exit45DegRight,
        bankLeft = entryBankLeft | exitBankLeft,
        bankRight = entryBankRight | exitBankRight,
    };

    enum class TrackTypeFlag : std::uint32_t {
        none = 0,
        hasLift = 1u << 0,
        hasSupports = 1u << 1,
        separateTie = 1u << 2,
        tieAtBoundary = 1u << 3,
        specialOffsets = 1u << 4,
    };

    enum class ViewFlag : std::uint32_t {
        none = 0,
        needsTrackMask = 1u << 0,
        extrudeBehind = 1u << 1,
        extrudeInFront = 1u << 2,
        maskEnd = 1u << 3,
    };

    enum class MaskOp : std::int32_t {
        none = 0,
        difference = 1,
        intersect = 2,
        transferNext = 3,
    };

    enum class TrackGroup : std::uint64_t {
        none = 0,
        flat = 1uLL << 0,
        brakes = 1uLL << 1,
        gentleSlopes = 1uLL << 2,
        steepSlopes = 1uLL << 3,
        verticalSlopes = 1uLL << 4,
        turns = 1uLL << 5,
        slopedTurns = 1uLL << 6,
        diagonals = 1uLL << 7,
        bankedTurns = 1uLL << 8,
        bankedSlopedTurns = 1uLL << 9,
        sBends = 1uLL << 10,
        helices = 1uLL << 11,
        largeSlopeTransitions = 1uLL << 12,
        barrelRolls = 1uLL << 13,
        inlineTwists = 1uLL << 14,
        halfLoops = 1uLL << 15,
        quarterLoops = 1uLL << 16,
        corkscrews = 1uLL << 17,
        largeCorkscrews = 1uLL << 18,
        boosters = 1uLL << 19,
        launchedLifts = 1uLL << 20,
        turnBankTransitions = 1uLL << 21,
        mediumHalfLoops = 1uLL << 22,
        largeHalfLoops = 1uLL << 23,
        zeroGRolls = 1uLL << 24,
        smallSlopeTransitions = 1uLL << 25,
        steepSlopedTurns = 1uLL << 26,
        largeSlopedTurns = 1uLL << 27,
        largeBankedSlopedTurns = 1uLL << 28,
        verticalLoops = 1uLL << 29,
        verticalBoosters = 1uLL << 30,
        diagonalBrakes = 1uLL << 31,
        slopedBrakes = 1uLL << 32,
        magneticBrakes = 1uLL << 33,
        diagonalVerticals = 1uLL << 34,
        bankedSBends = 1uLL << 35,
        blockBrakes = 1uLL << 36,
        diveLoops = 1uLL << 37,
        steepBankTransitions = 1uLL << 38,
        largeSteepSlopedTurns = 1uLL << 39,
        bankedBarrelRolls = 1uLL << 40,
        bankedInlineTwists = 1uLL << 41,
        bankedZeroGRolls = 1uLL << 42,
    };

    enum class TrackModel : int {
        trackAlt = 0,
        flat,
        bankSixth,
        bankThird,
        bankHalf,
        bankTwoThirds,
        bankFiveSixths,
        bank,
        base,
        specialBrake,
        specialBlockBrake,
        specialBooster,
        specialMagneticBrake,
        specialSteepToVertical,
        specialVerticalToSteep,
        specialVertical,
        specialVerticalTwist,
        specialBarrelRoll,
        specialHalfLoop,
        specialQuarterLoop,
        specialCorkscrew,
        specialZeroGRoll,
        specialLargeZeroGRoll,
    };

    enum class Offset : int {
        flat = 0,
        gentle = 1,
        steep = 2,
        bank = 3,
        gentleBank = 4,
        inverted = 5,
        diagonal = 6,
        diagonalBank = 7,
        diagonalGentle = 8,
        diagonalSteep = 9,
    };

    // Long enum of every loadable track section. Order matters - it is the
    // index into the `track_sections` array and also defines section IDs the
    // mask loader expects in its JSON keys (snake_case names below).
    enum class TrackSectionId : int {
        flat,
        flat_asymmetric,
        brake,
        brake_diag,
        brake_gentle,
        brake_gentle_diag,
        magnetic_brake,
        magnetic_brake_diag,
        magnetic_brake_gentle,
        magnetic_brake_gentle_diag,
        block_brake,
        block_brake_diag,
        booster,
        flat_to_gentle,
        gentle_to_flat,
        gentle,
        gentle_to_steep,
        steep_to_gentle,
        steep,
        steep_to_vertical,
        vertical_to_steep,
        vertical,
        small_turn_left,
        medium_turn_left,
        large_turn_left_to_diag,
        large_turn_right_to_diag,
        flat_diag,
        flat_to_gentle_diag,
        gentle_to_flat_diag,
        gentle_diag,
        gentle_to_steep_diag,
        steep_to_gentle_diag,
        steep_diag,
        flat_to_left_bank,
        flat_to_right_bank,
        left_bank_to_gentle,
        right_bank_to_gentle,
        gentle_to_left_bank,
        gentle_to_right_bank,
        left_bank,
        flat_to_left_bank_diag,
        flat_to_right_bank_diag,
        left_bank_to_gentle_diag,
        right_bank_to_gentle_diag,
        gentle_to_left_bank_diag,
        gentle_to_right_bank_diag,
        left_bank_diag,
        small_turn_left_bank,
        medium_turn_left_bank,
        large_turn_left_to_diag_bank,
        large_turn_right_to_diag_bank,
        small_turn_left_gentle,
        small_turn_right_gentle,
        medium_turn_left_gentle,
        medium_turn_right_gentle,
        very_small_turn_left_steep,
        very_small_turn_right_steep,
        vertical_twist_left,
        vertical_twist_right,
        gentle_to_gentle_left_bank,
        gentle_to_gentle_right_bank,
        gentle_left_bank_to_gentle,
        gentle_right_bank_to_gentle,
        left_bank_to_gentle_left_bank,
        right_bank_to_gentle_right_bank,
        gentle_left_bank_to_left_bank,
        gentle_right_bank_to_right_bank,
        gentle_left_bank,
        gentle_right_bank,
        flat_to_gentle_left_bank,
        flat_to_gentle_right_bank,
        gentle_left_bank_to_flat,
        gentle_right_bank_to_flat,
        small_turn_left_bank_gentle,
        small_turn_right_bank_gentle,
        medium_turn_left_bank_gentle,
        medium_turn_right_bank_gentle,
        s_bend_left,
        s_bend_right,
        s_bend_left_bank,
        s_bend_right_bank,
        small_helix_left,
        small_helix_right,
        medium_helix_left,
        medium_helix_right,
        barrel_roll_left,
        barrel_roll_right,
        inline_twist_left,
        inline_twist_right,
        half_loop,
        vertical_loop_left,
        vertical_loop_right,
        medium_half_loop_left,
        medium_half_loop_right,
        large_half_loop_left,
        large_half_loop_right,
        flat_to_steep,
        steep_to_flat,
        flat_to_steep_diag,
        steep_to_flat_diag,
        small_flat_to_steep,
        small_steep_to_flat,
        small_flat_to_steep_diag,
        small_steep_to_flat_diag,
        quarter_loop,
        corkscrew_left,
        corkscrew_right,
        large_corkscrew_left,
        large_corkscrew_right,
        zero_g_roll_left,
        zero_g_roll_right,
        large_zero_g_roll_left,
        large_zero_g_roll_right,
        dive_loop_45_left,
        dive_loop_45_right,
        dive_loop_90_left,
        dive_loop_90_right,
        small_turn_left_bank_to_gentle,
        small_turn_right_bank_to_gentle,
        launched_lift,
        large_turn_left_to_diag_gentle,
        large_turn_right_to_diag_gentle,
        large_turn_left_to_orthogonal_gentle,
        large_turn_right_to_orthogonal_gentle,
        gentle_to_gentle_left_bank_diag,
        gentle_to_gentle_right_bank_diag,
        gentle_left_bank_to_gentle_diag,
        gentle_right_bank_to_gentle_diag,
        left_bank_to_gentle_left_bank_diag,
        right_bank_to_gentle_right_bank_diag,
        gentle_left_bank_to_left_bank_diag,
        gentle_right_bank_to_right_bank_diag,
        gentle_left_bank_diag,
        gentle_right_bank_diag,
        flat_to_gentle_left_bank_diag,
        flat_to_gentle_right_bank_diag,
        gentle_left_bank_to_flat_diag,
        gentle_right_bank_to_flat_diag,
        large_turn_left_bank_to_diag_gentle,
        large_turn_right_bank_to_diag_gentle,
        large_turn_left_bank_to_orthogonal_gentle,
        large_turn_right_bank_to_orthogonal_gentle,
        steep_to_vertical_diag,
        vertical_to_steep_diag,
        vertical_diag,
        vertical_twist_left_to_diag,
        vertical_twist_right_to_diag,
        vertical_twist_left_to_orthogonal,
        vertical_twist_right_to_orthogonal,
        vertical_booster,
        gentle_left_bank_to_steep,
        gentle_right_bank_to_steep,
        steep_to_gentle_left_bank,
        steep_to_gentle_right_bank,
        gentle_left_bank_to_steep_diag,
        gentle_right_bank_to_steep_diag,
        steep_to_gentle_left_bank_diag,
        steep_to_gentle_right_bank_diag,
        small_turn_left_steep,
        small_turn_right_steep,
        large_turn_left_to_diag_steep,
        large_turn_right_to_diag_steep,
        large_turn_left_to_orthogonal_steep,
        large_turn_right_to_orthogonal_steep,
        inline_twist_left_bank,
        inline_twist_right_bank,
        barrel_roll_left_bank,
        barrel_roll_right_bank,
        zero_g_roll_left_bank,
        zero_g_roll_right_bank,
        count_,
    };

    template<class E>
    concept FlagEnum =
            std::is_same_v<E, TrackFlag> ||
            std::is_same_v<E, TrackTypeFlag> ||
            std::is_same_v<E, ViewFlag> ||
            std::is_same_v<E, TrackGroup>;

    template<FlagEnum E>
    constexpr E operator|(E a, E b) noexcept {
        using U = std::underlying_type_t<E>;
        return static_cast<E>(static_cast<U>(a) | static_cast<U>(b));
    }

    template<FlagEnum E>
    constexpr E operator&(E a, E b) noexcept {
        using U = std::underlying_type_t<E>;
        return static_cast<E>(static_cast<U>(a) & static_cast<U>(b));
    }

    template<FlagEnum E>
    constexpr E &operator|=(E &a, E b) noexcept {
        return a = a | b;
    }

    template<FlagEnum E>
    constexpr bool has_flag(E set, E flag) noexcept {
        using U = std::underlying_type_t<E>;
        return (static_cast<U>(set) & static_cast<U>(flag)) != 0;
    }

    // True if `set` carries any bits in common with `mask` (i.e. they overlap).
    template<FlagEnum E>
    constexpr bool any_of(E set, E mask) noexcept {
        using U = std::underlying_type_t<E>;
        return (static_cast<U>(set) & static_cast<U>(mask)) != 0;
    }
}