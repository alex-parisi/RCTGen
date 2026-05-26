/// Vehicle.hpp

#pragma once

#include <cstdint>
#include <vector>

#include "Model.hpp"

namespace RCTGen {
    struct Vehicle {
        Model model;
        std::uint32_t flags = 0;
        std::uint32_t mass = 0;
        std::uint32_t num_sprites = 0;
        std::uint32_t draw_order = 0;
        std::uint32_t num_riders = 0;
        float spacing = 0.0f;
        std::vector<Model> riders;
    };
}