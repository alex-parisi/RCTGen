/// Sprites.cpp

#include "Sprites.hpp"

#include <array>
#include <cstdint>

namespace RCTGen {
    namespace {
        inline constexpr std::array<std::uint8_t, 72> kFlatPixels{
            1, 2, 3, 1, 2, 3, 3, 1, 2, 3, 1, 2, 2, 3, 1, 2, 3, 1,
            1, 2, 3, 1, 2, 3, 2, 3, 1, 2, 3, 1, 3, 1, 2, 3, 1, 2,
            1, 3, 2, 1, 3, 2, 2, 1, 3, 2, 1, 3, 3, 2, 1, 3, 2, 1,
            1, 3, 2, 1, 3, 2, 3, 2, 1, 3, 2, 1, 2, 1, 3, 2, 1, 3
        };

        inline constexpr std::array<std::uint8_t, 42> kGentlePixels{
            1, 1, 2, 2, 3, 3, 3, 3, 1, 1, 2, 2, 2, 2, 3, 3, 1, 1,
            1, 2, 3, 3, 2, 1,
            2, 2, 1, 1, 3, 3, 1, 1, 3, 3, 2, 2, 3, 3, 2, 2, 1, 1
        };

        inline constexpr std::array<std::uint8_t, 6> kFlatDiagChainPixels{1, 2, 3, 3, 2, 1};
    }

    // image_t::pixels is non-const because legacy renderer code can mutate it,
    // so we const_cast off the constexpr storage. These tables are read-only
    // at runtime; nothing writes through these pointers.
    std::array<image_t, 4> kFlatChain{
        {
            {3, 6, 0, -2, const_cast<std::uint8_t *>(kFlatPixels.data())},
            {3, 6, 0, -1, const_cast<std::uint8_t *>(kFlatPixels.data() + 18)},
            {3, 6, 0, 0, const_cast<std::uint8_t *>(kFlatPixels.data() + 36)},
            {3, 6, -1, 0, const_cast<std::uint8_t *>(kFlatPixels.data() + 54)},
        }
    };

    std::array<image_t, 4> kGentleChain{
        {
            {6, 3, -3, -1, const_cast<std::uint8_t *>(kGentlePixels.data())},
            {3, 1, 1, 0, const_cast<std::uint8_t *>(kGentlePixels.data() + 18)},
            {3, 1, 1, 0, const_cast<std::uint8_t *>(kGentlePixels.data() + 21)},
            {6, 3, 0, -1, const_cast<std::uint8_t *>(kGentlePixels.data() + 24)},
        }
    };

    std::array<image_t, 4> kFlatDiagChain{
        {
            {3, 1, -2, 0, const_cast<std::uint8_t *>(kFlatDiagChainPixels.data())},
            {1, 3, 0, -2, const_cast<std::uint8_t *>(kFlatDiagChainPixels.data())},
            {3, 1, -1, 0, const_cast<std::uint8_t *>(kFlatDiagChainPixels.data() + 3)},
            {1, 3, 0, -1, const_cast<std::uint8_t *>(kFlatDiagChainPixels.data() + 3)},
        }
    };
}