/// main.cpp

#include <array>
#include <cstdint>
#include <cstring>
#include <expected>
#include <filesystem>
#include <numbers>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <jansson.h>

#include "Image.hpp"
#include "Mesh.hpp"
#include "Renderer.hpp"
#include "VectorMath.hpp"

#include "Constants.hpp"
#include "Json.hpp"
#include "Logging.hpp"
#include "Mask.hpp"
#include "Track.hpp"

namespace fs = std::filesystem;
using namespace RCTGen;

namespace {
    using LoadError = std::string;
    template<class T>
    using LoadResult = std::expected<T, LoadError>;

    // Returns 0 on success, 1 on error, 2 if the mesh entry is absent (caller
    // treats absent as "not loaded").
    int loadModelFile(Mesh *out, const Json *parent, const char *name) {
        const Json *mesh = json_object_get(parent, name);
        if (mesh == nullptr) return 2;
        if (!json_is_string(mesh)) {
            printMsg("Error: Property \"{}\" not found or is not an object", name);
            return 1;
        }
        const char *path = json_string_value(mesh);
        if (mesh_load_transform(out, path, rotate_y(-0.5f * std::numbers::pi_v<float>)) != 0) {
            printMsg("Failed to load model from file \"{}\"", path);
            return 1;
        }
        return 0;
    }

    LoadResult<TrackGroup> loadGroups(const Json *json) {
        struct Entry {
            std::string_view key;
            TrackGroup value;
        };
        static constexpr std::array<Entry, 42> kMap = {
            {
                {"flat", TrackGroup::flat},
                {"brakes", TrackGroup::brakes},
                {"block_brakes", TrackGroup::blockBrakes},
                {"diagonal_brakes", TrackGroup::diagonalBrakes},
                {"sloped_brakes", TrackGroup::slopedBrakes},
                {"magnetic_brakes", TrackGroup::magneticBrakes},
                {"turns", TrackGroup::turns},
                {"gentle_slopes", TrackGroup::gentleSlopes},
                {"steep_slopes", TrackGroup::steepSlopes},
                {"vertical_slopes", TrackGroup::verticalSlopes},
                {"diagonals", TrackGroup::diagonals},
                {"sloped_turns", TrackGroup::slopedTurns | TrackGroup::steepSlopedTurns},
                {"gentle_sloped_turns", TrackGroup::slopedTurns},
                {"banked_turns", TrackGroup::bankedTurns},
                {"banked_sloped_turns", TrackGroup::bankedSlopedTurns},
                {"large_sloped_turns", TrackGroup::largeSlopedTurns},
                {"large_banked_sloped_turns", TrackGroup::largeBankedSlopedTurns},
                {"s_bends", TrackGroup::sBends},
                {"banked_s_bends", TrackGroup::bankedSBends},
                {"helices", TrackGroup::helices},
                {"small_slope_transitions", TrackGroup::smallSlopeTransitions},
                {"large_slope_transitions", TrackGroup::largeSlopeTransitions},
                {"barrel_rolls", TrackGroup::barrelRolls},
                {"inline_twists", TrackGroup::inlineTwists},
                {"quarter_loops", TrackGroup::quarterLoops},
                {"corkscrews", TrackGroup::corkscrews},
                {"large_corkscrews", TrackGroup::largeCorkscrews},
                {"half_loops", TrackGroup::halfLoops},
                {"vertical_loops", TrackGroup::verticalLoops},
                {"medium_half_loops", TrackGroup::mediumHalfLoops},
                {"large_half_loops", TrackGroup::largeHalfLoops},
                {"zero_g_rolls", TrackGroup::zeroGRolls},
                {"dive_loops", TrackGroup::diveLoops},
                {"boosters", TrackGroup::boosters},
                {"launched_lifts", TrackGroup::launchedLifts},
                {"turn_bank_transitions", TrackGroup::turnBankTransitions},
                {"steep_bank_transitions", TrackGroup::steepBankTransitions},
                {"large_steep_sloped_turns", TrackGroup::largeSteepSlopedTurns},
                {"banked_barrel_rolls", TrackGroup::bankedBarrelRolls},
                {"banked_inline_twists", TrackGroup::bankedInlineTwists},
                {"banked_zero_g_rolls", TrackGroup::bankedZeroGRolls},
                {"vertical_boosters", TrackGroup::verticalBoosters},
            }
        };

        TrackGroup groups = TrackGroup::none;
        for (std::size_t i = 0; i < json_array_size(json); i++) {
            const Json *name_json = json_array_get(json, i);
            if (!json_is_string(name_json)) {
                return std::unexpected(std::string("Array \"sections\" contains non-string value"));
            }
            const std::string_view name = json_string_value(name_json);
            bool matched = false;
            for (const auto &e: kMap) {
                if (e.key == name) {
                    groups |= e.value;
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                return std::unexpected(std::format("Unrecognized section group \"{}\"", name));
            }
        }
        return groups;
    }

    LoadResult<void> loadOffsets(const Json *json, std::array<float, 88> &offsets) {
        static constexpr std::array<std::string_view, 10> kRows = {
            "flat", "gentle", "steep", "flat_banked", "gentle_banked",
            "inverted", "diagonal", "diagonal_banked", "diagonal_gentle", "diagonal_steep"
        };
        offsets.fill(0.0f);
        for (std::size_t i = 0; i < kRows.size(); i++) {
            const Json *row = json_object_get(json, std::string(kRows[i]).c_str());
            if (row == nullptr) continue;
            if (!json_is_array(row) || json_array_size(row) != 8) {
                return std::unexpected(std::format(
                    "Property \"{}\" is not an array of length 8", kRows[i]));
            }
            for (std::size_t j = 0; j < 8; j++) {
                const Json *value = json_array_get(row, j);
                if (!json_is_number(value)) {
                    return std::unexpected(std::format(
                        "Array \"{}\" contains non numeric value", kRows[i]));
                }
                offsets[8 * i + j] = static_cast<float>(json_number_value(value));
            }
        }
        return {};
    }

    LoadResult<void> loadOptionalFloat(const Json *json, const char *name,
                                       float &out, float mult, bool required) {
        const Json *v = json_object_get(json, name);
        if (v != nullptr) {
            if (!json_is_number(v)) {
                return std::unexpected(std::format("Property \"{}\" is not a number", name));
            }
            out = static_cast<float>(mult * json_number_value(v));
        } else if (required) {
            return std::unexpected(std::format("Property \"{}\" not found", name));
        }
        return {};
    }

    LoadResult<TrackTypeFlag> loadTrackFlags(const Json *flagsJson) {
        TrackTypeFlag flags = TrackTypeFlag::none;
        if (!json_is_array(flagsJson)) {
            return std::unexpected(std::string("Property \"flags\" is not an array"));
        }
        for (std::size_t i = 0; i < json_array_size(flagsJson); i++) {
            const Json *fn = json_array_get(flagsJson, i);
            if (!json_is_string(fn)) {
                return std::unexpected(std::string("Array \"flags\" contains non-string value"));
            }
            const std::string_view name = json_string_value(fn);
            if (name == "has_lift") flags |= TrackTypeFlag::hasLift;
            else if (name == "has_supports") flags |= TrackTypeFlag::hasSupports;
            else if (name == "separate_tie") flags |= TrackTypeFlag::separateTie;
            else if (name == "tie_at_boundary") flags |= TrackTypeFlag::separateTie | TrackTypeFlag::tieAtBoundary;
            else if (name == "special_end_offsets") flags |= TrackTypeFlag::specialOffsets;
            else return std::unexpected(std::format("Unrecognized flag \"{}\"", name));
        }
        return flags;
    }

    LoadResult<void> loadTrackType(TrackType &trackType, Json *json, bool preloaded) {
        if (Json *flags = json_object_get(json, "flags"); flags != nullptr) {
            auto f = loadTrackFlags(flags);
            if (!f) return std::unexpected(f.error());
            trackType.flags = *f;
        }

        if (Json *groups = json_object_get(json, "sections"); groups != nullptr) {
            if (!json_is_array(groups)) {
                return std::unexpected(std::string("Property \"sections\" is not an array"));
            }
            auto g = loadGroups(groups);
            if (!g) return std::unexpected(g.error());
            trackType.groups = *g;
        }

        if (Json *masksJson = json_object_get(json, "masks"); masksJson != nullptr) {
            if (!json_is_string(masksJson)) {
                return std::unexpected(std::string("Property \"masks\" is not a string"));
            }
            if (auto r = loadMasks(json_string_value(masksJson), trackType.masks_); !r)
                return std::unexpected(r.error());
        } else if (!preloaded) {
            return std::unexpected(std::string("Property \"masks\" not found"));
        }

        if (Json *name = json_object_get(json, "name"); name != nullptr) {
            if (!json_is_string(name)) {
                return std::unexpected(std::string("Property \"name\" is not a string"));
            }
            trackType.suffix = std::string("_") + json_string_value(name);
        } else if (!preloaded) {
            trackType.suffix.clear();
        }

        if (has_flag(trackType.flags, TrackTypeFlag::hasLift)) {
            if (Json *off = json_object_get(json, "lift_offset"); off != nullptr) {
                auto v = readInt(off, "lift_offset");
                if (!v) return std::unexpected(v.error());
                trackType.liftOffset = static_cast<std::int32_t>(*v);
            } else if (!preloaded) {
                trackType.liftOffset = 13;
            }
        }

        if (auto r = loadOptionalFloat(json, "length", trackType.length, kTileSize, !preloaded); !r) return r;
        if (auto r = loadOptionalFloat(json, "brake_length", trackType.brakeLength, kTileSize, false); !r) return r;
        if (!preloaded && json_object_get(json, "brake_length") == nullptr) trackType.brakeLength = kTileSize;

        if (has_flag(trackType.flags, TrackTypeFlag::tieAtBoundary)) {
            if (auto r = loadOptionalFloat(json, "tie_length", trackType.tieLength, kTileSize, !preloaded); !r) return
                    r;
        }

        if (auto r = loadOptionalFloat(json, "z_offset", trackType.zOffset, 1.0f, !preloaded); !r) return r;
        if (auto r = loadOptionalFloat(json, "support_spacing", trackType.supportSpacing, kTileSize, false); !r) return
                r;
        if (!preloaded && json_object_get(json, "support_spacing") == nullptr) trackType.supportSpacing = kTileSize;
        if (auto r = loadOptionalFloat(json, "pivot", trackType.pivot, kTileSize, false); !r) return r;
        if (!preloaded && json_object_get(json, "pivot") == nullptr) trackType.pivot = 0.0f;

        Json *models = json_object_get(json, "models");
        if (models == nullptr || !json_is_object(models)) {
            return std::unexpected(std::string(
                "Property \"models\" not found or is not an object"));
        }

        if (loadModelFile(&trackType.mesh, models, "track") != 0) {
            return std::unexpected(std::string("Track mesh not found"));
        }
        if (loadModelFile(&trackType.mask, models, "mask") != 0) {
            mesh_destroy(&trackType.mesh);
            return std::unexpected(std::string("Mask mesh not found"));
        }

        if (has_flag(trackType.flags, TrackTypeFlag::separateTie)) {
            if (loadModelFile(&trackType.tieMesh, models, "tie") != 0) {
                mesh_destroy(&trackType.mesh);
                mesh_destroy(&trackType.mask);
                return std::unexpected(std::string("separate tie mesh not found"));
            }
            if (has_flag(trackType.flags, TrackTypeFlag::tieAtBoundary)) {
                if (loadModelFile(&trackType.meshTie, models, "track_tie") != 0) {
                    mesh_destroy(&trackType.mesh);
                    mesh_destroy(&trackType.mask);
                    mesh_destroy(&trackType.tieMesh);
                    return std::unexpected(std::string("track_tie mesh not found"));
                }
            }
        }

        static constexpr std::array<std::string_view, kNumModels> kSupportNames = {
            "track_alt",
            "support_flat",
            "support_bank_sixth",
            "support_bank_third",
            "support_bank_half",
            "support_bank_two_thirds",
            "support_bank_five_sixths",
            "support_bank",
            "support_base",
            "brake",
            "block_brake",
            "booster",
            "magnetic_brake",
            "support_steep_to_vertical",
            "support_vertical_to_steep",
            "support_vertical",
            "support_vertical_twist",
            "support_barrel_roll",
            "support_half_loop",
            "support_quarter_loop",
            "support_corkscrew",
            "support_zero_g_roll",
            "support_large_zero_g_roll",
        };

        trackType.modelsLoaded = 0;
        for (std::size_t i = 0; i < kNumModels; i++) {
            int result = loadModelFile(&trackType.models[i], models, std::string(kSupportNames[i]).c_str());
            if (result == 0) trackType.modelsLoaded |= 1u << i;
            else if (result == 1) {
                mesh_destroy(&trackType.mesh);
                mesh_destroy(&trackType.mask);
                for (std::size_t j = 0; j < i; j++) mesh_destroy(&trackType.models[j]);
                return std::unexpected(std::format("Failed to load model {}", kSupportNames[i]));
            }
        }
        return {};
    }

    LoadResult<std::vector<Light> > loadLightsArray(Json *array) {
        if (!json_is_array(array)) {
            return std::unexpected(std::string("Property \"lights\" is not an array"));
        }
        std::vector<Light> lights;
        lights.reserve(json_array_size(array));
        for (std::size_t i = 0; i < json_array_size(array); i++) {
            Json *light = json_array_get(array, i);
            if (!json_is_object(light)) {
                printMsg("Warning: Light array contains an element which is not an object - ignoring");
                continue;
            }
            Light out{};
            auto type = readString(json_object_get(light, "type"), "type");
            if (!type) return std::unexpected(type.error());
            if (*type == "diffuse") out.type = LIGHT_DIFFUSE;
            else if (*type == "specular") out.type = LIGHT_SPECULAR;
            else return std::unexpected(std::format("Unrecognized light type \"{}\"", *type));

            auto shadow = readBool(json_object_get(light, "shadow"), "shadow");
            if (!shadow) return std::unexpected(shadow.error());
            out.shadow = *shadow ? 1 : 0;

            auto dir = readVector3(json_object_get(light, "direction"));
            if (!dir) return std::unexpected(dir.error());
            out.direction = (*dir).normalized();

            auto strength = readNumber(json_object_get(light, "strength"), "strength");
            if (!strength) return std::unexpected(strength.error());
            out.intensity = static_cast<float>(*strength);

            lights.push_back(out);
        }
        return lights;
    }

    std::vector<Light> defaultLights() {
        return {
            {LIGHT_DIFFUSE, 0, vector3(0.0f, -1.0f, 0.0f).normalized(), 0.25f},
            {LIGHT_DIFFUSE, 0, vector3(1.0f, 0.3f, 0.0f).normalized(), 0.32f},
            {LIGHT_SPECULAR, 0, vector3(1, 1, -1).normalized(), 1.0f},
            {LIGHT_DIFFUSE, 0, vector3(1, 0.65f, -1).normalized(), 0.8f},
            {LIGHT_DIFFUSE, 0, vector3(0.0f, 1.0f, 0.0f), 0.174f},
            {LIGHT_DIFFUSE, 0, vector3(-1.0f, 0.0f, 0.0f).normalized(), 0.15f},
            {LIGHT_DIFFUSE, 0, vector3(0.0f, 1.0f, 1.0f).normalized(), 0.2f},
            {LIGHT_DIFFUSE, 0, vector3(0.65f, 0.816f, -0.65f).normalized(), 0.25f},
            {LIGHT_DIFFUSE, 0, vector3(-1.0f, 0.0f, -1.0f).normalized(), 0.25f},
        };
    }

    struct CliArgs {
        fs::path inputFile;
    };

    std::expected<CliArgs, std::string> parseCli(std::span<char *const> argv) {
        if (argv.size() != 2) {
            return std::unexpected(std::string("Usage: maketrack <file>"));
        }
        return CliArgs{argv[1]};
    }
}

int main(int argc, char **argv) {
    auto cli = parseCli(std::span<char *const>(argv, static_cast<std::size_t>(argc)));
    if (!cli) {
        printMsg("{}", cli.error());
        return 1;
    }

    auto root = loadFile(cli->inputFile);
    if (!root) {
        printMsg("Error: {}", root.error());
        return 1;
    }
    Json *json = root->get();

    auto baseDir = readString(json_object_get(json, "base_directory"), "base_directory");
    if (!baseDir) printMsg("Error: {}", baseDir.error());
    auto spriteDir = readString(json_object_get(json, "sprite_directory"), "sprite_directory");
    if (!spriteDir) printMsg("Error: {}", spriteDir.error());
    auto spritefileIn = readString(json_object_get(json, "spritefile_in"), "spritefile_in");
    if (!spritefileIn) printMsg("Error: {}", spritefileIn.error());
    auto spritefileOut = readString(json_object_get(json, "spritefile_out"), "spritefile_out");
    if (!spritefileOut) printMsg("Error: {}", spritefileOut.error());

    std::vector<Light> lights = defaultLights();
    if (Json *lightArray = json_object_get(json, "lights"); lightArray != nullptr) {
        auto loaded = loadLightsArray(lightArray);
        if (!loaded) {
            printMsg("Error: {}", loaded.error());
            return 1;
        }
        lights = std::move(*loaded);
    }

    std::uint32_t dither = 1;
    if (Json *ditherJson = json_object_get(json, "dither"); ditherJson != nullptr) {
        if (!json_is_boolean(ditherJson)) {
            printMsg("Error: Property \"dither\" is not a boolean");
            return 1;
        }
        dither = json_is_true(ditherJson);
    }

    Json *tracksJson = json_object_get(json, "tracks");
    if (tracksJson == nullptr || !json_is_array(tracksJson)) {
        printMsg("Error: Property \"tracks\" not found or is not an array");
        return 1;
    }

    std::array < float, 88 > offsetTable{};
    if (Json *offsets = json_object_get(json, "offsets"); offsets != nullptr) {
        if (!json_is_object(offsets)) {
            printMsg("Error: Property \"offsets\" is not an object");
            return 1;
        }
        if (auto r = loadOffsets(offsets, offsetTable); !r) {
            printMsg("Error: {}", r.error());
            return 1;
        }
    }

    fs::path basePath = baseDir ? *baseDir : "";
    fs::path inPath = basePath / (spritefileIn ? *spritefileIn : std::string{});
    auto spritesRef = loadFile(inPath);
    if (!spritesRef) {
        printMsg("Error: {}", spritesRef.error());
        return 1;
    }
    Json *sprites = spritesRef->get();

    Context context;
    context_init(&context, lights.data(), static_cast<std::uint32_t>(lights.size()),
                 dither, palette_rct2(), kTileSize);

    TrackType trackType;
    for (std::size_t i = 0; i < json_array_size(tracksJson); i++) {
        Json *track = json_array_get(tracksJson, i);
        if (!json_is_object(track)) continue;
        if (auto r = loadTrackType(trackType, track, i != 0); !r) {
            printMsg("Error loading track: {}", r.error());
            context_destroy(&context);
            return 1;
        }
        writeTrackType(&context, &trackType, sprites,
                       std::span < const float, 88 >
        {
            offsetTable
        }
        ,
        basePath, spriteDir ? *spriteDir : ""
        )
        ;
    }

    fs::path outPath = basePath / (spritefileOut ? *spritefileOut : std::string{});
    dumpFile(sprites, outPath, JSON_INDENT(4));
    context_destroy(&context);
    return 0;
}