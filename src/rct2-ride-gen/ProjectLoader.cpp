/// ProjectLoader.cpp

#include "ProjectLoader.hpp"

#include <cassert>
#include <cstdio>
#include <format>
#include <span>
#include <utility>

#include "../iso-render/image.h"
#include "../iso-render/model.h"
#include "Constants.hpp"
#include "Json.hpp"
#include "Logging.hpp"
#include "SpriteRenderer.hpp"
#include "Vehicle.hpp"

namespace RCTGen
{
    namespace
    {
        constexpr int kRestraintFrameCount = 4;

        [[nodiscard]] LoadResult<void> loadMeshPath(mesh_t& mesh, const json_t* json)
        {
            if (!json_is_string(json))
            {
                return std::unexpected(std::string("Mesh path is not a string"));
            }
            if (const char* path = json_string_value(json); mesh_load(&mesh, path) != 0)
            {
                return std::unexpected(std::format("Failed to load model \"{}\"", path));
            }
            return {};
        }

        [[nodiscard]] LoadResult<void> loadConfiguration(
            std::array<std::uint8_t, 5>& config, const json_t* json)
        {
            if (json == nullptr || !json_is_object(json))
            {
                return std::unexpected(std::string(
                    "Property \"configuration\" not found or is not an object"));
            }
            config.fill(0xFF);

            auto get = [&](const char* key) -> JsonResult<std::int64_t> {
                return readInt(json_object_get(json, key), key);
            };

            auto def = get("default");
            if (!def)
                return std::unexpected(def.error());
            config[std::to_underlying(CarIndex::defaultVal)] = static_cast<std::uint8_t>(*def);

            for (auto [key, idx] : std::array<std::pair<const char*, CarIndex>, 4>{{
                {"front",  CarIndex::front},
                {"second", CarIndex::second},
                {"third",  CarIndex::third},
                {"rear",   CarIndex::rear},
            }})
            {
                const json_t* v = json_object_get(json, key);
                if (v == nullptr)
                    continue;
                if (!json_is_integer(v))
                {
                    return std::unexpected(std::format(
                        "Property \"{}\" is not an integer", key));
                }
                config[std::to_underlying(idx)] = static_cast<std::uint8_t>(json_integer_value(v));
            }
            return {};
        }

        [[nodiscard]] LoadResult<void> loadColors(Project& project, const json_t* json)
        {
            if (json == nullptr || !json_is_array(json))
            {
                return std::unexpected(std::string(
                    "Property \"default_colors\" not found or is not an array"));
            }
            project.colors.assign(json_array_size(json), {});
            for (std::size_t i = 0; i < project.colors.size(); i++)
            {
                const json_t* colors_json = json_array_get(json, i);
                if (colors_json == nullptr || !json_is_array(colors_json))
                {
                    return std::unexpected(std::string(
                        "Property \"default_colors\" contains an element which is not an array"));
                }
                for (std::size_t j = 0; j < json_array_size(colors_json) && j < 3; j++)
                {
                    auto color = readEnumIndex(
                        json_array_get(colors_json, j),
                        std::span(kColorNames),
                        "default_colors",
                        "color");
                    if (!color)
                        return std::unexpected(color.error());
                    project.colors[i][j] = static_cast<std::uint8_t>(*color);
                }
            }
            return {};
        }

        [[nodiscard]] LoadResult<std::string> readOptionalString(
            const json_t* parent, std::string_view key)
        {
            const json_t* v = json_object_get(parent, key.data()); // key is null-terminated literal
            if (v == nullptr)
                return std::string{};
            if (!json_is_string(v))
            {
                return std::unexpected(std::format(
                    "Property \"{}\" is not a string", key));
            }
            return std::string(json_string_value(v));
        }

        [[nodiscard]] LoadResult<std::string> readRequiredString(
            const json_t* parent, const std::string_view key)
        {
            const json_t* v = json_object_get(parent, key.data());
            return readString(v, key);
        }

        [[nodiscard]] LoadResult<void> loadMeshes(Project& project, const json_t* meshes)
        {
            if (meshes == nullptr || !json_is_array(meshes))
            {
                return std::unexpected(std::string(
                    "Property \"meshes\" does not exist or is not an array"));
            }
            project.meshes.resize(json_array_size(meshes));
            for (std::size_t i = 0; i < project.meshes.size(); i++)
            {
                const json_t* mesh = json_array_get(meshes, i);
                assert(mesh != nullptr);
                if (auto r = loadMeshPath(project.meshes[i], mesh); !r)
                {
                    for (std::size_t j = 0; j < i; j++)
                        mesh_destroy(&project.meshes[j]);
                    return std::unexpected(r.error());
                }
            }
            return {};
        }

        [[nodiscard]] LoadResult<void> loadVehicle(
            Vehicle& v, json_t* vehicle, const Project& project)
        {
            if (!json_is_object(vehicle))
            {
                return std::unexpected(std::string(
                    "Vehicle array contains an element which is not an object"));
            }

            auto spacing = readNumber(json_object_get(vehicle, "spacing"), "spacing");
            if (!spacing)
                return std::unexpected(spacing.error());
            v.spacing = static_cast<float>(*spacing);

            auto mass = readUint32(json_object_get(vehicle, "mass"), "mass");
            if (!mass)
                return std::unexpected(mass.error());
            v.mass = *mass;

            auto draw_order = readUint32(json_object_get(vehicle, "draw_order"), "draw_order");
            if (!draw_order)
                return std::unexpected(draw_order.error());
            v.draw_order = *draw_order;

            auto flags = readFlagBits(
                json_object_get(vehicle, "flags"),
                std::span(kVehicleFlagNames),
                "flags", "flag");
            if (!flags)
                return std::unexpected(flags.error());
            v.flags = *flags;

            const int num_frames =
                has_flag(static_cast<VehicleFlag>(v.flags), VehicleFlag::restraintAnimation)
                    ? kRestraintFrameCount : 1;
            const int num_meshes_int = static_cast<int>(project.meshes.size());

            if (auto r = loadModel(v.model, json_object_get(vehicle, "model"),
                                    num_meshes_int, num_frames); !r)
            {
                return std::unexpected(r.error());
            }

            if (json_t* riders = json_object_get(vehicle, "riders"); riders != nullptr && json_is_array(riders))
            {
                auto num_riders = readUint32(
                    json_object_get(vehicle, "capacity"), "capacity");
                if (!num_riders) return std::unexpected(num_riders.error());
                v.num_riders = *num_riders;

                v.riders.resize(json_array_size(riders));
                for (std::size_t i = 0; i < v.riders.size(); i++)
                {
                    if (auto r = loadModel(v.riders[i], json_array_get(riders, i),
                                            num_meshes_int, num_frames); !r)
                    {
                        return std::unexpected(r.error());
                    }
                }
            }
            else if (riders != nullptr && !json_is_array(riders))
            {
                return std::unexpected(std::string("Property \"riders\" is not an array"));
            }

            return {};
        }
    } // namespace

    LoadResult<void> loadModel(Model& model, json_t* json, int numMeshes, int numFrames)
    {
        if (json == nullptr)
        {
            return std::unexpected(std::string("Property \"model\" not found"));
        }

        auto arr_ref = asArrayOrWrap(json);
        if (!arr_ref)
            return std::unexpected(arr_ref.error());
        json_t* arr = arr_ref->get();

        model.meshes.resize(json_array_size(arr));
        for (std::size_t i = 0; i < model.meshes.size(); i++)
        {
            auto& mesh_frames = model.meshes[i];
            json_t* elem = json_array_get(arr, i);
            if (!json_is_object(elem))
            {
                return std::unexpected(std::string("Property \"model\" is not an object"));
            }

            // mesh_index
            json_t* mesh = json_object_get(elem, "mesh_index");
            if (mesh == nullptr)
            {
                return std::unexpected(std::string("Property \"mesh_index\" not found"));
            }
            auto mesh_arr_ref = asArrayOrWrap(mesh);
            if (!mesh_arr_ref)
                return std::unexpected(mesh_arr_ref.error());
            json_t* mesh_arr = mesh_arr_ref->get();
            const std::size_t mesh_count = json_array_size(mesh_arr);
            if (mesh_count != 1 && mesh_count != static_cast<std::size_t>(numFrames))
            {
                return std::unexpected(std::format(
                    "Number of elements in \"mesh_index\" ({}) does not match number of frames ({})",
                    mesh_count, numFrames));
            }
            for (std::size_t j = 0; j < mesh_count; j++)
            {
                auto idx = readInt(json_array_get(mesh_arr, j), "mesh_index");
                if (!idx)
                    return std::unexpected(idx.error());
                mesh_frames[j].mesh_index = static_cast<std::int32_t>(*idx);
                if (mesh_frames[j].mesh_index >= numMeshes || mesh_frames[j].mesh_index < -1)
                {
                    return std::unexpected(std::format(
                        "Mesh index {} is out of bounds", mesh_frames[j].mesh_index));
                }
            }
            if (mesh_count < static_cast<std::size_t>(numFrames))
            {
                for (int j = 0; j < numFrames; j++)
                {
                    mesh_frames[j].mesh_index = mesh_frames[0].mesh_index;
                }
            }

            // position & orientation share a structure: 3 components or num_frames vectors.
            for (auto [key, field] : std::array<std::pair<const char*, vector3_t Model::MeshFrame::*>, 2>{{
                {"position",    &Model::MeshFrame::position},
                {"orientation", &Model::MeshFrame::orientation},
            }})
            {
                json_t* prop = json_object_get(elem, key);
                if (prop == nullptr || !json_is_array(prop))
                {
                    return std::unexpected(std::format(
                        "Property \"{}\" not found or is not an array", key));
                }
                const std::size_t sz = json_array_size(prop);
                if (sz == 3)
                {
                    auto vec = readVector3(prop);
                    if (!vec)
                        return std::unexpected(vec.error());
                    for (int j = 0; j < numFrames; j++)
                        mesh_frames[j].*field = *vec;
                }
                else if (sz == static_cast<std::size_t>(numFrames))
                {
                    for (int j = 0; j < numFrames; j++)
                    {
                        auto vec = readVector3(json_array_get(prop, j));
                        if (!vec)
                            return std::unexpected(vec.error());
                        mesh_frames[j].*field = *vec;
                    }
                }
                else
                {
                    return std::unexpected(std::format(
                        "Number of elements in \"{}\" ({}) does not match number of frames ({})",
                        key, sz, numFrames));
                }
            }
        }
        return {};
    }

    LoadResult<std::vector<light_t>> loadLights(json_t* json)
    {
        if (json == nullptr || !json_is_array(json))
        {
            return std::unexpected(std::string("\"lights\" is not an array"));
        }
        std::vector<light_t> lights;
        lights.reserve(json_array_size(json));
        for (std::size_t i = 0; i < json_array_size(json); i++)
        {
            json_t* light = json_array_get(json, i);
            assert(light != nullptr);
            if (!json_is_object(light))
            {
                printMsg("Warning: Light array contains an element which is not an object - ignoring");
                continue;
            }

            light_t out{};

            auto type = readString(json_object_get(light, "type"), "type");
            if (!type)
                return std::unexpected(type.error());
            if (*type == "diffuse")
                out.type = LIGHT_DIFFUSE;
            else if (*type == "specular")
                out.type = LIGHT_SPECULAR;
            else
                return std::unexpected(std::format("Unrecognized light type \"{}\"", *type));

            json_t* shadow = json_object_get(light, "shadow");
            if (shadow == nullptr || !json_is_boolean(shadow))
            {
                return std::unexpected(std::string(
                    "Property \"shadow\" not found or is not a boolean"));
            }
            out.shadow = json_boolean_value(shadow) ? 1 : 0;

            auto dir = readVector3(json_object_get(light, "direction"));
            if (!dir)
                return std::unexpected(dir.error());
            out.direction = vector3_normalize(*dir);

            auto strength = readNumber(json_object_get(light, "strength"), "strength");
            if (!strength)
                return std::unexpected(strength.error());
            out.intensity = static_cast<float>(*strength);

            lights.push_back(out);
        }
        return lights;
    }

    LoadResult<void> loadProject(Project& project, json_t* json)
    {
        auto id = readRequiredString(json, "id");
        if (!id)
            return std::unexpected(id.error());
        project.id = std::move(*id);

        if (auto original_id = readOptionalString(json, "original_id"); original_id)
            project.original_id = std::move(*original_id);
        else
            return std::unexpected(original_id.error());

        auto name = readRequiredString(json, "name");
        if (!name)
            return std::unexpected(name.error());
        project.name = std::move(*name);

        auto desc = readRequiredString(json, "description");
        if (!desc)
            return std::unexpected(desc.error());
        project.description = std::move(*desc);

        auto capacity = readRequiredString(json, "capacity");
        if (!capacity)
            return std::unexpected(capacity.error());
        project.capacity = std::move(*capacity);

        if (auto author = readOptionalString(json, "author"); author)
            project.author = std::move(*author);
        else
            return std::unexpected(author.error());

        if (auto version = readOptionalString(json, "version"); version)
        {
            if (!version->empty()) project.version = std::move(*version);
        }
        else
        {
            return std::unexpected(version.error());
        }

        // Preview image (optional).
        if (json_t* preview = json_object_get(json, "preview"); preview == nullptr)
        {
            image_new(&project.preview, 1, 1, 0, 0, 0);
        }
        else
        {
            auto preview_path = readString(preview, "preview");
            if (!preview_path)
                return std::unexpected(preview_path.error());
            std::FILE* file = std::fopen(preview_path->c_str(), "rb");
            if (file == nullptr || image_read_png(&project.preview, file) != 0)
            {
                if (file)
                    std::fclose(file);
                return std::unexpected(std::format(
                    "Unable to open image file {}", *preview_path));
            }
            std::fclose(file);
        }

        auto ride_type = readRequiredString(json, "ride_type");
        if (!ride_type)
            return std::unexpected(ride_type.error());
        project.ride_type = std::move(*ride_type);

        if (json_t* f = json_object_get(json, "flags"))
        {
            auto flags = readFlagBits(f, std::span(kRideFlagNames), "flags", "flag");
            if (!flags)
                return std::unexpected(flags.error());
            project.flags = *flags;
        }

        {
            auto sprite_flags = readFlagBits(
                json_object_get(json, "sprites"),
                std::span(kSpriteGroupNames),
                "sprites", "sprite group");
            if (!sprite_flags)
                return std::unexpected(sprite_flags.error());
            project.sprite_flags = *sprite_flags;
        }

        // Implied sprite flags.
        {
            auto sf = static_cast<SpriteFlag>(project.sprite_flags);
            if (has_flag(sf, SpriteFlag::banking))
            {
                sf |= SpriteFlag::diagonalBankTransition;
                if (has_flag(sf, SpriteFlag::gentleSlope))
                    sf |= SpriteFlag::slopeBankTransition;
                if (has_flag(sf, SpriteFlag::slopedBankedTurn))
                    sf |= SpriteFlag::slopedBankTransition | SpriteFlag::bankedSlopeTransition;
            }
            project.sprite_flags = std::to_underlying(sf);
        }

        auto load_field = [&](std::uint32_t& out, const char* key) -> LoadResult<void> {
            auto v = readUint32(json_object_get(json, key), key);
            if (!v) return std::unexpected(v.error());
            out = *v;
            return {};
        };

        if (auto r = load_field(project.zero_cars, "zero_cars"); !r)
            return r;
        if (auto r = load_field(project.tab_car, "preview_tab_car"); !r)
            return r;
        if (auto r = load_field(project.build_menu_priority, "build_menu_priority"); !r)
            return r;

        {
            auto rs = readEnumIndex(
                json_object_get(json, "running_sound"),
                std::span(kRunningSoundNames),
                "running_sound", "running sound");
            if (!rs)
                return std::unexpected(rs.error());
            project.running_sound = *rs;
        }
        {
            auto ss = readEnumIndex(
                json_object_get(json, "secondary_sound"),
                std::span(kSecondarySoundNames),
                "secondary_sound", "secondary sound");
            if (!ss)
                return std::unexpected(ss.error());
            project.secondary_sound = *ss;
        }

        if (auto r = load_field(project.min_cars_per_train, "min_cars_per_train"); !r)
            return r;
        if (auto r = load_field(project.max_cars_per_train, "max_cars_per_train"); !r)
            return r;

        if (auto r = loadConfiguration(project.configuration,
                                        json_object_get(json, "configuration")); !r)
            return r;

        if (auto r = loadColors(project, json_object_get(json, "default_colors")); !r)
            return r;

        if (auto r = loadMeshes(project, json_object_get(json, "meshes")); !r)
            return r;

        json_t* vehicles = json_object_get(json, "vehicles");
        if (vehicles == nullptr || !json_is_array(vehicles))
        {
            return std::unexpected(std::string(
                "Property \"vehicles\" does not exist or is not an array"));
        }
        project.num_sprites = 3;
        project.vehicles.resize(json_array_size(vehicles));
        for (std::size_t i = 0; i < project.vehicles.size(); i++)
        {
            Vehicle& v = project.vehicles[i];
            if (auto r = loadVehicle(v, json_array_get(vehicles, i), project); !r)
                return r;
            v.num_sprites = static_cast<std::uint32_t>(countSprites(
                static_cast<SpriteFlag>(project.sprite_flags),
                static_cast<VehicleFlag>(v.flags)));
            project.num_sprites += v.num_sprites;
        }

        project.category = std::to_underlying(Category::thrillRide);
        return {};
    }
}
