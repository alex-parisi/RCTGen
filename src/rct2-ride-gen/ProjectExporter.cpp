/// ProjectExporter.cpp

#include "ProjectExporter.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <numbers>
#include <string>
#include <utility>
#include <vector>

#include <jansson.h>
#include <zip.h>

#include "image.h"
#include "model.h"
#include "Constants.hpp"
#include "Json.hpp"
#include "Logging.hpp"
#include "SpriteRenderer.hpp"
#include "Vehicle.hpp"

namespace fs = std::filesystem;

namespace RCTGen {

    using Context = context_t;

    namespace {
        // friction_sound_id table, indexed by the running_sound enum from JSON.
        // Preserves the historical mapping (which does not align name-for-name
        // with the running-sound names — see Constants.hpp).
        constexpr std::array kFrictionSoundIds = {
            std::to_underlying(RunningSound::woodenOld),
            std::to_underlying(RunningSound::woodenModern),
            std::to_underlying(RunningSound::steel),
            std::to_underlying(RunningSound::steelSmooth),
            std::to_underlying(RunningSound::waterslide),
            std::to_underlying(RunningSound::train),
            std::to_underlying(RunningSound::engine),
        };

        void addModelToContext(
            const Project &project, Context &context, const Model &model, const int frame, const int mask) {
            for (const auto &mesh_frames: model.meshes) {
                const auto &[mesh_index, position, orientation] = mesh_frames[frame];
                if (mesh_index == -1)
                    continue;
                auto [x, y, z] = vector3_mult(orientation, std::numbers::pi / 180.0);
                context_add_model(
                    &context,
                    const_cast<mesh_t *>(&project.meshes[mesh_index]),
                    transform(
                        matrix_mult(
                            rotate_y(x),
                            matrix_mult(rotate_z(y), rotate_x(z))),
                        position),
                    mask);
            }
        }

        void emitSpriteGroups(json_t *spriteGroups, const SpriteFlag sf, const VehicleFlag vf) {
            auto set = [&](const char *key, const int n) {
                json_object_set_new(spriteGroups, key, json_integer(n));
            };

            if (has_flag(sf, SpriteFlag::flatSlope)) set("slopeFlat", 32);
            if (has_flag(sf, SpriteFlag::gentleSlope)) {
                set("slopes12", 4);
                set("slopes25", 32);
            }
            if (has_flag(sf, SpriteFlag::steepSlope)) {
                set("slopes42", 8);
                set("slopes60", 32);
            }
            if (has_flag(sf, SpriteFlag::verticalSlope)) {
                set("slopes75", 4);
                set("slopes90", 32);
                set("slopesLoop", 4);
                set("slopeInverted", 4);
            }
            if (has_flag(sf, SpriteFlag::diagonalSlope)) {
                set("slopes8", 4);
                set("slopes16", 4);
                set("slopes50", 4);
            }
            if (has_flag(sf, SpriteFlag::banking)) {
                set("flatBanked22", 8);
                set("flatBanked45", 32);
            }
            if (has_flag(sf, SpriteFlag::inlineTwist)) {
                set("flatBanked67", 4);
                set("flatBanked90", 4);
                set("inlineTwists", 4);
            }
            if (has_flag(sf, SpriteFlag::slopeBankTransition)) set("slopes12Banked22", 32);
            if (has_flag(sf, SpriteFlag::diagonalBankTransition)) set("slopes8Banked22", 4);
            if (has_flag(sf, SpriteFlag::slopedBankTransition)) set("slopes25Banked22", 4);
            if (has_flag(sf, SpriteFlag::diagonalSlopedBankTransition)) {
                set("slopes8Banked45", 4);
                set("slopes16Banked22", 4);
                set("slopes16Banked45", 4);
            }
            if (has_flag(sf, SpriteFlag::slopedBankedTurn)) set("slopes25Banked45", 32);
            if (has_flag(sf, SpriteFlag::bankedSlopeTransition)) set("slopes12Banked45", 4);
            if (has_flag(sf, SpriteFlag::zeroGRoll)) {
                set("slopes25Banked67", 4);
                set("slopes25Banked90", 4);
                set("slopes25InlineTwists", 4);
                set("slopes42Banked22", 4);
                set("slopes42Banked45", 4);
                set("slopes42Banked67", 4);
                set("slopes42Banked90", 4);
                set("slopes60Banked22", has_flag(sf, SpriteFlag::diveLoop) ? 8 : 4);
            }
            if (has_flag(sf, SpriteFlag::diveLoop)) {
                set("slopes50Banked45", 8);
                set("slopes50Banked67", 8);
                set("slopes50Banked90", 8);
            }
            if (has_flag(sf, SpriteFlag::corkscrew)) set("corkscrews", 4);
            if (has_flag(vf, VehicleFlag::restraintAnimation)) set("restraintAnimation", 4);
        }

        JsonRef buildProjectJson(const Project &project) {
            json_t *json = json_object();
            json_object_set_new(json, "id", json_string(project.id.c_str()));
            if (!project.original_id.empty())
                json_object_set_new(json, "originalId", json_string(project.original_id.c_str()));
            json_object_set_new(json, "version", json_string(project.version.c_str()));

            json_t *authors = json_array();
            if (!project.author.empty())
                json_array_append_new(authors, json_string(project.author.c_str()));
            json_object_set_new(json, "authors", authors);
            json_object_set_new(json, "objectType", json_string("ride"));

            json_t *properties = json_object();
            {
                json_t *types = json_array();
                json_array_append_new(types, json_string(project.ride_type.c_str()));
                json_object_set_new(properties, "type", types);
            }
            json_object_set_new(properties, "category",
                                json_string(std::string(kCategoryNames[project.category]).c_str()));
            json_object_set_new(properties, "minCarsPerTrain", json_integer(project.min_cars_per_train));
            json_object_set_new(properties, "maxCarsPerTrain", json_integer(project.max_cars_per_train));
            json_object_set_new(properties, "numEmptyCars", json_integer(project.zero_cars));
            json_object_set_new(properties, "tabCar", json_integer(project.tab_car));
            json_object_set_new(properties, "defaultCar",
                                json_integer(project.configuration[std::to_underlying(CarIndex::defaultVal)]));

            auto front = project.configuration[std::to_underlying(CarIndex::front)];
            if (front != 0xFF) json_object_set_new(properties, "headCars", json_integer(front));
            auto rear = project.configuration[std::to_underlying(CarIndex::rear)];
            if (rear != 0xFF) json_object_set_new(properties, "tailCars", json_integer(rear));

            json_object_set_new(properties, "buildMenuPriority", json_integer(project.build_menu_priority));

            const auto rf = static_cast<RideFlag>(project.flags);
            if (has_flag(rf, RideFlag::noCollisionCrashes))
                json_object_set_new(properties, "noCollisionCrashes", json_true());
            if (has_flag(rf, RideFlag::riderControlsSpeed))
                json_object_set_new(properties, "riderControlsSpeed", json_true());

            // Color presets — wrapped in arrays-of-arrays.
            json_t *car_color_presets = json_array();
            for (const auto &preset: project.colors) {
                json_t *preset_arr = json_array();
                for (const auto idx: preset) {
                    json_array_append_new(preset_arr,
                                          json_string(std::string(kColorNames[idx]).c_str()));
                }
                json_t *wrapped = json_array();
                json_array_append_new(wrapped, preset_arr);
                json_array_append_new(car_color_presets, wrapped);
            }
            json_object_set_new(properties, "carColours", car_color_presets);

            // Cars (vehicles).
            json_t *cars = json_array();
            for (const Vehicle &vehicle: project.vehicles) {
                json_t *car = json_object();
                json_object_set_new(car, "rotationFrameMask", json_integer(31));
                json_object_set_new(car, "spacing",
                                    json_integer(static_cast<json_int_t>((vehicle.spacing * 278912) / kTileSize)));
                json_object_set_new(car, "mass", json_integer(vehicle.mass));
                json_object_set_new(car, "numSeats", json_integer(vehicle.num_riders));
                json_object_set_new(car, "numSeatRows", json_integer(vehicle.riders.size()));

                const std::uint8_t friction = (project.running_sound < kFrictionSoundIds.size())
                                            ? kFrictionSoundIds[project.running_sound]
                                            : 0u;
                json_object_set_new(car, "frictionSoundId", json_integer(friction));
                json_object_set_new(car, "soundRange", json_integer(project.secondary_sound));
                json_object_set_new(car, "drawOrder", json_integer(vehicle.draw_order));

                json_t *sprite_groups = json_object();
                emitSpriteGroups(sprite_groups,
                                 static_cast<SpriteFlag>(project.sprite_flags),
                                 static_cast<VehicleFlag>(vehicle.flags));
                json_object_set_new(car, "spriteGroups", sprite_groups);

                const auto vf = static_cast<VehicleFlag>(vehicle.flags);
                if (has_flag(vf, VehicleFlag::secondaryRemap))
                    json_object_set_new(car, "hasAdditionalColour1", json_true());
                if (has_flag(vf, VehicleFlag::tertiaryRemap))
                    json_object_set_new(car, "hasAdditionalColour2", json_true());
                if (has_flag(vf, VehicleFlag::ridersScream))
                    json_object_set_new(car, "hasScreamingRiders", json_true());

                json_t *loading_positions = json_array();
                for (const auto &[meshes]: vehicle.riders) {
                    const int position = static_cast<int>(std::round(
                        32.0 * meshes[0][0].position.x / kTileSize));
                    if (vehicle.num_riders > 1) {
                        json_array_append_new(loading_positions, json_integer(position - 1));
                        json_array_append_new(loading_positions, json_integer(position + 1));
                    } else {
                        json_array_append_new(loading_positions, json_integer(position));
                    }
                }
                json_object_set_new(car, "loadingPositions", loading_positions);
                json_array_append_new(cars, car);
            }
            json_object_set_new(properties, "cars", cars);
            json_object_set_new(json, "properties", properties);

            // String tables.
            auto add_string_table = [&](json_t *parent, const char *key, const std::string &value) {
                json_t *tbl = json_object();
                json_object_set_new(tbl, "en-GB", json_string(value.c_str()));
                json_object_set_new(parent, key, tbl);
            };
            json_t *strings = json_object();
            add_string_table(strings, "name", project.name);
            add_string_table(strings, "description", project.description);
            add_string_table(strings, "capacity", project.capacity);
            json_object_set_new(json, "strings", strings);

            return adoptJson(json);
        }

        ExportResult<void> renderSprites(
            Project &project, Context &context, json_t *images_json) {
            // Preview image.
            const fs::path preview_path = "object/images/preview.png";
            std::FILE *file = std::fopen(preview_path.string().c_str(), "wb");
            if (!file) {
                return std::unexpected(std::format("Failed to write file {}", preview_path.string()));
            }
            image_write_png(&project.preview, nullptr, file);
            std::fclose(file);

            for (int i = 0; i < 3; i++) {
                json_array_append_new(images_json,
                                      releaseJson(makeImageObject("images/preview.png", 0, 0, -1, -1, -1, -1)));
            }

            for (std::size_t i = 0; i < project.vehicles.size(); i++) {
                Vehicle &vehicle = project.vehicles[i];
                const auto sf = static_cast<SpriteFlag>(project.sprite_flags);
                const auto vf = static_cast<VehicleFlag>(vehicle.flags);
                const int num_frames = has_flag(vf, VehicleFlag::restraintAnimation) ? 4 : 1;
                const int num_car_images = countSprites(sf, vf);
                const int num_images = num_car_images * (1 + static_cast<int>(vehicle.riders.size()));

                std::vector images(num_images, image_t{});

                printMsg("Rendering car sprites");
                int base = 0;
                for (int frame = 0; frame < num_frames; frame++) {
                    context_begin_render(&context);
                    addModelToContext(project, context, vehicle.model, frame, 0);
                    context_finalize_render(&context);
                    base += renderVehicleFrame(&context, sf, frame, images.data() + base);
                    context_end_render(&context);
                }

                for (std::size_t j = 0; j < vehicle.riders.size(); j++) {
                    printMsg("Rendering peep sprites {}", j);
                    base = 0;
                    for (int frame = 0; frame < num_frames; frame++) {
                        context_begin_render(&context);
                        addModelToContext(project, context, vehicle.model, frame, 1);
                        for (std::size_t k = 0; k < j; k++)
                            addModelToContext(project, context, vehicle.riders[k], frame, 1);
                        addModelToContext(project, context, vehicle.riders[j], frame, 0);
                        context_finalize_render(&context);
                        base += renderVehicleFrame(
                            &context, sf, frame,
                            images.data() + (j + 1) * num_car_images + base);
                        context_end_render(&context);
                    }
                }

                image_t atlas;
                std::vector x_coords(num_images, 0);
                std::vector y_coords(num_images, 0);
                image_create_atlas(&atlas, images.data(), num_images,
                                   x_coords.data(), y_coords.data());

                const std::string image_path = std::format("images/car_{}.png", i);
                for (int k = 0; k < num_images; k++) {
                    json_array_append_new(images_json,
                                          releaseJson(makeImageObject(
                                              image_path,
                                              images[k].x_offset, images[k].y_offset,
                                              x_coords[k], y_coords[k],
                                              images[k].width, images[k].height)));
                }
                const fs::path out_image_path = fs::path("object") / image_path;
                std::FILE *out = std::fopen(out_image_path.string().c_str(), "wb");
                if (!out) {
                    for (auto &img: images) image_destroy(&img);
                    image_destroy(&atlas);
                    return std::unexpected(std::format(
                        "Failed to write file {}", out_image_path.string()));
                }
                image_write_png(&atlas, nullptr, out);
                std::fclose(out);
                for (auto &img: images) image_destroy(&img);
                image_destroy(&atlas);
            }
            return {};
        }

        ExportResult<void> parkObjAdd(zip_t *archive, const fs::path &archive_path) {
            const fs::path src = fs::path("object") / archive_path;
            std::FILE *file = std::fopen(src.string().c_str(), "rb");
            if (file == nullptr) {
                return std::unexpected(std::format("Unable to open \"{}\"", src.string()));
            }
            zip_source_t *zsrc = zip_source_filep(archive, file, 0, -1);
            if (zsrc == nullptr) {
                std::fclose(file);
                return std::unexpected(std::string("zip_source_filep failed"));
            }
            if (zip_file_add(archive, archive_path.generic_string().c_str(),
                             zsrc, ZIP_FL_OVERWRITE) < 0) {
                return std::unexpected(std::format(
                    "Unable to add \"{}\" to ZIP archive", archive_path.generic_string()));
            }
            return {};
        }

        ExportResult<void> makeParkObj(const Project &project, const fs::path &path) {
            int zerr = 0;
            zip_t *archive = zip_open(path.string().c_str(), ZIP_CREATE | ZIP_TRUNCATE, &zerr);
            if (archive == nullptr) {
                zip_error_t ze;
                zip_error_init_with_code(&ze, zerr);
                std::string msg = std::format(
                    "Unable to create \"{}\": {}", path.string(), zip_error_strerror(&ze));
                zip_error_fini(&ze);
                return std::unexpected(std::move(msg));
            }
            if (zip_dir_add(archive, "images", ZIP_FL_ENC_UTF_8) < 0) {
                zip_close(archive);
                return std::unexpected(std::string(
                    "Unable to add subdirectory \"images\" to ZIP archive"));
            }
            if (auto r = parkObjAdd(archive, "object.json"); !r) {
                zip_close(archive);
                return r;
            }
            if (auto r = parkObjAdd(archive, "images/preview.png"); !r) {
                zip_close(archive);
                return r;
            }
            for (std::size_t i = 0; i < project.vehicles.size(); i++) {
                if (auto r = parkObjAdd(archive, std::format("images/car_{}.png", i)); !r) {
                    zip_close(archive);
                    return r;
                }
            }
            if (zip_close(archive) < 0) {
                return std::unexpected(std::format("Failed to write ZIP file \"{}\"", path.string()));
            }
            return {};
        }

        void cleanWorkingDirectory(const Project &project) {
            std::error_code ec;
            fs::remove("object/object.json", ec);
            fs::remove("object/images/preview.png", ec);
            for (std::size_t i = 0; i < project.vehicles.size(); i++) {
                fs::remove(std::format("images/car_{}.png", i), ec);
            }
        }
    } // namespace

    ExportResult<void> exportProject(
        Project &project,
        Context &context,
        const fs::path &outputDirectory,
        const bool skipRender) {
        const JsonRef json = buildProjectJson(project);

        json_t *images_json = nullptr;
        JsonRef loaded_object_json;
        if (skipRender) {
            auto loaded = loadFile("object/object.json");
            if (!loaded) {
                return std::unexpected(std::format(
                    "Unable to load object/object.json: {}", loaded.error()));
            }
            loaded_object_json = std::move(*loaded);
            images_json = json_object_get(loaded_object_json.get(), "images");
            if (!json_is_array(images_json)) {
                return std::unexpected(std::string("Property \"images\" is not an array"));
            }
            // Borrow the array; json_object_set will incref it inside the new object.
            json_incref(images_json);
        } else {
            cleanWorkingDirectory(project);
            images_json = json_array();
            if (auto r = renderSprites(project, context, images_json); !r) {
                json_decref(images_json);
                return r;
            }
        }
        json_object_set_new(json.get(), "images", images_json);

        json_dump_file(json.get(), "object/object.json", JSON_INDENT(4));

        const fs::path parkobj_path = outputDirectory / (project.id + ".parkobj");
        return makeParkObj(project, parkobj_path);
    }

    ExportResult<void> exportProjectTest(Project &project, Context &context) {
        for (std::size_t i = 0; i < project.vehicles.size(); i++) {
            Vehicle &vehicle = project.vehicles[i];
            const auto vf = static_cast<VehicleFlag>(vehicle.flags);
            const int num_frames = has_flag(vf, VehicleFlag::restraintAnimation) ? 4 : 1;
            for (int j = 0; j < num_frames; j++) {
                printMsg("Rendering vehicle {} frame {}", i, j);
                image_t image;
                context_begin_render(&context);
                addModelToContext(project, context, vehicle.model, j, 0);
                for (const Model &rider: vehicle.riders) {
                    addModelToContext(project, context, rider, j, 0);
                }
                context_finalize_render(&context);
                context_render_view(&context, rotate_y(std::numbers::pi), &image);
                context_end_render(&context);

                const fs::path path = std::format("test/car_{}_{}.png", i, j);
                std::FILE *file = std::fopen(path.string().c_str(), "wb");
                if (!file) {
                    image_destroy(&image);
                    return std::unexpected(std::format("Failed to write file {}", path.string()));
                }
                image_write_png(&image, nullptr, file);
                std::fclose(file);
                image_destroy(&image);
            }
        }
        return {};
    }
}