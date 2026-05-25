/// SpriteRenderer.hpp

#pragma once

#include "../iso-render/image.h"
#include "../iso-render/renderer.h"
#include "Constants.hpp"

namespace RCTGen {

    using Context = context_t;
    using Image = image_t;

    // Number of sprite images emitted for the given sprite-flag set, plus
    // restraint-animation contribution from the vehicle flag set.
    [[nodiscard]] int countSprites(SpriteFlag spriteFlags, VehicleFlag vehicleFlags);

    // Render a vehicle (one frame: 0 for static, >0 for restraint animation
    // frames). Writes images into `out`. Returns the number of images written.
    int renderVehicleFrame(
        Context *context,
        SpriteFlag spriteFlags,
        int frame,
        Image *out);
}