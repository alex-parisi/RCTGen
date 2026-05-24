#include "renderer.h"
#include "model.h"
#include "palette.h"
#include "vectormath.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <numbers>
#include <vector>

// Coordinate-system constants for the dimetric projection.
namespace
{
inline constexpr float SQRT_2  = 1.4142135623731f;
inline constexpr float SQRT1_2 = 0.707106781f;
inline constexpr float SQRT_3  = 1.73205080757f;
inline constexpr float SQRT_6  = 2.44948974278f;

inline constexpr int AO_NUM_SAMPLES_U = 8;
inline constexpr int AO_NUM_SAMPLES_V = 4;
inline constexpr int AA_NUM_SAMPLES_U = 4;
inline constexpr int AA_NUM_SAMPLES_V = 4;
inline constexpr float AA_SAMPLE_WEIGHT = 1.0f / (AA_NUM_SAMPLES_U * AA_NUM_SAMPLES_V);
}

std::array<matrix_t, 4> views{{
    {{1, 0, 0, 0, 1, 0, 0, 0, 1}},
    {{0, 0, 1, 0, 1, 0, -1, 0, 0}},
    {{-1, 0, 0, 0, 1, 0, 0, 0, -1}},
    {{0, 0, -1, 0, 1, 0, 1, 0, 0}},
}};

void context_init(context_t* context, light_t* lights, std::uint32_t num_lights, std::uint32_t dither, palette_t palette, float upt)
{
    context->rt_device = device_init();
    context->lights = lights;
    context->num_lights = num_lights;
    context->dither = dither;
    context->projection = matrix_t{{
        32.0f / upt, 0.0f, -32.0f / upt,
        -16.0f / upt, -16.0f * SQRT_6 / upt, -16.0f / upt,
        16.0f * SQRT_3 / upt, -16.0f * SQRT_2 / upt, 16.0f * SQRT_3 / upt,
    }};
    context->palette = palette;
}

void context_begin_render(context_t* context)
{
    scene_init(&context->rt_scene, context->rt_device);
}

static vertex_t linear_transform(vector3_t vertex, vector3_t normal, const bool /*flat_shaded*/, void* matptr)
{
    const transform_t trans = *static_cast<transform_t*>(matptr);
    vertex_t out;
    out.vertex = transform_vector(trans, vertex);
    out.normal = matrix_vector(trans.matrix, normal).normalized();
    return out;
}

void context_add_model_transformed(context_t* context, mesh_t* mesh, vertex_t (*transform)(vector3_t, vector3_t, bool, void*), void* data, int mask)
{
    scene_add_model(&context->rt_scene, mesh, transform, data, mask);
}

void context_add_model(context_t* context, mesh_t* mesh, transform_t trans, int mask)
{
    scene_add_model(&context->rt_scene, mesh, &linear_transform, &trans, mask);
}

void context_finalize_render(context_t* context)
{
    scene_finalize(&context->rt_scene);
}

void context_end_render(context_t* context)
{
    scene_destroy(&context->rt_scene);
}

void context_destroy(context_t* context)
{
    device_destroy(context->rt_device);
}

namespace
{

// Specular shading helper (translated from the original Blender code).
constexpr float spec(float inp, int hard) noexcept
{
    if (inp >= 1.0f) return 1.0f;
    if (inp <= 0.0f) return 0.0f;

    float b1 = inp * inp;
    if (b1 < 0.01f) b1 = 0.01f;

    if ((hard & 1) == 0) inp = 1.0f;
    if (hard & 2)   inp *= b1;
    b1 *= b1;
    if (hard & 4)   inp *= b1;
    b1 *= b1;
    if (hard & 8)   inp *= b1;
    b1 *= b1;
    if (hard & 16)  inp *= b1;
    b1 *= b1;

    if (b1 < 0.001f) b1 = 0.0f;

    if (hard & 32)  inp *= b1;
    b1 *= b1;
    if (hard & 64)  inp *= b1;
    b1 *= b1;
    if (hard & 128) inp *= b1;

    if (b1 < 0.001f) b1 = 0.0f;

    if (hard & 256)
    {
        b1 *= b1;
        inp *= b1;
    }
    return inp;
}

constexpr float vector3_dot_clamped(vector3_t a, vector3_t b) noexcept
{
    return std::max(a.dot(b), 0.0f);
}

vector3_t shade_fragment(scene_t* scene, vector3_t pos, vector3_t normal, vector3_t view, vector3_t color,
                         vector3_t specular_color, float specular_exponent, vector3_t ambient_color,
                         light_t* lights, std::uint32_t num_lights)
{
    vector3_t output_color = vector3(0, 0, 0);

    for (std::uint32_t i = 0; i < num_lights; ++i)
    {
        if (lights[i].shadow && scene_trace_occlusion_ray(scene, pos, lights[i].direction)) continue;
        if (lights[i].type == LIGHT_HEMI)
        {
            const float diffuse_factor = 0.5f * lights[i].intensity * (1 + normal.dot(lights[i].direction));
            output_color = output_color + color * diffuse_factor;
        }
        else if (lights[i].type == LIGHT_DIFFUSE)
        {
            const float diffuse_factor = lights[i].intensity * vector3_dot_clamped(normal, lights[i].direction);
            output_color = output_color + color * diffuse_factor;
        }
        else
        {
            const vector3_t reflected = normal * (2.0f * lights[i].direction.dot(normal)) - lights[i].direction;
            const float specular_factor = lights[i].intensity * std::pow(vector3_dot_clamped(reflected, view), specular_exponent);
            output_color = output_color + specular_color * specular_factor;
        }
    }
    return output_color + ambient_color;
}

int scene_sample_point(scene_t* scene, vector2_t point, matrix_t camera, light_t* lights, std::uint32_t num_lights, fragment_t* fragment)
{
    ray_hit_t hit;
    vector3_t view_vector = matrix_vector(camera, vector3(0, 0, -1));
    if (scene_trace_ray(scene, matrix_vector(camera, vector3(point.x, point.y, -512)), view_vector * -1, &hit))
    {
        view_vector = view_vector.normalized();
        mesh_t* mesh = scene->meshes[hit.mesh_index];
        face_t* face = mesh->faces + hit.face_index;
        material_t* material = mesh->materials + face->material;

        if (scene_is_mask(scene, hit.mesh_index) || material->flags & MATERIAL_IS_MASK)
        {
            fragment->color = vector3(0, 1, 0);
            fragment->depth = hit.distance;
            fragment->flags = static_cast<std::uint8_t>(material->flags | MATERIAL_IS_MASK);
            fragment->region = FRAGMENT_UNUSED;
            return 1;
        }

        vector3_t color;
        if (material->flags & MATERIAL_HAS_TEXTURE)
        {
            const vector2_t tex_coord =
                mesh->uvs[face->indices[0]] * (1.0f - hit.u - hit.v)
                + mesh->uvs[face->indices[1]] * hit.u
                + mesh->uvs[face->indices[2]] * hit.v;
            color = texture_sample(&material->texture, tex_coord);
        }
        else
        {
            color = material->color;
        }

        if (material->flags & MATERIAL_IS_REMAPPABLE)
        {
            const float intensity = std::max({color.x, color.y, color.z});
            color = vector3_t::splat(intensity);
        }

        const vector3_t shaded = shade_fragment(scene, hit.position, hit.normal, view_vector, color,
                                                material->specular_color, material->specular_exponent,
                                                material->ambient_color, lights, num_lights);

        const vector3_t normal = hit.normal;
        vector3_t tangent;
        if (std::fabs(normal.x) > std::fabs(normal.y))
            tangent = vector3(normal.z, 0, -normal.x) * (1.0f / std::sqrt(normal.x * normal.x + normal.z * normal.z));
        else
            tangent = vector3(0, -normal.z, normal.y) * (1.0f / std::sqrt(normal.y * normal.y + normal.z * normal.z));
        const vector3_t bitangent = normal.cross(tangent);

        float ao_factor = 1.0f;
        if (!(material->flags & MATERIAL_NO_AO))
        {
            std::uint32_t not_occluded_samples = 0;
            for (int i = 0; i < AO_NUM_SAMPLES_U; ++i)
                for (int j = 0; j < AO_NUM_SAMPLES_V; ++j)
                {
                    const float theta = 2 * std::numbers::pi_v<float> * ((i + static_cast<float>(std::rand()) / RAND_MAX) / AO_NUM_SAMPLES_U);
                    const float phi = std::asin(1 - ((j + static_cast<float>(std::rand()) / RAND_MAX) / AO_NUM_SAMPLES_V));

                    const vector3_t local = vector3(std::cos(phi) * std::sin(theta), std::cos(phi) * std::cos(theta), std::sin(phi));
                    const vector3_t sample_dir = normal * local.z + tangent * local.x + bitangent * local.y;
                    if (!scene_trace_occlusion_ray(scene, hit.position, sample_dir)) not_occluded_samples++;
                }
            ao_factor = static_cast<float>(not_occluded_samples) / (AO_NUM_SAMPLES_U * AO_NUM_SAMPLES_V);
        }

        fragment->color = shaded * ao_factor;
        fragment->depth = hit.distance;
        fragment->ghost_depth = hit.ghost_distance;
        fragment->flags = static_cast<std::uint8_t>(material->flags);
        fragment->region = material->region;
        return 1;
    }
    fragment->ghost_depth = hit.ghost_distance;
    return 0;
}

int scene_sample_material(scene_t* scene, vector2_t point, matrix_t camera, material_t** material_out, float* depth_out, float* ghost_depth_out, int* is_mask)
{
    ray_hit_t hit;
    vector3_t view_vector = matrix_vector(camera, vector3(0, 0, -1));

    if (scene_trace_ray(scene, matrix_vector(camera, vector3(point.x, point.y, -512)), view_vector * -1, &hit))
    {
        mesh_t* mesh = scene->meshes[hit.mesh_index];
        face_t* face = mesh->faces + hit.face_index;
        material_t* material = mesh->materials + face->material;

        *is_mask = scene_is_mask(scene, hit.mesh_index) || (material->flags & MATERIAL_IS_MASK);
        *material_out = material;
        *depth_out = hit.distance;
        *ghost_depth_out = hit.ghost_distance;
        return 1;
    }
    *ghost_depth_out = hit.ghost_distance;
    return 0;
}

constexpr rect_t make_rect(int xl, int xu, int yl, int yu) noexcept
{
    return {xl, yl, xu, yu};
}

rect_t rect_enclose_point(rect_t r, float x, float y) noexcept
{
    return make_rect(static_cast<int>(std::min<float>(r.x_lower, std::floor(x))),
                     static_cast<int>(std::max<float>(r.x_upper, std::ceil(x))),
                     static_cast<int>(std::min<float>(r.y_lower, std::floor(y))),
                     static_cast<int>(std::max<float>(r.y_upper, std::ceil(y))));
}

rect_t scene_get_bounds(scene_t* scene, matrix_t camera)
{
    const vector3_t bounding_points[8] = {
        vector3(scene->x_min, scene->y_min, scene->z_min),
        vector3(scene->x_max, scene->y_min, scene->z_min),
        vector3(scene->x_min, scene->y_max, scene->z_min),
        vector3(scene->x_max, scene->y_max, scene->z_min),
        vector3(scene->x_min, scene->y_min, scene->z_max),
        vector3(scene->x_max, scene->y_min, scene->z_max),
        vector3(scene->x_min, scene->y_max, scene->z_max),
        vector3(scene->x_max, scene->y_max, scene->z_max),
    };

    rect_t bounds = make_rect(static_cast<int>(std::floor(bounding_points[0].x)),
                              static_cast<int>(std::ceil(bounding_points[0].x)),
                              static_cast<int>(std::floor(bounding_points[0].y)),
                              static_cast<int>(std::ceil(bounding_points[0].y)));
    for (const auto& p : bounding_points)
    {
        const vector3_t screen_point = matrix_vector(camera, p);
        bounds = rect_enclose_point(bounds, screen_point.x, screen_point.y);
    }
    bounds.x_lower--;
    bounds.x_upper++;
    bounds.y_lower--;
    bounds.y_upper++;
    return bounds;
}

inline fragment_t& framebuffer_at(framebuffer_t* fb, int x, int y) noexcept
{
    return fb->fragments[x + y * fb->width];
}

rect_t framebuffer_get_bounds(framebuffer_t* framebuffer)
{
    int found_pixel = 0;
    rect_t bounds{};
    for (std::uint32_t y = 0; y < framebuffer->height; ++y)
        for (std::uint32_t x = 0; x < framebuffer->width; ++x)
        {
            if (framebuffer_at(framebuffer, x, y).region != FRAGMENT_UNUSED)
            {
                if (found_pixel) bounds = rect_enclose_point(bounds, x, y);
                else
                {
                    bounds = make_rect(x, x + 1, y, y + 1);
                    found_pixel = 1;
                }
            }
        }
    if (!found_pixel) return make_rect(0, 0, 0, 0);
    return bounds;
}

void image_from_framebuffer(image_t* image, framebuffer_t* framebuffer, palette_t* palette, std::uint32_t dither)
{
    const rect_t bb = framebuffer_get_bounds(framebuffer);
    image->width  = static_cast<std::uint16_t>(1 + bb.x_upper - bb.x_lower);
    image->height = static_cast<std::uint16_t>(1 + bb.y_upper - bb.y_lower);
    image->x_offset = static_cast<std::int16_t>(bb.x_lower + std::floor(framebuffer->offset.x));
    image->y_offset = static_cast<std::int16_t>(bb.y_lower + std::floor(framebuffer->offset.y) - 1);
    image->pixels = static_cast<std::uint8_t*>(std::calloc(static_cast<std::size_t>(image->width) * image->height, sizeof(std::uint8_t)));

    for (int y = bb.y_lower; y <= bb.y_upper; ++y)
    {
        const int start = bb.x_lower;
        const int stop  = bb.x_upper + 1;
        const int step  = 1;

        for (int x = start; x != stop; x += step)
        {
            fragment_t fragment = framebuffer_at(framebuffer, x, y);
            fragment.color = vector_from_color(color_from_vector(fragment.color));
            if (fragment.region != FRAGMENT_UNUSED)
            {
                vector3_t error;
                image->pixels[(x - bb.x_lower) + (y - bb.y_lower) * image->width] =
                    palette_get_nearest(palette, fragment.region & REGION_MASK, fragment.color, &error);
                if (dither)
                {
                    const int points[4][2] = {{x + step, y}, {x - step, y + 1}, {x, y + 1}, {x + step, y + 1}};
                    const float weights[4] = {7.0f / 16.0f, 3.0f / 16.0f, 5.0f / 16.0f, 1.0f / 16.0f};
                    for (int i = 0; i < 4; ++i)
                    {
                        const int px = points[i][0];
                        const int py = points[i][1];
                        if (px >= 0 && px < framebuffer->width - 1 && py >= 0 && py < framebuffer->height - 1
                            && (!(fragment.flags & MATERIAL_NO_BLEED) || (framebuffer_at(framebuffer, px, py).flags & MATERIAL_NO_BLEED)))
                        {
                            framebuffer_at(framebuffer, px, py).color =
                                framebuffer_at(framebuffer, px, py).color + error * (0.3f * weights[i]);
                        }
                    }
                }
            }
        }
    }
    std::free(framebuffer->fragments);
}

void context_render_view_internal(context_t* context, matrix_t view, image_t* image, std::uint32_t silhouette)
{
    const matrix_t camera = matrix_mult(context->projection, view);

    const rect_t bounds = scene_get_bounds(&context->rt_scene, camera);

    framebuffer_t framebuffer;
    framebuffer.width = static_cast<std::uint16_t>(bounds.x_upper - bounds.x_lower + 1);
    framebuffer.height = static_cast<std::uint16_t>(bounds.y_upper - bounds.y_lower);
    framebuffer.offset = vector2(static_cast<float>(bounds.x_lower) - 0.5f, static_cast<float>(bounds.y_lower));
    framebuffer.fragments = static_cast<fragment_t*>(std::malloc(static_cast<std::size_t>(framebuffer.width) * framebuffer.height * sizeof(fragment_t)));

    std::vector<light_t> transformed_lights(context->num_lights);
    const matrix_t view_inverse = matrix_inverse(view);
    for (std::uint32_t i = 0; i < context->num_lights; ++i)
    {
        transformed_lights[i].type = context->lights[i].type;
        transformed_lights[i].shadow = context->lights[i].shadow;
        transformed_lights[i].direction = matrix_vector(view_inverse, context->lights[i].direction);
        transformed_lights[i].intensity = context->lights[i].intensity;
    }

    for (int i = 0; i < framebuffer.width * framebuffer.height; ++i)
    {
        framebuffer.fragments[i].color = vector3(0.0f, 0.0f, 0.0f);
        framebuffer.fragments[i].region = FRAGMENT_UNUSED;
        framebuffer.fragments[i].depth = 0;
        framebuffer.fragments[i].flags = 0;
    }

    const matrix_t camera_inverse = matrix_inverse(camera);
    for (int y = 0; y < framebuffer.height; ++y)
        for (int x = 0; x < framebuffer.width; ++x)
        {
            const vector2_t sample_point = vector2(x, y) + framebuffer.offset;
            material_t* material;

            int flags = 0;
            int region = FRAGMENT_UNUSED;
            float depth = std::numeric_limits<float>::infinity();
            float ghost_depth = std::numeric_limits<float>::infinity();
            int mask = 0;
            if (scene_sample_material(&context->rt_scene, sample_point, camera_inverse, &material, &depth, &ghost_depth, &mask))
            {
                region = mask ? FRAGMENT_UNUSED : material->region;
                flags = material->flags;
                if (material->flags & MATERIAL_IS_VISIBLE_MASK) mask = 1;
            }

            fragment_t subsamples[AA_NUM_SAMPLES_U * AA_NUM_SAMPLES_V];
            for (int i = 0; i < AA_NUM_SAMPLES_U; ++i)
                for (int j = 0; j < AA_NUM_SAMPLES_V; ++j)
                {
                    auto& ss = subsamples[i + j * AA_NUM_SAMPLES_U];
                    ss.color = vector3(0, 0, 0);
                    ss.region = FRAGMENT_UNUSED;
                    ss.flags = 0;
                    ss.depth = std::numeric_limits<float>::infinity();

                    const vector2_t sub_pt = vector2((i + 0.5f) / AA_NUM_SAMPLES_U - 0.5f, (j + 0.5f) / AA_NUM_SAMPLES_V - 0.5f);

                    if (!silhouette)
                    {
                        scene_sample_point(&context->rt_scene, sample_point + sub_pt, camera_inverse, transformed_lights.data(), context->num_lights, &ss);
                    }
                    else
                    {
                        float sub_depth = 0.0f;
                        float sub_ghost = 0.0f;
                        material_t* sub_mat;
                        int sub_mask = 0;
                        if (scene_sample_material(&context->rt_scene, sample_point + sub_pt, camera_inverse, &sub_mat, &sub_depth, &sub_ghost, &sub_mask))
                        {
                            ss.color = vector3(0.5f, 0.5f, 0.5f);
                            ss.region = sub_mask ? FRAGMENT_UNUSED : sub_mat->region;
                            ss.flags = static_cast<std::uint8_t>(sub_mat->flags);
                            ss.depth = sub_depth;
                            ss.ghost_depth = sub_ghost;
                        }
                    }
                }

            int front_background_aa_sample = -1;
            float min_depth = std::numeric_limits<float>::infinity();
            for (int i = 0; i < AA_NUM_SAMPLES_U * AA_NUM_SAMPLES_V; ++i)
            {
                if (subsamples[i].depth < min_depth && (subsamples[i].flags & (MATERIAL_BACKGROUND_AA | MATERIAL_BACKGROUND_AA_DARK)))
                {
                    front_background_aa_sample = i;
                    min_depth = subsamples[i].depth;
                }
            }

            if (front_background_aa_sample != -1 && (min_depth < ghost_depth - 4 || mask))
            {
                int inside_samples = 0;
                for (int i = 0; i < AA_NUM_SAMPLES_U * AA_NUM_SAMPLES_V; ++i)
                {
                    if (!(subsamples[i].depth > min_depth + 4 || (subsamples[i].region == FRAGMENT_UNUSED && !(subsamples[i].flags & MATERIAL_IS_MASK)) || (subsamples[i].flags & MATERIAL_IS_VISIBLE_MASK)))
                        inside_samples++;
                }
                if (inside_samples > 3)
                {
                    region = subsamples[front_background_aa_sample].region;
                    depth = min_depth;
                    flags = subsamples[front_background_aa_sample].flags;
                }
            }
            framebuffer.fragments[x + y * framebuffer.width].region = static_cast<std::uint8_t>(region);
            framebuffer.fragments[x + y * framebuffer.width].flags = static_cast<std::uint8_t>(flags);

            if (region == FRAGMENT_UNUSED) continue;

            if (flags & (MATERIAL_BACKGROUND_AA | MATERIAL_BACKGROUND_AA_DARK))
            {
                vector3_t color = vector3(0, 0, 0);
                float weight = 0;
                float total_weight = 0;
                int inside_samples = 0;
                for (int i = 0; i < AA_NUM_SAMPLES_U * AA_NUM_SAMPLES_V; ++i)
                {
                    if ((!(subsamples[i].flags & MATERIAL_NO_BLEED) || (flags & MATERIAL_NO_BLEED))
                        && !((subsamples[i].ghost_depth <= depth + 4 && subsamples[i].depth > depth + 4)))
                    {
                        if (!(subsamples[i].depth > depth + 4 || (subsamples[i].region == FRAGMENT_UNUSED && !(subsamples[i].flags & MATERIAL_IS_MASK)) || (subsamples[i].flags & MATERIAL_IS_VISIBLE_MASK)))
                        {
                            color = color + subsamples[i].color * AA_SAMPLE_WEIGHT;
                            weight += AA_SAMPLE_WEIGHT;
                            inside_samples++;
                        }
                        total_weight += AA_SAMPLE_WEIGHT;
                    }
                }
                color = color * (1 / total_weight);
                if (flags & MATERIAL_BACKGROUND_AA_DARK)
                    framebuffer.fragments[x + y * framebuffer.width].color = color * (0.5f + 0.5f * (weight / total_weight));
                else
                    framebuffer.fragments[x + y * framebuffer.width].color = color;
            }
            else
            {
                vector3_t color = vector3(0, 0, 0);
                float weight = 0.0f;
                for (int i = 0; i < AA_NUM_SAMPLES_U * AA_NUM_SAMPLES_V; ++i)
                {
                    if (subsamples[i].region != FRAGMENT_UNUSED && (!(subsamples[i].flags & MATERIAL_NO_BLEED) || (flags & MATERIAL_NO_BLEED)))
                    {
                        color = color + subsamples[i].color * AA_SAMPLE_WEIGHT;
                        weight += AA_SAMPLE_WEIGHT;
                    }
                }
                framebuffer.fragments[x + y * framebuffer.width].color = color * (1.0f / weight);
            }
        }

    image_from_framebuffer(image, &framebuffer, &context->palette, context->dither);
}

} // namespace

void context_render_view(context_t* context, matrix_t view, image_t* image)
{
    context_render_view_internal(context, view, image, 0);
}

void context_render_silhouette(context_t* context, matrix_t view, image_t* image)
{
    context_render_view_internal(context, view, image, 1);
}
