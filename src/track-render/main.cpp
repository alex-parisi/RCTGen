#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <numbers>

#include <jansson.h>

#include "track.h"
#include "mask.h"

namespace
{

context_t get_context(light_t* lights, std::uint32_t num_lights, std::uint32_t dither)
{
    context_t context;
    context_init(&context, lights, num_lights, dither, palette_rct2(), TILE_SIZE);
    return context;
}

int load_model(mesh_t* model, json_t* json, const char* name)
{
    json_t* mesh = json_object_get(json, name);
    if (mesh != nullptr)
    {
        if (json_is_string(mesh))
        {
            if (mesh_load_transform(model, json_string_value(mesh), rotate_y(-0.5f * std::numbers::pi_v<float>)))
            {
                std::printf("Failed to load model from file \"%s\"\n", json_string_value(mesh));
                return 1;
            }
            return 0;
        }
        std::printf("Error: Property \"%s\" not found or is not an object\n", name);
        return 1;
    }
    return 2;
}

int load_groups(json_t* json, std::uint64_t* out)
{
    std::uint64_t groups = 0;
    for (int i = 0; i < static_cast<int>(json_array_size(json)); ++i)
    {
        json_t* group_name = json_array_get(json, i);
        assert(group_name != nullptr);
        if (!json_is_string(group_name))
        {
            std::printf("Error: Array \"sections\" contains non-string value\n");
            return 1;
        }
        const char* name = json_string_value(group_name);

        struct { const char* key; std::uint64_t value; } map[] = {
            {"flat", TRACK_GROUP_FLAT},
            {"brakes", TRACK_GROUP_BRAKES},
            {"block_brakes", TRACK_GROUP_BLOCK_BRAKES},
            {"diagonal_brakes", TRACK_GROUP_DIAGONAL_BRAKES},
            {"sloped_brakes", TRACK_GROUP_SLOPED_BRAKES},
            {"magnetic_brakes", TRACK_GROUP_MAGNETIC_BRAKES},
            {"turns", TRACK_GROUP_TURNS},
            {"gentle_slopes", TRACK_GROUP_GENTLE_SLOPES},
            {"steep_slopes", TRACK_GROUP_STEEP_SLOPES},
            {"vertical_slopes", TRACK_GROUP_VERTICAL_SLOPES},
            {"diagonals", TRACK_GROUP_DIAGONALS},
            {"sloped_turns", TRACK_GROUP_SLOPED_TURNS | TRACK_GROUP_STEEP_SLOPED_TURNS},
            {"gentle_sloped_turns", TRACK_GROUP_SLOPED_TURNS},
            {"banked_turns", TRACK_GROUP_BANKED_TURNS},
            {"banked_sloped_turns", TRACK_GROUP_BANKED_SLOPED_TURNS},
            {"large_sloped_turns", TRACK_GROUP_LARGE_SLOPED_TURNS},
            {"large_banked_sloped_turns", TRACK_GROUP_LARGE_BANKED_SLOPED_TURNS},
            {"s_bends", TRACK_GROUP_S_BENDS},
            {"banked_s_bends", TRACK_GROUP_BANKED_S_BENDS},
            {"helices", TRACK_GROUP_HELICES},
            {"small_slope_transitions", TRACK_GROUP_SMALL_SLOPE_TRANSITIONS},
            {"large_slope_transitions", TRACK_GROUP_LARGE_SLOPE_TRANSITIONS},
            {"barrel_rolls", TRACK_GROUP_BARREL_ROLLS},
            {"inline_twists", TRACK_GROUP_INLINE_TWISTS},
            {"quarter_loops", TRACK_GROUP_QUARTER_LOOPS},
            {"corkscrews", TRACK_GROUP_CORKSCREWS},
            {"large_corkscrews", TRACK_GROUP_LARGE_CORKSCREWS},
            {"half_loops", TRACK_GROUP_HALF_LOOPS},
            {"vertical_loops", TRACK_GROUP_VERTICAL_LOOPS},
            {"medium_half_loops", TRACK_GROUP_MEDIUM_HALF_LOOPS},
            {"large_half_loops", TRACK_GROUP_LARGE_HALF_LOOPS},
            {"zero_g_rolls", TRACK_GROUP_ZERO_G_ROLLS},
            {"dive_loops", TRACK_GROUP_DIVE_LOOPS},
            {"boosters", TRACK_GROUP_BOOSTERS},
            {"launched_lifts", TRACK_GROUP_LAUNCHED_LIFTS},
            {"turn_bank_transitions", TRACK_GROUP_TURN_BANK_TRANSITIONS},
            {"steep_bank_transitions", TRACK_GROUP_STEEP_BANK_TRANSITIONS},
            {"large_steep_sloped_turns", TRACK_GROUP_LARGE_STEEP_SLOPED_TURNS},
            {"banked_barrel_rolls", TRACK_GROUP_BANKED_BARREL_ROLLS},
            {"banked_inline_twists", TRACK_GROUP_BANKED_INLINE_TWISTS},
            {"banked_zero_g_rolls", TRACK_GROUP_BANKED_ZERO_G_ROLLS},
            {"vertical_boosters", TRACK_GROUP_VERTICAL_BOOSTERS},
        };
        bool matched = false;
        for (const auto& [key, value] : map)
        {
            if (std::strcmp(name, key) == 0) { groups |= value; matched = true; break; }
        }
        if (!matched)
        {
            std::printf("Error: Unrecognized section group \"%s\"\n", name);
            return 1;
        }
    }
    *out = groups;
    return 0;
}

int load_offsets(json_t* json, float* offsets)
{
    constexpr const char* row_names[10] = {"flat", "gentle", "steep", "flat_banked", "gentle_banked", "inverted", "diagonal", "diagonal_banked", "diagonal_gentle", "diagonal_steep"};

    std::memset(offsets, 0, 88 * sizeof(float));

    for (int i = 0; i < 10; ++i)
    {
        json_t* row = json_object_get(json, row_names[i]);
        if (row == nullptr) continue;
        if (!json_is_array(row) || json_array_size(row) != 8)
        {
            std::printf("Property \"%s\" is not an array of length 8\n", row_names[i]);
            return 1;
        }
        for (int j = 0; j < 8; ++j)
        {
            json_t* value = json_array_get(row, j);
            if (!json_is_number(value))
            {
                std::printf("Array \"%s\" contains non numeric value\n", row_names[i]);
                return 1;
            }
            offsets[8 * i + j] = json_number_value(value);
        }
    }
    return 0;
}

int load_required_float(json_t* json, const char* name, float* value, float mult, int preloaded)
{
    json_t* value_json = json_object_get(json, name);
    if (value_json != nullptr)
    {
        if (json_is_number(value_json)) *value = mult * json_number_value(value_json);
        else
        {
            std::printf("Error: Property \"%s\" is not a number\n", name);
            return 1;
        }
    }
    else if (!preloaded)
    {
        std::printf("Error: Property \"%s\" not found\n", name);
        return 1;
    }
    return 0;
}

int load_float_with_default(json_t* json, const char* name, float* value, float mult, float default_value, int preloaded)
{
    json_t* value_json = json_object_get(json, name);
    if (value_json != nullptr)
    {
        if (json_is_number(value_json)) *value = mult * json_number_value(value_json);
        else
        {
            std::printf("Error: Property \"%s\" not found or is not a number\n", name);
            return 1;
        }
    }
    else if (!preloaded) *value = default_value;
    return 0;
}

int load_track_type(track_type_t* track_type, json_t* json, int preloaded)
{
    json_t* flags = json_object_get(json, "flags");
    if (flags != nullptr)
    {
        track_type->flags = 0;

        if (!json_is_array(flags)) { std::printf("Error: Property \"flags\" is not an array\n"); return 1; }
        for (int i = 0; i < static_cast<int>(json_array_size(flags)); ++i)
        {
            json_t* flag_name = json_array_get(flags, i);
            assert(flag_name != nullptr);
            if (!json_is_string(flag_name))
            {
                std::printf("Error: Array \"flags\" contains non-string value\n");
                return 1;
            }
            const char* name = json_string_value(flag_name);
            if (std::strcmp(name, "has_lift") == 0) track_type->flags |= TRACK_HAS_LIFT;
            else if (std::strcmp(name, "has_supports") == 0) track_type->flags |= TRACK_HAS_SUPPORTS;
            else if (std::strcmp(name, "separate_tie") == 0) track_type->flags |= TRACK_SEPARATE_TIE;
            else if (std::strcmp(name, "tie_at_boundary") == 0) track_type->flags |= TRACK_SEPARATE_TIE | TRACK_TIE_AT_BOUNDARY;
            else if (std::strcmp(name, "special_end_offsets") == 0) track_type->flags |= TRACK_SPECIAL_OFFSETS;
            else
            {
                std::printf("Error: Unrecognized flag \"%s\"\n", name);
                return 1;
            }
        }
    }

    json_t* groups = json_object_get(json, "sections");
    if (groups != nullptr)
    {
        if (!json_is_array(groups)) { std::printf("Error: Property \"sections\" is not an array\n"); return 1; }
        if (load_groups(groups, &track_type->groups)) return 1;
    }

    json_t* masks_json = json_object_get(json, "masks");
    if (masks_json != nullptr)
    {
        if (!json_is_string(masks_json)) { std::printf("Error: Property \"masks\" is not a string\n"); return 1; }
        if (load_masks(json_string_value(masks_json), track_type->masks)) return 1;
    }
    else if (!preloaded)
    {
        std::printf("Error: Property \"masks\" not found\n");
        return 1;
    }

    json_t* name = json_object_get(json, "name");
    if (name)
    {
        if (!json_is_string(name)) { std::printf("Error: Property \"name\" is not a string\n"); return 1; }
        track_type->suffix[0] = '_';
        std::strncpy(track_type->suffix + 1, json_string_value(name), 254);
        track_type->suffix[255] = '\0';
    }
    else if (!preloaded) track_type->suffix[0] = 0;

    if (track_type->flags & TRACK_HAS_LIFT)
    {
        json_t* offset = json_object_get(json, "lift_offset");
        if (offset)
        {
            if (!json_is_integer(offset)) { std::printf("Error: Property \"lift_offset\" is not an int\n"); return 1; }
            track_type->lift_offset = json_integer_value(offset);
        }
        else if (!preloaded) track_type->lift_offset = 13;
    }

    if (load_required_float(json, "length", &track_type->length, TILE_SIZE, preloaded)) return 1;
    if (load_float_with_default(json, "brake_length", &track_type->brake_length, TILE_SIZE, TILE_SIZE, preloaded)) return 1;

    if (track_type->flags & TRACK_TIE_AT_BOUNDARY)
    {
        if (load_required_float(json, "tie_length", &track_type->tie_length, TILE_SIZE, preloaded)) return 1;
    }

    if (load_required_float(json, "z_offset", &track_type->z_offset, 1, preloaded)) return 1;
    if (load_float_with_default(json, "support_spacing", &track_type->support_spacing, TILE_SIZE, TILE_SIZE, preloaded)) return 1;
    if (load_float_with_default(json, "pivot", &track_type->pivot, TILE_SIZE, 0, preloaded)) return 1;

    json_t* models = json_object_get(json, "models");
    if (models == nullptr || !json_is_object(models))
    {
        std::printf("Error: Property \"models\" not found or is not an object\n");
        return 1;
    }

    if (load_model(&track_type->mesh, models, "track"))
    {
        std::printf("Error: Track mesh not found\n");
        return 1;
    }
    if (load_model(&track_type->mask, models, "mask"))
    {
        mesh_destroy(&track_type->mesh);
        std::printf("Error: Mask mesh not found\n");
        return 1;
    }

    if (track_type->flags & TRACK_SEPARATE_TIE)
    {
        if (load_model(&track_type->tie_mesh, models, "tie"))
        {
            mesh_destroy(&track_type->mesh);
            mesh_destroy(&track_type->mask);
            std::printf("Error: separate tie mesh not found\n");
            return 1;
        }

        if (track_type->flags & TRACK_TIE_AT_BOUNDARY)
        {
            if (load_model(&track_type->mesh_tie, models, "track_tie"))
            {
                mesh_destroy(&track_type->mesh);
                mesh_destroy(&track_type->mask);
                mesh_destroy(&track_type->tie_mesh);
                std::printf("Error: track_tie mesh not found\n");
                return 1;
            }
        }
    }

    constexpr const char* support_model_names[NUM_MODELS] = {
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

    track_type->models_loaded = 0;
    for (int i = 0; i < NUM_MODELS; ++i)
    {
        int result = load_model(&track_type->models[i], models, support_model_names[i]);
        if (result == 0) track_type->models_loaded |= 1u << i;
        else if (result == 1)
        {
            mesh_destroy(&track_type->mesh);
            mesh_destroy(&track_type->mask);
            for (int j = 0; j < i; ++j) mesh_destroy(&track_type->models[j]);
            std::printf("Error: failed to load model %s\n", support_model_names[i]);
            return 1;
        }
    }

    return 0;
}

int load_vector(vector3_t* vector, json_t* array)
{
    const int size = json_array_size(array);
    if (size != 3) { std::printf("Vector must have 3 components\n"); return 1; }

    json_t* x = json_array_get(array, 0);
    json_t* y = json_array_get(array, 1);
    json_t* z = json_array_get(array, 2);

    if (!json_is_number(x) || !json_is_number(y) || !json_is_number(z))
    {
        std::printf("Vector components must be numeric\n");
        return 1;
    }
    vector->x = json_number_value(x);
    vector->y = json_number_value(y);
    vector->z = json_number_value(z);
    return 0;
}

int load_lights(light_t* lights, int* lights_count, json_t* json)
{
    const int num_lights = json_array_size(json);
    for (int i = 0; i < num_lights; ++i)
    {
        json_t* light = json_array_get(json, i);
        assert(light != nullptr);
        if (!json_is_object(light))
        {
            std::printf("Warning: Light array contains an element which is not an object-ignoring\n");
            continue;
        }

        json_t* type = json_object_get(light, "type");
        if (type == nullptr || !json_is_string(type))
        {
            std::printf("Error: Property \"type\" not found or is not a string\n");
            return 1;
        }

        const char* type_value = json_string_value(type);
        if (std::strcmp(type_value, "diffuse") == 0) lights[i].type = LIGHT_DIFFUSE;
        else if (std::strcmp(type_value, "specular") == 0) lights[i].type = LIGHT_SPECULAR;
        else std::printf("Unrecognized light type \"%s\"\n", type_value);

        json_t* shadow = json_object_get(light, "shadow");
        if (shadow == nullptr || !json_is_boolean(shadow))
        {
            std::printf("Error: Property \"shadow\" not found or is not a boolean\n");
            return 1;
        }
        lights[i].shadow = json_boolean_value(shadow) ? 1 : 0;

        json_t* direction = json_object_get(light, "direction");
        if (direction == nullptr || !json_is_array(direction))
        {
            std::printf("Error: Property \"direction\" not found or is not a direction\n");
            return 1;
        }
        if (load_vector(&lights[i].direction, direction)) return 1;
        lights[i].direction = lights[i].direction.normalized();

        json_t* strength = json_object_get(light, "strength");
        if (strength == nullptr || !json_is_number(strength))
        {
            std::printf("Error: Property \"strength\" not found or is not a number\n");
            return 1;
        }
        lights[i].intensity = json_number_value(strength);
    }
    *lights_count = num_lights;
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::printf("Usage: TrackRender <file>\n");
        return 1;
    }

    json_error_t error;
    json_t* json = json_load_file(argv[1], 0, &error);
    if (json == nullptr)
    {
        std::printf("Error: %s at line %d column %d\n", error.text, error.line, error.column);
        return 1;
    }

    const char* base_dir = nullptr;
    if (json_t* j = json_object_get(json, "base_directory"); j != nullptr && json_is_string(j)) base_dir = json_string_value(j);
    else std::printf("Error: No property \"base_directory\" found\n");

    const char* sprite_dir = nullptr;
    if (json_t* j = json_object_get(json, "sprite_directory"); j != nullptr && json_is_string(j)) sprite_dir = json_string_value(j);
    else std::printf("Error: No property \"sprite_directory\" found\n");

    const char* spritefile_in = nullptr;
    if (json_t* j = json_object_get(json, "spritefile_in"); j != nullptr && json_is_string(j)) spritefile_in = json_string_value(j);
    else std::printf("Error: No property \"spritefile_in\" found\n");

    const char* spritefile_out = nullptr;
    if (json_t* j = json_object_get(json, "spritefile_out"); j != nullptr && json_is_string(j)) spritefile_out = json_string_value(j);
    else std::printf("Error: No property \"spritefile_out\" found\n");

    int num_lights = 9;
    light_t lights[16] = {
        {LIGHT_DIFFUSE, 0, vector3(0.0f, -1.0f, 0.0f).normalized(), 0.25f},
        {LIGHT_DIFFUSE, 0, vector3(1.0f,  0.3f, 0.0f).normalized(), 0.32f},
        {LIGHT_SPECULAR, 0, vector3(1, 1, -1).normalized(), 1.0f},
        {LIGHT_DIFFUSE, 0, vector3(1, 0.65f, -1).normalized(), 0.8f},
        {LIGHT_DIFFUSE, 0, vector3(0.0f, 1.0f, 0.0f), 0.174f},
        {LIGHT_DIFFUSE, 0, vector3(-1.0f, 0.0f, 0.0f).normalized(), 0.15f},
        {LIGHT_DIFFUSE, 0, vector3(0.0f, 1.0f, 1.0f).normalized(), 0.2f},
        {LIGHT_DIFFUSE, 0, vector3(0.65f, 0.816f, -0.65f).normalized(), 0.25f},
        {LIGHT_DIFFUSE, 0, vector3(-1.0f, 0.0f, -1.0f).normalized(), 0.25f},
        {0, 0, {0, 0, 0}, 0}, {0, 0, {0, 0, 0}, 0}, {0, 0, {0, 0, 0}, 0},
        {0, 0, {0, 0, 0}, 0}, {0, 0, {0, 0, 0}, 0}, {0, 0, {0, 0, 0}, 0},
        {0, 0, {0, 0, 0}, 0},
    };

    if (json_t* light_array = json_object_get(json, "lights"); light_array != nullptr)
    {
        if (!json_is_array(light_array)) { std::printf("Error: Property \"lights\" is not an array\n"); return 1; }
        if (load_lights(lights, &num_lights, light_array)) return 1;
    }

    int dither = 1;
    if (json_t* dither_json = json_object_get(json, "dither"); dither_json != nullptr)
    {
        if (!json_is_true(dither_json) && !json_is_false(dither_json))
        {
            std::printf("Error: Property \"dither\" is not a boolean\n");
            return 1;
        }
        dither = json_is_true(dither_json);
    }

    json_t* tracks_json = json_object_get(json, "tracks");
    if (tracks_json == nullptr || !json_is_array(tracks_json))
    {
        std::printf("Error: Property \"tracks\" not found or is not an array\n");
        return 1;
    }

    float offset_table[88];
    if (json_t* offsets = json_object_get(json, "offsets"); offsets != nullptr)
    {
        if (!json_is_object(offsets)) { std::printf("Error: Property \"offsets\" is not an object\n"); return 1; }
        if (load_offsets(offsets, offset_table)) return 1;
    }
    else std::memset(offset_table, 0, 88 * sizeof(float));

    char full_path[256];
    std::snprintf(full_path, 256, "%s%s", base_dir, spritefile_in);
    json_t* sprites = json_load_file(full_path, 0, &error);
    if (sprites == nullptr)
    {
        std::printf("Error: %s in file %s line %d column %d\n", error.text, error.source, error.line, error.column);
        return 1;
    }

    context_t context = get_context(lights, num_lights, dither);

    track_type_t track_type;
    for (int i = 0; i < static_cast<int>(json_array_size(tracks_json)); ++i)
    {
        json_t* track = json_array_get(tracks_json, i);
        if (json_is_object(track) && load_track_type(&track_type, track, i != 0))
        {
            std::printf("Error loading track\n");
            json_decref(sprites);
            context_destroy(&context);
            return 1;
        }
        write_track_type(&context, &track_type, sprites, offset_table, base_dir, sprite_dir);
    }

    std::snprintf(full_path, 256, "%s%s", base_dir, spritefile_out);
    json_dump_file(sprites, full_path, JSON_INDENT(4));
    context_destroy(&context);

    return 0;
}
