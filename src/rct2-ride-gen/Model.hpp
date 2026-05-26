/// Model.hpp

#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "VectorMath.hpp"

namespace RCTGen {
    struct Model {
        static constexpr std::size_t kMaxFrames = 4;

        struct MeshFrame {
            std::int32_t mesh_index = -1;
            Vector3 position{};
            Vector3 orientation{};
        };

        std::vector<std::array<MeshFrame, kMaxFrames> > meshes;
    };
}