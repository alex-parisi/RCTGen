#include "sprites.h"

#include <array>
#include <cstdint>

namespace
{
inline constexpr std::array<std::uint8_t, 72> flat_pixels{
    1, 2, 3, 1, 2, 3, 3, 1, 2, 3, 1, 2, 2, 3, 1, 2, 3, 1,
    1, 2, 3, 1, 2, 3, 2, 3, 1, 2, 3, 1, 3, 1, 2, 3, 1, 2,
    1, 3, 2, 1, 3, 2, 2, 1, 3, 2, 1, 3, 3, 2, 1, 3, 2, 1,
    1, 3, 2, 1, 3, 2, 3, 2, 1, 3, 2, 1, 2, 1, 3, 2, 1, 3};

inline constexpr std::array<std::uint8_t, 42> gentle_pixels{
    1, 1, 2, 2, 3, 3, 3, 3, 1, 1, 2, 2, 2, 2, 3, 3, 1, 1,
    1, 2, 3, 3, 2, 1,
    2, 2, 1, 1, 3, 3, 1, 1, 3, 3, 2, 2, 3, 3, 2, 2, 1, 1};

inline constexpr std::array<std::uint8_t, 6> flat_diag_chain_pixels{1, 2, 3, 3, 2, 1};
} // namespace

image_t flat_chain[4] = {
    {3, 6, 0, -2, const_cast<std::uint8_t*>(flat_pixels.data())},
    {3, 6, 0, -1, const_cast<std::uint8_t*>(flat_pixels.data() + 18)},
    {3, 6, 0,  0, const_cast<std::uint8_t*>(flat_pixels.data() + 36)},
    {3, 6, -1, 0, const_cast<std::uint8_t*>(flat_pixels.data() + 54)},
};

image_t gentle_chain[4] = {
    {6, 3, -3, -1, const_cast<std::uint8_t*>(gentle_pixels.data())},
    {3, 1,  1,  0, const_cast<std::uint8_t*>(gentle_pixels.data() + 18)},
    {3, 1,  1,  0, const_cast<std::uint8_t*>(gentle_pixels.data() + 21)},
    {6, 3,  0, -1, const_cast<std::uint8_t*>(gentle_pixels.data() + 24)},
};

image_t flat_diag_chain[4] = {
    {3, 1, -2,  0, const_cast<std::uint8_t*>(flat_diag_chain_pixels.data())},
    {1, 3,  0, -2, const_cast<std::uint8_t*>(flat_diag_chain_pixels.data())},
    {3, 1, -1,  0, const_cast<std::uint8_t*>(flat_diag_chain_pixels.data() + 3)},
    {1, 3,  0, -1, const_cast<std::uint8_t*>(flat_diag_chain_pixels.data() + 3)},
};
