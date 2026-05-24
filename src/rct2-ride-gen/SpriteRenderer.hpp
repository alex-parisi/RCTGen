/// SpriteRenderer.hpp

#pragma once

#include "../iso-render/image.h"
#include "../iso-render/renderer.h"
#include "Constants.hpp"

namespace rctgen
{
    // Number of sprite images emitted for the given sprite-flag set, plus
    // restraint-animation contribution from the vehicle flag set.
    [[nodiscard]] int count_sprites(SpriteFlag sprite_flags, VehicleFlag vehicle_flags);

    // Render a vehicle (one frame: 0 for static, >0 for restraint animation
    // frames). Writes images into `out`. Returns the number of images written.
    int render_vehicle_frame(
        context_t* context,
        SpriteFlag sprite_flags,
        int frame,
        image_t* out);
}
