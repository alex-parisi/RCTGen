#include "track.h"
#include "sprites.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <numbers>

namespace
{
vector3_t start_offset;
vector3_t end_offset;

constexpr vector3_t change_coordinates(vector3_t a) noexcept
{
    return {a.z, a.y, a.x};
}

struct track_transform_args_t
{
    track_point_t (*track_curve)(float);
    float scale;
    float offset;
    float z_offset;
    float length;
    int flags;
    float track_length;
};

track_point_t get_track_point(track_point_t (*curve)(float distance), int flags, float z_offset, float length, float u)
{
    track_point_t track_point;
    if (u < 0)
    {
        track_point = curve(0);
        track_point.position = track_point.position + track_point.tangent * u;
    }
    else if (u > length)
    {
        track_point = curve(length);
        track_point.position = track_point.position + track_point.tangent * (u - length);
    }
    else
    {
        track_point = curve(u);
    }

    if (flags & TRACK_DIAGONAL)   track_point.position.x += 0.5f * TILE_SIZE;
    if (flags & TRACK_DIAGONAL_2) track_point.position.z += 0.5f * TILE_SIZE;
    track_point.position.y += z_offset - 2 * CLEARANCE_HEIGHT;
    if (!(flags & TRACK_VERTICAL)) track_point.position.z -= 0.5f * TILE_SIZE;

    float v = u / length;
    v = std::clamp(v, 0.0f, 1.0f);

    track_point.position = track_point.position + start_offset * (2 * v * v * v - 3 * v * v + 1);
    track_point.position = track_point.position + end_offset   * (-2 * v * v * v + 3 * v * v);
    return track_point;
}

track_point_t only_yaw(track_point_t input)
{
    track_point_t output;
    output.position = input.position;
    output.normal = vector3(0, 1, 0);
    output.binormal = output.normal.cross(change_coordinates(input.tangent)).normalized();
    output.tangent = output.normal.cross(output.binormal).normalized();
    return output;
}

vertex_t track_transform(vector3_t vertex, vector3_t normal, const bool flat_shaded, void* data)
{
    const track_transform_args_t args = *static_cast<track_transform_args_t*>(data);

    vertex.z = args.scale * vertex.z + args.offset;

    const track_point_t track_point = get_track_point(args.track_curve, args.flags, args.z_offset, args.length, vertex.z);

    vertex_t out;
    out.vertex = change_coordinates(track_point.position
                                    + track_point.normal * vertex.y
                                    + track_point.binormal * vertex.x);

    if (flat_shaded)
    {
        const track_point_t central = get_track_point(args.track_curve, args.flags, args.z_offset, args.length, args.offset + args.track_length / 2);
        out.normal = change_coordinates(central.tangent * normal.z + central.normal * normal.y + central.binormal * normal.x);
    }
    else
    {
        out.normal = change_coordinates(track_point.tangent * normal.z + track_point.normal * normal.y + track_point.binormal * normal.x);
    }
    return out;
}

vertex_t base_transform(vector3_t vertex, vector3_t normal, const bool /*flat_shaded*/, void* data)
{
    const track_transform_args_t args = *static_cast<track_transform_args_t*>(data);

    vertex.z = args.scale * vertex.z + args.offset;

    track_point_t track_point = get_track_point(args.track_curve, args.flags, args.z_offset, args.length, vertex.z);

    track_point.binormal = vector3(0, 1, 0).cross(track_point.tangent).normalized();
    track_point.normal = track_point.tangent.cross(track_point.binormal).normalized();

    vertex_t out;
    out.vertex = change_coordinates(track_point.position + vector3(0, 1, 0) * vertex.y + track_point.binormal * vertex.x);
    out.normal = change_coordinates(track_point.tangent * normal.z + track_point.normal * normal.y + track_point.binormal * normal.x);
    return out;
}

inline constexpr int DENOM = 6;

constexpr int get_support_index(int bank)
{
    switch (bank)
    {
    case 0:                    return MODEL_FLAT;
    case -1: case 1:           return MODEL_BANK_SIXTH;
    case -2: case 2:           return MODEL_BANK_THIRD;
    case -3: case 3:           return MODEL_BANK_HALF;
    case -4: case 4:           return MODEL_BANK_TWO_THIRDS;
    case -5: case 5:           return MODEL_BANK_FIVE_SIXTHS;
    case -6: case 6:           return MODEL_BANK;
    }
    return MODEL_FLAT;
}

constexpr int get_special_index(int flags)
{
    switch (flags & TRACK_SPECIAL_MASK)
    {
    case TRACK_SPECIAL_STEEP_TO_VERTICAL: return MODEL_SPECIAL_STEEP_TO_VERTICAL;
    case TRACK_SPECIAL_VERTICAL_TO_STEEP: return MODEL_SPECIAL_VERTICAL_TO_STEEP;
    case TRACK_SPECIAL_VERTICAL:          return MODEL_SPECIAL_VERTICAL;
    case TRACK_SPECIAL_VERTICAL_TWIST_LEFT:
    case TRACK_SPECIAL_VERTICAL_TWIST_RIGHT: return MODEL_SPECIAL_VERTICAL_TWIST;
    case TRACK_SPECIAL_BARREL_ROLL_LEFT:
    case TRACK_SPECIAL_BARREL_ROLL_RIGHT:    return MODEL_SPECIAL_BARREL_ROLL;
    case TRACK_SPECIAL_HALF_LOOP:    return MODEL_SPECIAL_HALF_LOOP;
    case TRACK_SPECIAL_QUARTER_LOOP: return MODEL_SPECIAL_QUARTER_LOOP;
    case TRACK_SPECIAL_CORKSCREW_LEFT:
    case TRACK_SPECIAL_CORKSCREW_RIGHT: return MODEL_SPECIAL_CORKSCREW;
    case TRACK_SPECIAL_ZERO_G_ROLL_LEFT:
    case TRACK_SPECIAL_ZERO_G_ROLL_RIGHT: return MODEL_SPECIAL_ZERO_G_ROLL;
    case TRACK_SPECIAL_LARGE_ZERO_G_ROLL_LEFT:
    case TRACK_SPECIAL_LARGE_ZERO_G_ROLL_RIGHT: return MODEL_SPECIAL_LARGE_ZERO_G_ROLL;
    case TRACK_SPECIAL_BRAKE:          return MODEL_SPECIAL_BRAKE;
    case TRACK_SPECIAL_BLOCK_BRAKE:    return MODEL_SPECIAL_BLOCK_BRAKE;
    case TRACK_SPECIAL_MAGNETIC_BRAKE: return MODEL_SPECIAL_MAGNETIC_BRAKE;
    case TRACK_SPECIAL_BOOSTER:
    case TRACK_SPECIAL_LAUNCHED_LIFT:
    case TRACK_SPECIAL_VERTICAL_BOOSTER: return MODEL_SPECIAL_BOOSTER;
    }
    return 0;
}

void render_track_section(context_t* context, track_section_t* track_section, track_type_t* track_type, int view_flags, int track_mask, int rendered_views, image_t* images)
{
    int num_meshes = static_cast<int>(std::floor(0.5f + track_section->length / track_type->length));
    float scale = track_section->length / (num_meshes * track_type->length);

    if (track_type->models_loaded & (1 << MODEL_TRACK_ALT))
    {
        int num_meshes_even = 2 * static_cast<int>(std::floor(0.5f + track_section->length / (2 * track_type->length)));
        if (track_section->flags & TRACK_ALT_PREFER_ODD)
            num_meshes_even = 2 * static_cast<int>(std::floor(track_section->length / (2 * track_type->length))) + 1;
        const float scale_even = track_section->length / (num_meshes_even * track_type->length);
        if (scale_even > 0.9f && scale_even < 1.11111f)
        {
            num_meshes = num_meshes_even;
            scale = scale_even;
        }
    }

    const float length = scale * track_type->length;
    const float z_offset = (track_type->z_offset / 8.0f) * CLEARANCE_HEIGHT;

    mesh_t* mesh = &track_type->mesh;
    mesh_t* mesh_tie = &track_type->mesh_tie;

    context_begin_render(context);

    track_transform_args_t args;
    args.scale = scale;
    args.offset = -length;
    args.z_offset = z_offset;
    args.track_curve = track_section->curve;
    args.flags = track_section->flags;
    args.length = track_section->length;
    args.track_length = track_type->length;
    if (track_mask) context_add_model_transformed(context, &track_type->mask, track_transform, &args, 0);
    else if (!(view_flags & VIEW_EXTRUDE_BEHIND)) context_add_model_transformed(context, mesh, track_transform, &args, MESH_GHOST);
    args.offset = track_section->length;
    if (track_mask) context_add_model_transformed(context, &track_type->mask, track_transform, &args, 0);
    else if (!(view_flags & VIEW_EXTRUDE_IN_FRONT) && !(view_flags & VIEW_MASK_END))
        context_add_model_transformed(context, mesh, track_transform, &args, MESH_GHOST);

    if (track_type->flags & TRACK_TIE_AT_BOUNDARY)
    {
        int start_angle = 3;
        if (rendered_views & 1)      start_angle = 0;
        else if (rendered_views & 2) start_angle = 1;
        else if (rendered_views & 4) start_angle = 2;
        if (track_section->flags & (TRACK_DIAGONAL | TRACK_DIAGONAL_2)) start_angle = (start_angle + 1) % 4;
        int end_angle = start_angle;
        if (track_section->flags & TRACK_EXIT_90_DEG_LEFT)        end_angle -= 1;
        else if (track_section->flags & TRACK_EXIT_90_DEG_RIGHT)  end_angle += 1;
        else if (track_section->flags & TRACK_EXIT_180_DEG)       end_angle += 2;
        else if ((track_section->flags & TRACK_EXIT_45_DEG_LEFT) && (track_section->flags & (TRACK_DIAGONAL | TRACK_DIAGONAL_2)))   end_angle -= 1;
        else if ((track_section->flags & TRACK_EXIT_45_DEG_RIGHT) && !(track_section->flags & (TRACK_DIAGONAL | TRACK_DIAGONAL_2))) end_angle += 1;
        if (end_angle < 0) end_angle += 4;
        if (end_angle > 3) end_angle -= 4;

        const int start_tie = start_angle <= 1;
        const int end_tie = end_angle > 1;

        double corrected_length = num_meshes * track_type->length;
        if (!start_tie) corrected_length -= track_type->tie_length;
        if (end_tie)    corrected_length += track_type->tie_length;
        const double corrected_scale = track_section->length / corrected_length;

        const double tie_length = corrected_scale * track_type->tie_length;
        const double inter_length = corrected_scale * (track_type->length - track_type->tie_length);

        double offset = 0;

        if (view_flags & VIEW_EXTRUDE_BEHIND)
        {
            num_meshes++;
            offset -= ((view_flags & VIEW_EXTRUDE_BEHIND) ? 1 : 0) * corrected_scale * track_type->length;
        }
        if (view_flags & VIEW_EXTRUDE_IN_FRONT) num_meshes++;
        for (int i = 0; i < 2 * num_meshes + 1; ++i)
        {
            track_transform_args_t loop_args;
            loop_args.scale = corrected_scale;
            loop_args.offset = offset;
            loop_args.z_offset = z_offset;
            loop_args.track_curve = track_section->curve;
            loop_args.flags = track_section->flags;
            loop_args.length = track_section->length;
            loop_args.track_length = track_type->length;
            if ((!(i & 1)) && (i != 0 || start_tie) && (i != 2 * num_meshes || end_tie))
            {
                const track_point_t tp = get_track_point(track_section->curve, track_section->flags, z_offset, loop_args.length, loop_args.offset + track_type->tie_length / 2);
                context_add_model(
                    context, &track_type->tie_mesh,
                    transform(matrix(tp.binormal.z, tp.normal.z, tp.tangent.z,
                                     tp.binormal.y, tp.normal.y, tp.tangent.y,
                                     tp.binormal.x, tp.normal.x, tp.tangent.x),
                              change_coordinates(tp.position)),
                    track_mask);
                context_add_model_transformed(context, mesh_tie, track_transform, &loop_args, track_mask);
                offset += tie_length;
            }
            else if (i & 1)
            {
                int use_alt = i & 2;
                if (track_section->flags & TRACK_ALT_INVERT) use_alt = !use_alt;
                if (view_flags & VIEW_EXTRUDE_BEHIND)        use_alt = !use_alt;
                if (!(track_type->models_loaded & (1 << MODEL_TRACK_ALT))) use_alt = 0;
                if (use_alt) context_add_model_transformed(context, &track_type->models[MODEL_TRACK_ALT], track_transform, &loop_args, track_mask);
                else         context_add_model_transformed(context, mesh, track_transform, &loop_args, track_mask);
                if (track_mask)
                {
                    if (start_tie) loop_args.offset = offset - tie_length;
                    context_add_model_transformed(context, &track_type->mask, track_transform, &loop_args, 0);
                }
                offset += inter_length;
            }
        }
    }
    else
    {
        if (view_flags & VIEW_EXTRUDE_BEHIND)   num_meshes++;
        if (view_flags & VIEW_EXTRUDE_IN_FRONT) num_meshes++;
        if (view_flags & VIEW_MASK_END)         num_meshes++;
        for (int i = 0; i < num_meshes; ++i)
        {
            track_transform_args_t loop_args;
            loop_args.scale = scale;
            loop_args.offset = (i - ((view_flags & VIEW_EXTRUDE_BEHIND) ? 1 : 0)) * length;
            loop_args.z_offset = z_offset;
            loop_args.track_curve = track_section->curve;
            loop_args.flags = track_section->flags;
            loop_args.length = track_section->length;
            loop_args.track_length = track_type->length;

            const int alt_available = track_type->models_loaded & (1 << MODEL_TRACK_ALT);
            int use_alt = alt_available && (i & 1);
            if (alt_available && (track_section->flags & TRACK_ALT_INVERT)) use_alt = !use_alt;

            if (track_mask) context_add_model_transformed(context, &track_type->mask, track_transform, &loop_args, 0);
            if (use_alt) context_add_model_transformed(context, &track_type->models[MODEL_TRACK_ALT], track_transform, &loop_args, track_mask || (view_flags & VIEW_MASK_END && i + 1 == num_meshes));
            else         context_add_model_transformed(context, mesh, track_transform, &loop_args, track_mask || (view_flags & VIEW_MASK_END && i + 1 == num_meshes));
            if ((track_type->models_loaded & (1 << MODEL_BASE)) && (track_type->flags & TRACK_HAS_SUPPORTS) && !(track_section->flags & TRACK_NO_SUPPORTS))
                context_add_model_transformed(context, &track_type->models[MODEL_BASE], base_transform, &loop_args, track_mask);
            if (track_type->flags & TRACK_SEPARATE_TIE)
            {
                const track_point_t tp = get_track_point(track_section->curve, track_section->flags, z_offset, loop_args.length, loop_args.offset + 0.5f * length);
                context_add_model(context, &track_type->tie_mesh,
                                  transform(matrix(tp.binormal.z, tp.normal.z, tp.tangent.z,
                                                   tp.binormal.y, tp.normal.y, tp.tangent.y,
                                                   tp.binormal.x, tp.normal.x, tp.tangent.x),
                                            change_coordinates(tp.position)),
                                  track_mask || (view_flags & VIEW_MASK_END && i + 1 == num_meshes));
            }
        }
    }

    if (track_section->flags & TRACK_SPECIAL_MASK)
    {
        const int index = get_special_index(track_section->flags);
        if (track_type->models_loaded & (1 << index))
        {
            matrix_t mat = views[1];
            const std::uint32_t sp = track_section->flags & TRACK_SPECIAL_MASK;
            if (sp != TRACK_SPECIAL_VERTICAL_TWIST_RIGHT
                && sp != TRACK_SPECIAL_BARREL_ROLL_RIGHT
                && sp != TRACK_SPECIAL_CORKSCREW_RIGHT
                && sp != TRACK_SPECIAL_ZERO_G_ROLL_RIGHT
                && sp != TRACK_SPECIAL_LARGE_ZERO_G_ROLL_RIGHT)
            {
                mat.entries[6] *= -1;
                mat.entries[7] *= -1;
                mat.entries[8] *= -1;
            }

            if (sp == TRACK_SPECIAL_BRAKE || sp == TRACK_SPECIAL_MAGNETIC_BRAKE
                || sp == TRACK_SPECIAL_BLOCK_BRAKE || sp == TRACK_SPECIAL_BOOSTER)
            {
                float special_length = track_type->brake_length;
                if (sp == TRACK_SPECIAL_BLOCK_BRAKE) special_length = TILE_SIZE;
                const int num_special_meshes = static_cast<int>(std::floor(0.5f + track_section->length / special_length));
                const float special_scale = track_section->length / (num_special_meshes * special_length);
                special_length = special_scale * special_length;
                for (int i = 0; i < num_special_meshes; ++i)
                {
                    track_transform_args_t sp_args;
                    sp_args.scale = special_scale;
                    sp_args.offset = i * special_length;
                    sp_args.z_offset = z_offset;
                    sp_args.track_curve = track_section->curve;
                    sp_args.flags = track_section->flags;
                    sp_args.length = track_section->length;
                    sp_args.track_length = track_type->length;
                    context_add_model_transformed(context, &track_type->models[index], track_transform, &sp_args, track_mask);
                }
            }
            else
            {
                context_add_model(context, &track_type->models[index],
                                  transform(mat, vector3(!(track_section->flags & TRACK_VERTICAL) ? -0.5f * TILE_SIZE : 0, z_offset - 2 * CLEARANCE_HEIGHT, 0)),
                                  track_mask);
            }
        }
    }

    if ((track_type->flags & TRACK_HAS_SUPPORTS) && !(track_section->flags & TRACK_NO_SUPPORTS))
    {
        const int num_supports = static_cast<int>(std::floor(0.5f + track_section->length / track_type->support_spacing));
        const float support_step = track_section->length / num_supports;
        int entry = 0;
        int exit_a = 0;
        if (track_section->flags & TRACK_ENTRY_BANK_LEFT)        entry = DENOM;
        else if (track_section->flags & TRACK_ENTRY_BANK_RIGHT)  entry = -DENOM;

        if (track_section->flags & TRACK_EXIT_BANK_LEFT)         exit_a = DENOM;
        else if (track_section->flags & TRACK_EXIT_BANK_RIGHT)   exit_a = -DENOM;

        for (int i = 0; i < num_supports + 1; ++i)
        {
            const int u = (i * DENOM) / num_supports;
            const int bank_angle = (entry * (DENOM - u) + (exit_a * u)) / DENOM;

            const track_point_t tp = get_track_point(track_section->curve, track_section->flags, z_offset, track_section->length, i * support_step);
            const track_point_t support_point = only_yaw(tp);

            matrix_t rotation = matrix(support_point.binormal.x, support_point.normal.x, support_point.tangent.x,
                                       support_point.binormal.y, support_point.normal.y, support_point.tangent.y,
                                       support_point.binormal.z, support_point.normal.z, support_point.tangent.z);
            if (bank_angle >= 0) rotation = matrix_mult(views[2], rotation);

            vector3_t translation = change_coordinates(support_point.position);
            translation.y -= track_type->pivot / std::sqrt(tp.tangent.x * tp.tangent.x + tp.tangent.z * tp.tangent.z) - track_type->pivot;

            context_add_model(context, &track_type->models[get_support_index(bank_angle)], transform(rotation, translation), track_mask);
        }
    }

    context_finalize_render(context);

    for (int i = 0; i < 4; ++i)
    {
        if (rendered_views & (1 << i))
        {
            if (track_mask) context_render_silhouette(context, rotate_y(0.5f * i * std::numbers::pi_v<float>), images + i);
            else            context_render_view(context,      rotate_y(0.5f * i * std::numbers::pi_v<float>), images + i);
        }
    }
    context_end_render(context);
}

int is_in_mask(int x, int y, mask_t* mask)
{
    for (std::uint32_t i = 0; i < mask->num_rects; ++i)
    {
        if (x >= mask->rects[i].x_lower && x < mask->rects[i].x_upper
            && y >= mask->rects[i].y_lower && y < mask->rects[i].y_upper) return 1;
    }
    return 0;
}

int compare_vec(vector3_t vec1, vector3_t vec2, int rot)
{
    return (vec1 - matrix_vector(views[rot], vec2).normalized()).norm() < 0.15f;
}

int offset_table_index_with_rot(track_point_t track, int rot)
{
    const int banked = std::fabs(std::fabs(std::asin(std::sqrt(track.normal.x * track.normal.x + track.normal.z * track.normal.z))) - 0.25f * std::numbers::pi_v<float>) < 0.1f;
    const int right = (banked && track.binormal.y < 0) ? 0x10 : 0;
    if (compare_vec(track.tangent, vector3(0, 0, TILE_SIZE), rot))
    {
        if (track.normal.y < -0.9f) return OFFSET_INVERTED;
        if (banked) return right | OFFSET_BANK;
        return OFFSET_FLAT;
    }
    if (compare_vec(track.tangent, vector3(0, 2 * CLEARANCE_HEIGHT, TILE_SIZE), rot))
    {
        if (banked) return right | OFFSET_GENTLE_BANK;
        return OFFSET_GENTLE;
    }
    if (compare_vec(track.tangent, vector3(0, 8 * CLEARANCE_HEIGHT, TILE_SIZE), rot)) return OFFSET_STEEP;
    if (compare_vec(track.tangent, vector3(-TILE_SIZE, 0, TILE_SIZE), rot))
    {
        if (banked) return right | OFFSET_DIAGONAL_BANK;
        return OFFSET_DIAGONAL;
    }
    if (compare_vec(track.tangent, vector3(-TILE_SIZE, 2 * CLEARANCE_HEIGHT, TILE_SIZE), rot) && !banked) return OFFSET_DIAGONAL_GENTLE;
    if (compare_vec(track.tangent, vector3(-TILE_SIZE, 8 * CLEARANCE_HEIGHT, TILE_SIZE), rot)) return OFFSET_DIAGONAL_STEEP;
    return 0xFF;
}

int offset_table_index(track_point_t track)
{
    int index = offset_table_index_with_rot(track, 0);
    if (index != 0xFF) return index;
    index = offset_table_index_with_rot(track, 1);
    if (index != 0xFF) return 0x60 | index;
    index = offset_table_index_with_rot(track, 3);
    if (index != 0xFF) return 0x20 | index;
    return 0xFF;
}

vector3_t get_offset(int table, int view_angle, float* offset_table)
{
    const int index = table & 0xF;
    const int end_angle = table >> 5;
    const int right = (table & 0x10) >> 4;
    const int rotated_view_angle = (view_angle + end_angle + 2 * right) % 4;

    if (table == 0xFF) return vector3(0, 0, 0);

    vector3_t offset = vector3(0,
                               offset_table[8 * index + 2 * rotated_view_angle + 1] * CLEARANCE_HEIGHT / 8.0f,
                               offset_table[8 * index + 2 * rotated_view_angle] * TILE_SIZE / 32.0f);

    if (right) offset.z *= -1;

    if (index >= 6 && index <= 8)
    {
        offset.z *= std::numbers::sqrt2_v<float> / 2.0f;
        offset.x = offset.z;
    }

    if (end_angle != 0) offset = matrix_vector(rotate_y(-0.5f * std::numbers::pi_v<float> * end_angle), offset);

    return offset;
}

void set_offset(int view_angle, track_section_t* track_section, float* offset_table)
{
    const int start_table = offset_table_index(track_section->curve(0));
    const int end_table = offset_table_index(track_section->curve(track_section->length));

    start_offset = get_offset(start_table, view_angle, offset_table);
    end_offset = get_offset(end_table, view_angle, offset_table);
}

constexpr int mod(int k, int n) noexcept
{
    return ((k % n) + n) % n;
}

void apply_lift(image_t* image, image_t* pattern)
{
    for (int x = 0; x < image->width; ++x)
        for (int y = 0; y < image->height; ++y)
        {
            const std::uint8_t pixel = image->pixels[y * image->width + x];
            if (pixel >= 1 && pixel <= 3)
            {
                const int x_index = mod(x + image->x_offset - pattern->x_offset, pattern->width);
                const int y_index = mod(y + image->y_offset - pattern->y_offset, pattern->height);
                image->pixels[y * image->width + x] = pattern->pixels[x_index + y_index * pattern->width];
            }
        }
}

void write_track_section(context_t* context, int track_section_id, track_type_t* track_type, float offset_table[88], const char* base_directory, const char* output_directory, json_t* sprites)
{
    track_section_t* track_section = track_sections + track_section_id;
    view_t* views_arr = track_type->masks[track_section_id];

    const int z_offset = static_cast<int>(track_type->z_offset + 0.499999f);
    image_t full_sprites[4];
    image_t track_masks[4];
    for (int i = 0; i < 4; ++i)
    {
        if (views_arr[i].num_sprites == 0) continue;
        if (track_type->flags & TRACK_SPECIAL_OFFSETS) set_offset(i, track_section, offset_table);
        if (views_arr[i].flags & VIEW_NEEDS_TRACK_MASK)
            render_track_section(context, track_section, track_type, views_arr[i].flags, 1, 1 << i, track_masks);
        render_track_section(context, track_section, track_type, views_arr[i].flags, 0, 1 << i, full_sprites);
    }

    for (int i = 0; i < 4; ++i)
    {
        int angle = i;

        if (track_type->flags & TRACK_HAS_LIFT)
        {
            if (views_arr[i].num_sprites == 0) angle = i % 2;
        }

        if (views_arr[angle].num_sprites == 0) continue;

        if (track_type->flags & TRACK_HAS_LIFT && track_section->chain_pattern != nullptr)
            apply_lift(full_sprites + angle, track_section->chain_pattern + i);

        view_t* view = views_arr + angle;

        for (int sprite = 0; sprite < view->num_sprites; ++sprite)
        {
            char final_filename[512];
            char relative_filename[512];
            if (view->num_sprites == 1)
                std::snprintf(relative_filename, 512, "%s%s%s_%d.png", output_directory, track_section->name, track_type->suffix, i + 1);
            else
                std::snprintf(relative_filename, 512, "%s%s%s_%d_%d.png", output_directory, track_section->name, track_type->suffix, i + 1, sprite + 1);
            std::snprintf(final_filename, 512, "%s%s", base_directory, relative_filename);
            std::printf("%s\n", final_filename);

            image_t part_sprite;
            image_copy(full_sprites + angle, &part_sprite);
            if (view->masks != nullptr)
            {
                for (int x = 0; x < full_sprites[angle].width; ++x)
                    for (int y = 0; y < full_sprites[angle].height; ++y)
                    {
                        int in_mask = is_in_mask(x + full_sprites[angle].x_offset,
                                                 y + full_sprites[angle].y_offset + ((track_section->flags & TRACK_OFFSET_SPRITE_MASK) ? (z_offset - 8) : 0),
                                                 view->masks + sprite);

                        if (view->masks[sprite].track_mask_op != TRACK_MASK_NONE)
                        {
                            const int mask_x = (x + full_sprites[angle].x_offset) - track_masks[angle].x_offset;
                            const int mask_y = (y + full_sprites[angle].y_offset) - track_masks[angle].y_offset;
                            const int in_track_mask = mask_x >= 0 && mask_y >= 0
                                                       && mask_x < track_masks[angle].width
                                                       && mask_y < track_masks[angle].height
                                                       && track_masks[angle].pixels[mask_x + mask_y * track_masks[angle].width] != 0;

                            switch (view->masks[sprite].track_mask_op)
                            {
                            case TRACK_MASK_DIFFERENCE: in_mask = in_mask && !in_track_mask; break;
                            case TRACK_MASK_INTERSECT:  in_mask = in_mask &&  in_track_mask; break;
                            case TRACK_MASK_TRANSFER_NEXT:
                                if (sprite < view->num_sprites - 1 && in_track_mask
                                    && is_in_mask(x + full_sprites[angle].x_offset,
                                                  y + full_sprites[angle].y_offset + ((track_section->flags & TRACK_OFFSET_SPRITE_MASK) ? (z_offset - 8) : 0),
                                                  view->masks + sprite + 1)) in_mask = 1;
                                break;
                            }
                        }

                        if (in_mask)
                            part_sprite.pixels[x + part_sprite.width * y] = full_sprites[angle].pixels[x + full_sprites[angle].width * y];
                        else
                            part_sprite.pixels[x + part_sprite.width * y] = 0;
                    }
                part_sprite.x_offset += view->masks[sprite].x_offset;
                part_sprite.y_offset += view->masks[sprite].y_offset;
            }

            std::FILE* file = std::fopen(final_filename, "wb");
            if (file == nullptr)
            {
                std::printf("Error: could not open %s for writing\n", final_filename);
                std::exit(1);
            }
            image_crop(&part_sprite);
            image_write_png(&part_sprite, nullptr, file);
            std::fclose(file);

            json_t* sprite_entry = json_object();
            json_object_set(sprite_entry, "path", json_string(relative_filename));
            json_object_set(sprite_entry, "x", json_integer(part_sprite.x_offset));
            json_object_set(sprite_entry, "y", json_integer(part_sprite.y_offset));
            json_object_set(sprite_entry, "palette", json_string("keep"));
            json_array_append(sprites, sprite_entry);
            image_destroy(&part_sprite);
        }
    }
}

} // namespace

int write_track_type(context_t* context, track_type_t* track_type, json_t* sprites, float offset_table[88], const char* base_dir, const char* output_dir)
{
    const std::uint64_t groups = track_type->groups;

    if (groups & TRACK_GROUP_FLAT)            write_track_section(context, FLAT, track_type, offset_table, base_dir, output_dir, sprites);
    if (groups & TRACK_GROUP_BRAKES)          write_track_section(context, BRAKE, track_type, offset_table, base_dir, output_dir, sprites);
    if (groups & TRACK_GROUP_BLOCK_BRAKES)    write_track_section(context, BLOCK_BRAKE, track_type, offset_table, base_dir, output_dir, sprites);
    if (groups & TRACK_GROUP_SLOPED_BRAKES)   write_track_section(context, BRAKE_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
    if (groups & TRACK_GROUP_MAGNETIC_BRAKES) write_track_section(context, MAGNETIC_BRAKE, track_type, offset_table, base_dir, output_dir, sprites);

    if (groups & TRACK_GROUP_BOOSTERS)          write_track_section(context, BOOSTER, track_type, offset_table, base_dir, output_dir, sprites);
    if (groups & TRACK_GROUP_LAUNCHED_LIFTS)    write_track_section(context, LAUNCHED_LIFT, track_type, offset_table, base_dir, output_dir, sprites);
    if (groups & TRACK_GROUP_VERTICAL_BOOSTERS) write_track_section(context, VERTICAL_BOOSTER, track_type, offset_table, base_dir, output_dir, sprites);

    if (groups & TRACK_GROUP_GENTLE_SLOPES)
    {
        write_track_section(context, FLAT_TO_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_TO_FLAT, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
    }
    if (groups & TRACK_GROUP_MAGNETIC_BRAKES) write_track_section(context, MAGNETIC_BRAKE_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);

    if (groups & TRACK_GROUP_STEEP_SLOPES)
    {
        write_track_section(context, GENTLE_TO_STEEP, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, STEEP_TO_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, STEEP, track_type, offset_table, base_dir, output_dir, sprites);
    }

    if (groups & TRACK_GROUP_VERTICAL_SLOPES)
    {
        write_track_section(context, STEEP_TO_VERTICAL, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, VERTICAL_TO_STEEP, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, VERTICAL, track_type, offset_table, base_dir, output_dir, sprites);
    }

    if (groups & TRACK_GROUP_TURNS)
    {
        write_track_section(context, SMALL_TURN_LEFT, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, MEDIUM_TURN_LEFT, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, LARGE_TURN_LEFT_TO_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, LARGE_TURN_RIGHT_TO_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
    }

    if (groups & TRACK_GROUP_DIAGONALS) write_track_section(context, FLAT_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
    if (groups & TRACK_GROUP_DIAGONAL_BRAKES)
    {
        if (groups & TRACK_GROUP_BRAKES)          write_track_section(context, BRAKE_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        if (groups & TRACK_GROUP_BLOCK_BRAKES)    write_track_section(context, BLOCK_BRAKE_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        if (groups & TRACK_GROUP_MAGNETIC_BRAKES) write_track_section(context, MAGNETIC_BRAKE_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
    }
    if ((groups & TRACK_GROUP_DIAGONALS) && (groups & TRACK_GROUP_GENTLE_SLOPES))
    {
        write_track_section(context, FLAT_TO_GENTLE_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_TO_FLAT_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
    }
    if (groups & TRACK_GROUP_DIAGONAL_BRAKES)
    {
        if (groups & TRACK_GROUP_SLOPED_BRAKES)   write_track_section(context, BRAKE_GENTLE_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        if (groups & TRACK_GROUP_MAGNETIC_BRAKES) write_track_section(context, MAGNETIC_BRAKE_GENTLE_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
    }
    if ((groups & TRACK_GROUP_DIAGONALS) && (groups & TRACK_GROUP_STEEP_SLOPES))
    {
        write_track_section(context, GENTLE_TO_STEEP_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, STEEP_TO_GENTLE_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, STEEP_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
    }

    if (groups & TRACK_GROUP_BANKED_TURNS)
    {
        write_track_section(context, FLAT_TO_LEFT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, FLAT_TO_RIGHT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, LEFT_BANK_TO_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, RIGHT_BANK_TO_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_TO_LEFT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_TO_RIGHT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, LEFT_BANK, track_type, offset_table, base_dir, output_dir, sprites);

        if (groups & TRACK_GROUP_DIAGONALS)
        {
            write_track_section(context, FLAT_TO_LEFT_BANK_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
            write_track_section(context, FLAT_TO_RIGHT_BANK_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
            write_track_section(context, LEFT_BANK_TO_GENTLE_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
            write_track_section(context, RIGHT_BANK_TO_GENTLE_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
            write_track_section(context, GENTLE_TO_LEFT_BANK_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
            write_track_section(context, GENTLE_TO_RIGHT_BANK_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
            write_track_section(context, LEFT_BANK_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        }

        write_track_section(context, SMALL_TURN_LEFT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, MEDIUM_TURN_LEFT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, LARGE_TURN_LEFT_TO_DIAG_BANK, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, LARGE_TURN_RIGHT_TO_DIAG_BANK, track_type, offset_table, base_dir, output_dir, sprites);
    }

    if (groups & TRACK_GROUP_SLOPED_TURNS && (groups & TRACK_GROUP_GENTLE_SLOPES))
    {
        write_track_section(context, SMALL_TURN_LEFT_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, SMALL_TURN_RIGHT_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, MEDIUM_TURN_LEFT_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, MEDIUM_TURN_RIGHT_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
    }
    if ((groups & TRACK_GROUP_STEEP_SLOPED_TURNS) && (groups & TRACK_GROUP_STEEP_SLOPES))
    {
        write_track_section(context, VERY_SMALL_TURN_LEFT_STEEP, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, VERY_SMALL_TURN_RIGHT_STEEP, track_type, offset_table, base_dir, output_dir, sprites);
    }
    if ((groups & TRACK_GROUP_SLOPED_TURNS) && (groups & TRACK_GROUP_VERTICAL_SLOPES))
    {
        write_track_section(context, VERTICAL_TWIST_LEFT, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, VERTICAL_TWIST_RIGHT, track_type, offset_table, base_dir, output_dir, sprites);
    }

    if (groups & TRACK_GROUP_BANKED_SLOPED_TURNS)
    {
        write_track_section(context, GENTLE_TO_GENTLE_LEFT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_TO_GENTLE_RIGHT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_LEFT_BANK_TO_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_RIGHT_BANK_TO_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, LEFT_BANK_TO_GENTLE_LEFT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, RIGHT_BANK_TO_GENTLE_RIGHT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_LEFT_BANK_TO_LEFT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_RIGHT_BANK_TO_RIGHT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_LEFT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_RIGHT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, FLAT_TO_GENTLE_LEFT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, FLAT_TO_GENTLE_RIGHT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_LEFT_BANK_TO_FLAT, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_RIGHT_BANK_TO_FLAT, track_type, offset_table, base_dir, output_dir, sprites);

        write_track_section(context, SMALL_TURN_LEFT_BANK_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, SMALL_TURN_RIGHT_BANK_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, MEDIUM_TURN_LEFT_BANK_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, MEDIUM_TURN_RIGHT_BANK_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
    }

    if (groups & TRACK_GROUP_S_BENDS)
    {
        write_track_section(context, S_BEND_LEFT, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, S_BEND_RIGHT, track_type, offset_table, base_dir, output_dir, sprites);
    }
    if (groups & TRACK_GROUP_BANKED_S_BENDS)
    {
        write_track_section(context, S_BEND_LEFT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, S_BEND_RIGHT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
    }

    if (groups & TRACK_GROUP_HELICES)
    {
        write_track_section(context, SMALL_HELIX_LEFT, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, SMALL_HELIX_RIGHT, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, MEDIUM_HELIX_LEFT, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, MEDIUM_HELIX_RIGHT, track_type, offset_table, base_dir, output_dir, sprites);
    }

    if (groups & TRACK_GROUP_STEEP_BANK_TRANSITIONS)
    {
        write_track_section(context, GENTLE_LEFT_BANK_TO_STEEP, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_RIGHT_BANK_TO_STEEP, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, STEEP_TO_GENTLE_LEFT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, STEEP_TO_GENTLE_RIGHT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_LEFT_BANK_TO_STEEP_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_RIGHT_BANK_TO_STEEP_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, STEEP_TO_GENTLE_LEFT_BANK_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, STEEP_TO_GENTLE_RIGHT_BANK_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
    }

    if (groups & TRACK_GROUP_LARGE_STEEP_SLOPED_TURNS)
    {
        std::printf("Here\n");
        write_track_section(context, SMALL_TURN_LEFT_STEEP, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, SMALL_TURN_RIGHT_STEEP, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, LARGE_TURN_LEFT_TO_DIAG_STEEP, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, LARGE_TURN_RIGHT_TO_DIAG_STEEP, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, LARGE_TURN_LEFT_TO_ORTHOGONAL_STEEP, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, LARGE_TURN_RIGHT_TO_ORTHOGONAL_STEEP, track_type, offset_table, base_dir, output_dir, sprites);
    }

    if (groups & TRACK_GROUP_INLINE_TWISTS)
    {
        write_track_section(context, INLINE_TWIST_LEFT, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, INLINE_TWIST_RIGHT, track_type, offset_table, base_dir, output_dir, sprites);
    }
    if (groups & TRACK_GROUP_BANKED_INLINE_TWISTS)
    {
        write_track_section(context, INLINE_TWIST_LEFT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, INLINE_TWIST_RIGHT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
    }
    if (groups & TRACK_GROUP_BARREL_ROLLS)
    {
        write_track_section(context, BARREL_ROLL_LEFT, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, BARREL_ROLL_RIGHT, track_type, offset_table, base_dir, output_dir, sprites);
    }
    if (groups & TRACK_GROUP_BANKED_BARREL_ROLLS)
    {
        write_track_section(context, BARREL_ROLL_LEFT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, BARREL_ROLL_RIGHT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
    }
    if (groups & TRACK_GROUP_HALF_LOOPS)     write_track_section(context, HALF_LOOP, track_type, offset_table, base_dir, output_dir, sprites);
    if (groups & TRACK_GROUP_VERTICAL_LOOPS)
    {
        write_track_section(context, VERTICAL_LOOP_LEFT, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, VERTICAL_LOOP_RIGHT, track_type, offset_table, base_dir, output_dir, sprites);
    }
    if (groups & TRACK_GROUP_LARGE_SLOPE_TRANSITIONS)
    {
        write_track_section(context, FLAT_TO_STEEP, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, STEEP_TO_FLAT, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, FLAT_TO_STEEP_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, STEEP_TO_FLAT_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
    }
    if (groups & TRACK_GROUP_QUARTER_LOOPS) write_track_section(context, QUARTER_LOOP, track_type, offset_table, base_dir, output_dir, sprites);
    if (groups & TRACK_GROUP_CORKSCREWS)
    {
        write_track_section(context, CORKSCREW_LEFT, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, CORKSCREW_RIGHT, track_type, offset_table, base_dir, output_dir, sprites);
    }
    if (groups & TRACK_GROUP_LARGE_CORKSCREWS)
    {
        write_track_section(context, LARGE_CORKSCREW_LEFT, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, LARGE_CORKSCREW_RIGHT, track_type, offset_table, base_dir, output_dir, sprites);
    }
    if (groups & TRACK_GROUP_TURN_BANK_TRANSITIONS)
    {
        write_track_section(context, SMALL_TURN_LEFT_BANK_TO_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, SMALL_TURN_RIGHT_BANK_TO_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
    }

    if (groups & TRACK_GROUP_MEDIUM_HALF_LOOPS)
    {
        write_track_section(context, MEDIUM_HALF_LOOP_LEFT, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, MEDIUM_HALF_LOOP_RIGHT, track_type, offset_table, base_dir, output_dir, sprites);
    }
    if (groups & TRACK_GROUP_LARGE_HALF_LOOPS)
    {
        write_track_section(context, LARGE_HALF_LOOP_LEFT, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, LARGE_HALF_LOOP_RIGHT, track_type, offset_table, base_dir, output_dir, sprites);
    }
    if (groups & TRACK_GROUP_ZERO_G_ROLLS)
    {
        write_track_section(context, ZERO_G_ROLL_LEFT, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, ZERO_G_ROLL_RIGHT, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, LARGE_ZERO_G_ROLL_LEFT, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, LARGE_ZERO_G_ROLL_RIGHT, track_type, offset_table, base_dir, output_dir, sprites);
    }
    if (groups & TRACK_GROUP_BANKED_ZERO_G_ROLLS)
    {
        write_track_section(context, ZERO_G_ROLL_LEFT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, ZERO_G_ROLL_RIGHT_BANK, track_type, offset_table, base_dir, output_dir, sprites);
    }
    if (groups & TRACK_GROUP_DIVE_LOOPS)
    {
        write_track_section(context, DIVE_LOOP_45_LEFT, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, DIVE_LOOP_45_RIGHT, track_type, offset_table, base_dir, output_dir, sprites);
    }

    if (groups & TRACK_GROUP_SMALL_SLOPE_TRANSITIONS)
    {
        write_track_section(context, SMALL_FLAT_TO_STEEP, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, SMALL_STEEP_TO_FLAT, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, SMALL_FLAT_TO_STEEP_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, SMALL_STEEP_TO_FLAT_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
    }

    if (groups & TRACK_GROUP_LARGE_SLOPED_TURNS)
    {
        write_track_section(context, LARGE_TURN_LEFT_TO_DIAG_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, LARGE_TURN_RIGHT_TO_DIAG_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, LARGE_TURN_LEFT_TO_ORTHOGONAL_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, LARGE_TURN_RIGHT_TO_ORTHOGONAL_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
    }

    if (groups & TRACK_GROUP_LARGE_BANKED_SLOPED_TURNS)
    {
        write_track_section(context, GENTLE_TO_GENTLE_LEFT_BANK_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_TO_GENTLE_RIGHT_BANK_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_LEFT_BANK_TO_GENTLE_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_RIGHT_BANK_TO_GENTLE_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, LEFT_BANK_TO_GENTLE_LEFT_BANK_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, RIGHT_BANK_TO_GENTLE_RIGHT_BANK_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_LEFT_BANK_TO_LEFT_BANK_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_RIGHT_BANK_TO_RIGHT_BANK_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_LEFT_BANK_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_RIGHT_BANK_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, FLAT_TO_GENTLE_LEFT_BANK_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, FLAT_TO_GENTLE_RIGHT_BANK_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_LEFT_BANK_TO_FLAT_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, GENTLE_RIGHT_BANK_TO_FLAT_DIAG, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, LARGE_TURN_LEFT_BANK_TO_DIAG_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, LARGE_TURN_RIGHT_BANK_TO_DIAG_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, LARGE_TURN_LEFT_BANK_TO_ORTHOGONAL_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
        write_track_section(context, LARGE_TURN_RIGHT_BANK_TO_ORTHOGONAL_GENTLE, track_type, offset_table, base_dir, output_dir, sprites);
    }
    return 0;
}
