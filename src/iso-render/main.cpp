#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <numbers>
#include <string>

#include <jansson.h>

#include "Mesh.hpp"
#include "Renderer.hpp"

namespace RCTGen {
    namespace {
        inline constexpr float TILE_SIZE = 3.3f;
        inline constexpr float CLEARANCE_HEIGHT_VAL = 0.5f * TILE_SIZE * 0.40824829046f; // 0.5 / sqrt(6)
        inline constexpr std::size_t MAX_MESHES_PER_MODEL = 16;
        inline constexpr std::size_t MAX_FRAMES = 16;
        inline constexpr std::size_t MAX_ITEMS = 256;

        struct model_t {
            std::int32_t num_meshes;
            std::int32_t mesh_index[MAX_MESHES_PER_MODEL][MAX_FRAMES];
            Vector3 position[MAX_MESHES_PER_MODEL][MAX_FRAMES];
            Vector3 orientation[MAX_MESHES_PER_MODEL][MAX_FRAMES];
        };

        struct item_t {
            std::string name;
            std::uint32_t rotations;
            std::uint32_t frames;
            model_t model;
        };

        struct project_t {
            std::uint32_t num_meshes;
            std::uint32_t num_items;
            Mesh meshes[kMaxMeshes];
            item_t items[MAX_ITEMS];
        };

        void print_msg(const char *fmt, ...) {
            std::va_list args;
            va_start(args, fmt);
            std::vprintf(fmt, args);
            std::putchar('\r');
            std::putchar('\n');
            va_end(args);
            std::fflush(stdout);
        }

        int load_mesh(Mesh *model, json_t *mesh) {
            if (json_is_string(mesh)) {
                if (mesh_load(model, json_string_value(mesh))) {
                    print_msg("Failed to load model \"%s\"", json_string_value(mesh));
                    return 1;
                }
                return 0;
            }
            print_msg("Error: Mesh path is not a string");
            return 1;
        }

        int load_vector(Vector3 *vector, json_t *array) {
            const int size = json_array_size(array);
            if (size != 3) {
                print_msg("Vector must have 3 components");
                return 1;
            }

            json_t *x = json_array_get(array, 0);
            json_t *y = json_array_get(array, 1);
            json_t *z = json_array_get(array, 2);

            if (!json_is_number(x) || !json_is_number(y) || !json_is_number(z)) {
                print_msg("Vector components must be numeric");
                return 1;
            }
            vector->x = json_number_value(x);
            vector->y = json_number_value(y);
            vector->z = json_number_value(z);
            return 0;
        }

        json_t *load_optional_array(json_t *json) {
            if (!json) return nullptr;
            if (json_is_array(json)) {
                if (json_array_size(json) == 0) {
                    print_msg("Empty array");
                    return nullptr;
                }
                json_incref(json);
                return json;
            }
            json_t *arr = json_array();
            json_array_append(arr, json);
            return arr;
        }

        int load_model(model_t *model, json_t *json, int num_meshes, int num_frames) {
            if (!json) {
                print_msg("Error: Property \"model\" not found");
                return 1;
            }

            json_t *arr = load_optional_array(json);
            model->num_meshes = json_array_size(arr);
            for (int i = 0; i < model->num_meshes; ++i) {
                json_t *elem = json_array_get(arr, i);
                if (model == nullptr || !json_is_object(elem)) {
                    print_msg("Property \"model\" is not an object");
                    return 1;
                }

                json_t *mesh = json_object_get(elem, "mesh_index");
                if (mesh == nullptr) {
                    print_msg("Error: Property \"mesh_index\" not found");
                    return 1;
                }
                json_t *mesh_arr = load_optional_array(mesh);
                if (mesh_arr == nullptr || (json_array_size(mesh_arr) != 1 && json_array_size(mesh_arr) !=
                                            num_frames)) {
                    print_msg("Error: Number of elements in \"mesh_index\"(%zu) does not match number of frames(%d)",
                              json_array_size(mesh_arr), num_frames);
                    return 1;
                }

                for (int j = 0; j < static_cast<int>(json_array_size(mesh_arr)); ++j) {
                    json_t *mesh_index = json_array_get(mesh_arr, j);
                    if (!json_is_integer(mesh_index)) {
                        print_msg("Error: Property \"mesh_index\" is not an integer");
                        return 1;
                    }
                    model->mesh_index[i][j] = json_integer_value(mesh_index);
                    if (model->mesh_index[i][j] >= num_meshes || model->mesh_index[i][j] < -1) {
                        print_msg("Mesh index %d is out of bounds", model->mesh_index[i][j]);
                        return 1;
                    }
                }
                if (static_cast<int>(json_array_size(mesh_arr)) < num_frames) {
                    for (int j = 0; j < num_frames; ++j) model->mesh_index[i][j] = model->mesh_index[i][0];
                }
                json_decref(mesh_arr);

                json_t *position = json_object_get(elem, "position");
                if (position == nullptr || !json_is_array(position)) {
                    print_msg("Error: Property \"position\" not found or is not an array");
                    return 1;
                }

                if (json_array_size(position) == 3) {
                    Vector3 vec;
                    if (load_vector(&vec, position)) return 1;
                    for (int j = 0; j < num_frames; ++j) model->position[i][j] = vec;
                } else if (static_cast<int>(json_array_size(position)) == num_frames) {
                    for (int j = 0; j < num_frames; ++j)
                        if (load_vector(&model->position[i][j], json_array_get(position, j))) return 1;
                } else {
                    print_msg("Error: Number of elements in \"position\"(%zu) does not match number of frames(%d)",
                              json_array_size(position), num_frames);
                    return 1;
                }

                json_t *orientation = json_object_get(elem, "orientation");
                if (orientation == nullptr || !json_is_array(orientation)) {
                    print_msg("Error: Property \"orientation\" not found or is not an array");
                    return 1;
                }

                if (json_array_size(orientation) == 3) {
                    Vector3 vec;
                    if (load_vector(&vec, orientation)) return 1;
                    for (int j = 0; j < num_frames; ++j) model->orientation[i][j] = vec;
                } else if (static_cast<int>(json_array_size(orientation)) == num_frames) {
                    for (int j = 0; j < num_frames; ++j)
                        if (load_vector(&model->orientation[i][j], json_array_get(orientation, j))) return 1;
                } else {
                    print_msg("Error: Number of elements in \"orientation\"(%zu) does not match number of frames(%d)",
                              json_array_size(orientation), num_frames);
                    return 1;
                }
            }
            json_decref(arr);
            return 0;
        }

        int load_int(std::uint32_t *out, json_t *json, const char *property) {
            if (json != nullptr && json_is_integer(json)) *out = json_integer_value(json);
            else {
                print_msg("Error: Property \"%s\" not found or is not a integer", property);
                return 1;
            }
            return 0;
        }

        int load_lights(Light *lights, int *lights_count, json_t *json) {
            const int num_lights = json_array_size(json);
            for (int i = 0; i < num_lights; ++i) {
                json_t *light = json_array_get(json, i);
                assert(light != nullptr);
                if (!json_is_object(light)) {
                    print_msg("Warning: Light array contains an element which is not an object-ignoring");
                    continue;
                }

                json_t *type = json_object_get(light, "type");
                if (type == nullptr || !json_is_string(type)) {
                    print_msg("Error: Property \"type\" not found or is not a string");
                    return 1;
                }

                const char *type_value = json_string_value(type);
                if (std::strcmp(type_value, "diffuse") == 0) lights[i].type = LIGHT_DIFFUSE;
                else if (std::strcmp(type_value, "specular") == 0) lights[i].type = LIGHT_SPECULAR;
                else print_msg("Unrecognized light type \"%s\"", type_value);

                json_t *shadow = json_object_get(light, "shadow");
                if (shadow == nullptr || !json_is_boolean(shadow)) {
                    print_msg("Error: Property \"shadow\" not found or is not a boolean");
                    return 1;
                }
                lights[i].shadow = json_boolean_value(shadow) ? 1 : 0;

                json_t *direction = json_object_get(light, "direction");
                if (direction == nullptr || !json_is_array(direction)) {
                    print_msg("Error: Property \"direction\" not found or is not a direction");
                    return 1;
                }
                if (load_vector(&lights[i].direction, direction)) return 1;
                lights[i].direction = lights[i].direction.normalized();

                json_t *strength = json_object_get(light, "strength");
                if (strength == nullptr || !json_is_number(strength)) {
                    print_msg("Error: Property \"strength\" not found or is not a number");
                    return 1;
                }
                lights[i].intensity = json_number_value(strength);
            }
            *lights_count = num_lights;
            return 0;
        }

        int load_project(project_t *project, json_t *json) {
            json_t *meshes = json_object_get(json, "meshes");
            if (meshes == nullptr) print_msg("Error: Property \"meshes\" does not exist or is not an array");
            project->num_meshes = json_array_size(meshes);

            for (std::uint32_t i = 0; i < project->num_meshes; ++i) {
                json_t *mesh = json_array_get(meshes, i);
                assert(mesh != nullptr);
                if (load_mesh(project->meshes + i, mesh)) {
                    for (std::uint32_t j = 0; j < i; ++j) mesh_destroy(project->meshes + j);
                    return 1;
                }
            }

            json_t *items = json_object_get(json, "items");
            if (items == nullptr || !json_is_array(items))
                print_msg(
                    "Error: Property \"items\" does not exist or is not an array");
            project->num_items = json_array_size(items);

            for (std::uint32_t i = 0; i < project->num_items; ++i) {
                json_t *item = json_array_get(items, i);
                assert(item != nullptr);
                if (!json_is_object(item)) {
                    print_msg("Error: Item array contains an element which is not an object");
                    return 1;
                }

                json_t *name = json_object_get(item, "name");
                if (name == nullptr || !json_is_string(name)) {
                    print_msg("Error: No property \"name\" found");
                    return 1;
                }
                project->items[i].name = json_string_value(name);

                if (load_int(&project->items[i].rotations, json_object_get(item, "rotations"), "rotations")) return 1;
                if (load_int(&project->items[i].frames, json_object_get(item, "frames"), "frames")) return 1;

                if (load_model(&project->items[i].model, json_object_get(item, "model"), project->num_meshes,
                               project->items[i].frames))
                    return 1;
            }
            return 0;
        }

        void project_add_model_to_context(project_t *project, Context *context, model_t *model, int frame,
                                          int rotation) {
            constexpr float offsets[8] = {0, -1, 0, -1.5f, 0, -1, 0, -1.5f};
            for (int i = 0; i < model->num_meshes; ++i) {
                if (model->mesh_index[i][frame] == -1) continue;
                const Vector3 orientation = model->orientation[i][frame] * static_cast<float>(std::numbers::pi / 180.0);
                const Matrix3 rot = matrix_mult(rotate_y(orientation.x),
                                                matrix_mult(rotate_z(orientation.y), rotate_x(orientation.z)));
                const Vector3 pos = model->position[i][frame]
                                    + vector3(CLEARANCE_HEIGHT_VAL * offsets[2 * rotation] / 8.0f,
                                              CLEARANCE_HEIGHT_VAL * offsets[2 * rotation + 1] / 8.0f, 0);
                context_add_model(context, project->meshes + model->mesh_index[i][frame], transform(rot, pos), 0);
            }
        }

        int project_export(project_t *project, Context *context, json_t *sprites, const char *base_dir,
                           const char *output_dir) {
            Image images[4 * MAX_FRAMES];

            for (std::uint32_t i = 0; i < project->num_items; ++i) {
                std::printf("Rendering item %u\n", i);
                for (std::uint32_t frame = 0; frame < project->items[i].frames; ++frame) {
                    for (std::uint32_t j = 0; j < project->items[i].rotations; ++j) {
                        context_begin_render(context);
                        project_add_model_to_context(project, context, &project->items[i].model, frame, j);
                        context_finalize_render(context);
                        context_render_view(context, rotate_y(0.5f * j * std::numbers::pi_v<float>),
                                            images + frame * project->items[i].rotations + j);
                    }
                    context_end_render(context);
                }

                for (std::uint32_t frame = 0; frame < project->items[i].frames; ++frame)
                    for (std::uint32_t angle = 0; angle < project->items[i].rotations; ++angle) {
                        char final_filename[512];
                        char relative_filename[512];
                        if (project->items[i].frames > 1)
                            std::snprintf(relative_filename, 512, "%s%s_%u_%u.png", output_dir,
                                          project->items[i].name.c_str(), frame, angle + 1);
                        else
                            std::snprintf(relative_filename, 512, "%s%s_%u.png", output_dir,
                                          project->items[i].name.c_str(), angle + 1);
                        std::snprintf(final_filename, 512, "%s%s", base_dir, relative_filename);

                        std::FILE *file = std::fopen(final_filename, "wb");
                        if (file == nullptr) {
                            std::printf("Error: could not open %s for writing\n", final_filename);
                            std::exit(1);
                        }
                        const std::uint32_t index = frame * project->items[i].rotations + angle;
                        image_crop(&images[index]);
                        image_write_png(&images[index], nullptr, file);
                        std::fclose(file);

                        json_t *sprite_entry = json_object();
                        json_object_set(sprite_entry, "path", json_string(relative_filename));
                        json_object_set(sprite_entry, "x", json_integer(images[index].x_offset));
                        json_object_set(sprite_entry, "y", json_integer(images[index].y_offset));
                        json_object_set(sprite_entry, "palette", json_string("keep"));
                        json_array_append(sprites, sprite_entry);

                        std::printf("%s (%d %d)\n", final_filename, images[index].x_offset, images[index].y_offset);
                        image_destroy(images + index);
                    }
            }
            return 0;
        }
    } // namespace
} // namespace RCTGen

int main(int argc, char *argv[]) {
    using namespace RCTGen;
    project_t project{};

    json_error_t error;
    json_t *project_json = json_load_file(argv[1], 0, &error);
    if (project_json == nullptr) {
        print_msg("Error: %s at line %d column %d", error.text, error.line, error.column);
        return 1;
    }

    const char *base_dir = nullptr;
    if (json_t *j = json_object_get(project_json, "base_directory"); j != nullptr && json_is_string(j))
        base_dir = json_string_value(j);
    else std::printf("Error: No property \"base_directory\" found\n");

    const char *sprite_dir = nullptr;
    if (json_t *j = json_object_get(project_json, "sprite_directory"); j != nullptr && json_is_string(j))
        sprite_dir = json_string_value(j);
    else std::printf("Error: No property \"sprite_directory\" found\n");

    const char *spritefile_in = nullptr;
    if (json_t *j = json_object_get(project_json, "spritefile_in"); j != nullptr && json_is_string(j))
        spritefile_in = json_string_value(j);
    else std::printf("Error: No property \"spritefile_in\" found\n");

    const char *spritefile_out = nullptr;
    if (json_t *j = json_object_get(project_json, "spritefile_out"); j != nullptr && json_is_string(j))
        spritefile_out = json_string_value(j);
    else std::printf("Error: No property \"spritefile_out\" found\n");

    int num_lights = 9;
    Light lights[16] = {
        {LIGHT_DIFFUSE, 0, vector3(0.0f, -1.0f, 0.0f).normalized(), 0.25f},
        {LIGHT_DIFFUSE, 0, vector3(1.0f, 0.3f, 0.0f).normalized(), 0.32f},
        {LIGHT_SPECULAR, 0, vector3(1, 1, -1).normalized(), 1.0f},
        {LIGHT_DIFFUSE, 0, vector3(1, 0.65f, -1).normalized(), 0.8f},
        {LIGHT_DIFFUSE, 0, vector3(0.0f, 1.0f, 0.0f), 0.174f},
        {LIGHT_DIFFUSE, 0, vector3(-1.0f, 0.0f, 0.0f).normalized(), 0.15f},
        {LIGHT_DIFFUSE, 0, vector3(0.0f, 1.0f, 1.0f).normalized(), 0.2f},
        {LIGHT_DIFFUSE, 0, vector3(0.65f, 0.816f, -0.65f).normalized(), 0.25f},
        {LIGHT_DIFFUSE, 0, vector3(-1.0f, 0.0f, -1.0f).normalized(), 0.25f},
        {0, 0, {0, 0, 0}, 0},
        {0, 0, {0, 0, 0}, 0},
        {0, 0, {0, 0, 0}, 0},
        {0, 0, {0, 0, 0}, 0},
        {0, 0, {0, 0, 0}, 0},
        {0, 0, {0, 0, 0}, 0},
        {0, 0, {0, 0, 0}, 0},
    };

    if (json_t *light_array = json_object_get(project_json, "lights"); light_array != nullptr) {
        if (!json_is_array(light_array)) {
            print_msg("Error: Property \"lights\" is not an array");
            return 1;
        }
        if (load_lights(lights, &num_lights, light_array)) return 1;
    }

    if (load_project(&project, project_json)) return 1;

    char full_path[256];
    std::snprintf(full_path, 256, "%s%s", base_dir, spritefile_in);
    json_t *sprites = json_load_file(full_path, 0, &error);
    if (sprites == nullptr) {
        std::printf("Error: %s in file %s line %d column %d\n", error.text, error.source, error.line, error.column);
        return 1;
    }

    Context context;
    context_init(&context, lights, num_lights, 1, palette_rct2(), TILE_SIZE);

    if (project_export(&project, &context, sprites, base_dir, sprite_dir)) return 1;

    std::snprintf(full_path, 256, "%s%s", base_dir, spritefile_out);
    json_dump_file(sprites, full_path, JSON_INDENT(4));
    context_destroy(&context);
    return 0;
}