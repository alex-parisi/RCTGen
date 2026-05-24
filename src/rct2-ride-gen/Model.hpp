/// Model.hpp

#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "vectormath.h"

struct Model {
    static constexpr std::size_t kMaxFrames = 4;

    struct MeshFrame {
        std::int32_t mesh_index = -1;
        vector3_t position{};
        vector3_t orientation{};
    };

    std::vector<std::array<MeshFrame, kMaxFrames> > meshes;
};