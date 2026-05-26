/// Track.hpp

#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include <jansson.h>

#include "Mesh.hpp"
#include "Renderer.hpp"
#include "VectorMath.hpp"

#include "Constants.hpp"

namespace RCTGen {
    struct Mask {
        MaskOp op = MaskOp::none;
        std::uint32_t numRects = 0;
        std::int32_t xOffset = 0;
        std::int32_t yOffset = 0;
        Rect *rects = nullptr;
    };

    struct View {
        ViewFlag flags = ViewFlag::none;
        int numSprites = 0;
        Mask *masks = nullptr;
    };

    using ViewSet = std::array<View, 4>;

    struct TrackType {
        std::string suffix;
        TrackTypeFlag flags = TrackTypeFlag::none;
        TrackGroup groups = TrackGroup::none;
        std::int32_t liftOffset = 0;
        std::uint32_t modelsLoaded = 0;
        Mesh mesh{};
        Mesh meshTie{};
        Mesh tieMesh{};
        Mesh mask{};
        std::array<Mesh, kNumModels> models{};
        float length = 0.0f;
        float brakeLength = 0.0f;
        float tieLength = 0.0f;
        float pivot = 0.0f;
        float zOffset = 0.0f;
        float supportSpacing = 0.0f;
        std::array<float, 88> offsetTable{};
        std::array<ViewSet, kNumTrackSections> masks_;
    };

    struct TrackPoint {
        Vector3 position;
        Vector3 normal;
        Vector3 tangent;
        Vector3 binormal;
    };

    using CurveFn = TrackPoint (*)(float distance);

    struct TrackSection {
        const char *name;
        TrackFlag flags;
        CurveFn curve;
        float length;
        Image *chainPattern;
    };

    extern std::array<TrackSection, kNumTrackSections> kTrackSections;

    // Render every track section enabled by track_type->groups and append
    // each emitted sprite to `sprites` as a JSON object.
    int writeTrackType(
        Context *context,
        TrackType *trackType,
        json_t *sprites,
        std::span<const float, 88> offsetTable,
        const std::filesystem::path &baseDir,
        const std::filesystem::path &outputDir);
}