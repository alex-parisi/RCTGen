/// Mask.hpp

#pragma once

#include <array>
#include <expected>
#include <filesystem>
#include <string>

#include "Constants.hpp"
#include "Track.hpp"

namespace RCTGen {
    [[nodiscard]] std::expected<void, std::string> loadMasks(
        const std::filesystem::path &filename,
        std::array<ViewSet, kNumTrackSections> &views);
}