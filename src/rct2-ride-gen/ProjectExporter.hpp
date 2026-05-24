/// ProjectExporter.hpp

#pragma once

#include <expected>
#include <filesystem>
#include <string>

#include "../iso-render/renderer.h"
#include "Project.hpp"

namespace rctgen
{
    using ExportError = std::string;

    template <class T>
    using ExportResult = std::expected<T, ExportError>;

    // Render sprites + write object.json + bundle a .parkobj. If skip_render is
    // true, reuses images from a previously generated object.json instead.
    [[nodiscard]] ExportResult<void> export_project(
        Project& project,
        context_t& context,
        const std::filesystem::path& output_directory,
        bool skip_render);

    // Render each vehicle from a single test viewpoint and dump the PNGs to
    // ./test/. Used in --test mode.
    [[nodiscard]] ExportResult<void> export_project_test(
        Project& project,
        context_t& context);
}
