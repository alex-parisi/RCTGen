#pragma once

#include <bitset>
#include <cstdint>
#include <embree4/rtcore.h>
#include "model.h"
#include "vectormath.h"

inline constexpr std::size_t MAX_MESHES = 128;

using device_t = RTCDevice;

struct vertex_t
{
    vector3_t vertex;
    vector3_t normal;
};

// Mesh flag bits (combined with bitwise OR).
inline constexpr int MESH_MASK  = 1 << 0;
inline constexpr int MESH_GHOST = 1 << 1;

struct context_s;

struct scene_t
{
    context_s* context;
    mesh_t* meshes[MAX_MESHES];
    std::bitset<MAX_MESHES> mask;
    std::bitset<MAX_MESHES> ghost;
    std::uint32_t num_meshes;
    float x_min, x_max, y_min, y_max, z_min, z_max;
    RTCDevice embree_device;
    RTCScene embree_scene;
};

struct ray_hit_t
{
    std::uint32_t mesh_index;
    std::uint32_t face_index;
    vector3_t position;
    vector3_t normal;
    float ghost_distance;
    float distance;
    float u;
    float v;
};

device_t device_init();
void device_destroy(device_t device);

void scene_init(scene_t* scene, device_t device);
void scene_finalize(scene_t* scene);
void scene_destroy(scene_t* scene);
void scene_add_model(scene_t* scene, mesh_t* mesh, vertex_t (*transform)(vector3_t, vector3_t, bool, void*), void* data, int flags);
int scene_trace_ray(scene_t* scene, vector3_t origin, vector3_t direction, ray_hit_t* hit);
int scene_trace_occlusion_ray(scene_t* scene, vector3_t origin, vector3_t direction);
int scene_is_mask(scene_t* scene, int index);
int scene_is_ghost(scene_t* scene, int index);
