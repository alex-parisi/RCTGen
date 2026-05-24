/// Constants.hpp

#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <utility>

namespace RCTGen {
    inline constexpr float kTileSize = 3.3f;

    enum class SpriteFlag : std::uint32_t {
        flatSlope = 1u << 0,
        gentleSlope = 1u << 1,
        steepSlope = 1u << 2,
        verticalSlope = 1u << 3,
        diagonalSlope = 1u << 4,
        banking = 1u << 5,
        inlineTwist = 1u << 6,
        slopeBankTransition = 1u << 7,
        diagonalBankTransition = 1u << 8,
        slopedBankTransition = 1u << 9,
        slopedBankedTurn = 1u << 10,
        bankedSlopeTransition = 1u << 11,
        corkscrew = 1u << 12,
        zeroGRoll = 1u << 13,
        diagonalSlopedBankTransition = 1u << 14,
        diveLoop = 1u << 15,
    };

    enum class RideFlag : std::uint32_t {
        noCollisionCrashes = 1u << 0,
        riderControlsSpeed = 1u << 1,
    };

    enum class VehicleFlag : std::uint32_t {
        secondaryRemap = 1u << 0,
        tertiaryRemap = 1u << 1,
        ridersScream = 1u << 2,
        restraintAnimation = 1u << 3,
    };

    enum class RunningSound : std::uint8_t {
        woodenOld = 1,
        woodenModern = 54,
        steel = 2,
        steelSmooth = 57,
        waterslide = 32,
        train = 31,
        engine = 21,
        none = 255,
    };

    enum class SecondarySound : std::uint8_t {
        screams1 = 0,
        screams2 = 1,
        screams3 = 2,
        whistle = 3,
        bell = 4,
        none = 255,
    };

    enum class CarIndex : std::uint8_t {
        defaultVal = 0,
        front = 1,
        second = 2,
        rear = 3,
        third = 4,
    };

    enum class Category : std::uint8_t {
        transportRide = 0,
        gentleRide = 1,
        rollercoaster = 2,
        thrillRide = 3,
        waterRide = 4,
        shop = 5,
    };

    template<class E>
    concept FlagEnum =
            std::is_same_v<E, SpriteFlag> ||
            std::is_same_v<E, RideFlag> ||
            std::is_same_v<E, VehicleFlag>;

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

    inline constexpr std::array<std::string_view, 16> kSpriteGroupNames = {
        "flat", "gentle_slopes", "steep_slopes", "vertical_slopes",
        "diagonals", "banked_turns", "inline_twists", "slope_bank_transition",
        "diagonal_bank_transition", "sloped_bank_transition", "banked_sloped_turns",
        "banked_slope_transition", "corkscrews", "zero_g_rolls",
        "diagonal_sloped_bank_transition", "dive_loops",
    };

    inline constexpr std::array<std::string_view, 2> kRideFlagNames = {
        "no_collision_crashes", "rider_controls_speed",
    };

    inline constexpr std::array<std::string_view, 4> kVehicleFlagNames = {
        "secondary_remap", "tertiary_remap", "riders_scream", "restraint_animation",
    };

    inline constexpr std::array<std::string_view, 6> kRunningSoundNames = {
        "wooden_old", "wooden", "steel", "steel_smooth", "train", "engine",
    };

    inline constexpr std::array<std::string_view, 4> kSecondarySoundNames = {
        "scream1", "scream2", "scream3", "bell",
    };

    inline constexpr std::array kRunningSoundValues = {
        std::to_underlying(RunningSound::woodenOld),
        std::to_underlying(RunningSound::woodenModern),
        std::to_underlying(RunningSound::steel),
        std::to_underlying(RunningSound::steelSmooth),
        std::to_underlying(RunningSound::waterslide),
        std::to_underlying(RunningSound::train),
    };

    inline constexpr std::array<std::string_view, 32> kColorNames = {
        "black", "grey", "white", "dark_purple", "light_purple", "bright_purple",
        "dark_blue", "light_blue", "icy_blue", "teal", "aquamarine",
        "saturated_green", "dark_green", "moss_green", "bright_green",
        "olive_green", "dark_olive_green", "bright_yellow", "yellow",
        "dark_yellow", "light_orange", "dark_orange", "light_brown",
        "saturated_brown", "dark_brown", "salmon_pink", "bordeaux_red",
        "saturated_red", "bright_red", "dark_pink", "bright_pink", "light_pink",
    };

    inline constexpr std::array<std::string_view, 4> kCategoryNames = {
        "transport", "gentle", "water", "rollercoaster",
    };
}