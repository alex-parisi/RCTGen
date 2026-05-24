/// ProjectExporter.hpp

#pragma once

#include <expected>
#include <filesystem>
#include <string>

#include "renderer.h"
#include "Project.hpp"

namespace RCTGen {
    using ExportError = std::string;

    template<class T>
    using ExportResult = std::expected<T, ExportError>;

    // Render sprites + write object.json + bundle a .parkobj. If skip_render is
    // true, reuses images from a previously generated object.json instead.
    [[nodiscard]] ExportResult<void> exportProject(
        Project &project,
        context_t &context,
        const std::filesystem::path &outputDirectory,
        bool skipRender);

    // Render each vehicle from a single test viewpoint and dump the PNGs to
    // ./test/. Used in --test mode.
    [[nodiscard]] ExportResult<void> exportProjectTest(
        Project & project,
        context_t & context);
}