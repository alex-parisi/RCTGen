#include "raytrace.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <embree4/rtcore.h>
#include <limits>
#include <numbers>

namespace
{

void rt_error(void* /*user_ptr*/, RTCError error, const char* str)
{
    std::printf("error %d: %s\n", error, str);
    std::exit(1);
}

void occlusion_filter(const RTCFilterFunctionNArguments* args)
{
    const unsigned int N = args->N;
    assert(N == 1);

    auto* ray = reinterpret_cast<RTCRay*>(args->ray);
    auto* hit = reinterpret_cast<RTCHit*>(args->hit);

    if (hit->Ng_x * ray->dir_x + hit->Ng_y * ray->dir_y + hit->Ng_z * ray->dir_z > 0)
        args->valid[0] = 0;
}

} // namespace

device_t device_init()
{
    device_t device = rtcNewDevice(nullptr);
    if (!device)
    {
        std::printf("error %d: cannot create device\n", rtcGetDeviceError(nullptr));
        std::exit(1);
    }
    rtcSetDeviceErrorFunction(device, rt_error, nullptr);
    return device;
}

void device_destroy(device_t device)
{
    rtcReleaseDevice(device);
}

int scene_is_mask(scene_t* scene, int index)
{
    return scene->mask.test(static_cast<std::size_t>(index));
}

int scene_is_ghost(scene_t* scene, int index)
{
    return scene->ghost.test(static_cast<std::size_t>(index));
}

void scene_init(scene_t* scene, device_t device)
{
    scene->num_meshes = 0;
    scene->mask.reset();
    scene->ghost.reset();
    scene->embree_device = device;
    scene->embree_scene = rtcNewScene(device);
    constexpr float inf = std::numeric_limits<float>::infinity();
    scene->x_max = -inf;
    scene->y_max = -inf;
    scene->z_max = -inf;
    scene->x_min = inf;
    scene->y_min = inf;
    scene->z_min = inf;
}

void scene_finalize(scene_t* scene)
{
    rtcCommitScene(scene->embree_scene);
}

void scene_destroy(scene_t* scene)
{
    rtcReleaseScene(scene->embree_scene);
}

void scene_add_model(scene_t* scene, mesh_t* mesh, vertex_t (*transform)(vector3_t, vector3_t, bool, void*), void* data, int flags)
{
    assert(scene->num_meshes < MAX_MESHES);
    scene->meshes[scene->num_meshes] = mesh;
    if (flags & MESH_MASK)  scene->mask.set(scene->num_meshes);
    if (flags & MESH_GHOST) scene->ghost.set(scene->num_meshes);
    scene->num_meshes++;

    RTCGeometry geom = rtcNewGeometry(scene->embree_device, RTC_GEOMETRY_TYPE_TRIANGLE);
    if (geom == nullptr)
    {
        std::printf("Failed allocating geometry\n");
        return;
    }

    rtcSetGeometryVertexAttributeCount(geom, 1);
    auto* vertices = static_cast<float*>(rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, 3 * sizeof(float), mesh->num_vertices));
    auto* normals  = static_cast<float*>(rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, RTC_FORMAT_FLOAT3, 3 * sizeof(float), mesh->num_vertices));
    auto* indices  = static_cast<unsigned int*>(rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3 * sizeof(unsigned int), mesh->num_faces));
    if (!(vertices && indices && normals))
    {
        std::printf("Failed allocating geometry buffer\n");
        rtcReleaseGeometry(geom);
        return;
    }

    for (std::uint32_t i = 0; i < mesh->num_vertices; ++i)
    {
        bool flat_shaded = false;
        for (std::uint32_t face_index = 0; face_index < mesh->num_faces; ++face_index)
        {
            if (mesh->materials[mesh->faces[face_index].material].flags & MATERIAL_IS_FLAT_SHADED)
            {
                flat_shaded = true;
                break;
            }
        }

        const vertex_t transformed = transform(mesh->vertices[i], mesh->normals[i], flat_shaded, data);
        vertices[3 * i + 0] = transformed.vertex.x;
        vertices[3 * i + 1] = transformed.vertex.y;
        vertices[3 * i + 2] = transformed.vertex.z;
        normals[3 * i + 0] = transformed.normal.x;
        normals[3 * i + 1] = transformed.normal.y;
        normals[3 * i + 2] = transformed.normal.z;
        scene->x_max = std::max(scene->x_max, transformed.vertex.x);
        scene->y_max = std::max(scene->y_max, transformed.vertex.y);
        scene->z_max = std::max(scene->z_max, transformed.vertex.z);
        scene->x_min = std::min(scene->x_min, transformed.vertex.x);
        scene->y_min = std::min(scene->y_min, transformed.vertex.y);
        scene->z_min = std::min(scene->z_min, transformed.vertex.z);
    }

    for (std::uint32_t i = 0; i < mesh->num_faces; ++i)
    {
        indices[3 * i + 0] = static_cast<unsigned int>(mesh->faces[i].indices[0]);
        indices[3 * i + 1] = static_cast<unsigned int>(mesh->faces[i].indices[1]);
        indices[3 * i + 2] = static_cast<unsigned int>(mesh->faces[i].indices[2]);
    }
    rtcSetGeometryOccludedFilterFunction(geom, occlusion_filter);
    rtcCommitGeometry(geom);
    rtcAttachGeometry(scene->embree_scene, geom);
    rtcReleaseGeometry(geom);
}

int scene_trace_ray(scene_t* scene, vector3_t origin, vector3_t direction, ray_hit_t* hit)
{
    RTCRayHit rayhit{};

    rayhit.ray.org_x = origin.x;
    rayhit.ray.org_y = origin.y;
    rayhit.ray.org_z = origin.z;
    rayhit.ray.dir_x = direction.x;
    rayhit.ray.dir_y = direction.y;
    rayhit.ray.dir_z = direction.z;
    rayhit.ray.tnear = 0;
    rayhit.ray.tfar = std::numeric_limits<float>::infinity();
    rayhit.ray.mask = static_cast<unsigned int>(-1);
    rayhit.ray.flags = 0;
    rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

    rtcIntersect1(scene->embree_scene, &rayhit);

    hit->ghost_distance = rayhit.ray.tfar;

    while ((rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) && scene_is_ghost(scene, rayhit.hit.geomID))
    {
        rayhit.ray.tnear = rayhit.ray.tfar + 0.0001f;
        rayhit.ray.tfar = std::numeric_limits<float>::infinity();
        rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
        rtcIntersect1(scene->embree_scene, &rayhit);
    }

    if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
    {
        hit->mesh_index = rayhit.hit.geomID;
        hit->face_index = rayhit.hit.primID;
        hit->u = rayhit.hit.u;
        hit->v = rayhit.hit.v;

        float position_components[3];
        float normal_components[3];
        rtcInterpolate0(rtcGetGeometry(scene->embree_scene, rayhit.hit.geomID), rayhit.hit.primID, rayhit.hit.u, rayhit.hit.v, RTC_BUFFER_TYPE_VERTEX, 0, position_components, 3);
        rtcInterpolate0(rtcGetGeometry(scene->embree_scene, rayhit.hit.geomID), rayhit.hit.primID, rayhit.hit.u, rayhit.hit.v, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, normal_components, 3);
        hit->position = vector3(position_components[0], position_components[1], position_components[2]);
        hit->normal = vector3(normal_components[0], normal_components[1], normal_components[2]).normalized();
        hit->distance = rayhit.ray.tfar;
        return 1;
    }
    return 0;
}

int scene_trace_occlusion_ray(scene_t* scene, vector3_t origin, vector3_t direction)
{
    RTCRay ray{};
    ray.org_x = origin.x;
    ray.org_y = origin.y;
    ray.org_z = origin.z;
    ray.dir_x = direction.x;
    ray.dir_y = direction.y;
    ray.dir_z = direction.z;
    ray.tnear = 1e-5f;
    ray.tfar = std::numeric_limits<float>::infinity();
    ray.mask = static_cast<unsigned int>(-1);
    ray.flags = 0;

    rtcOccluded1(scene->embree_scene, &ray);

    return ray.tfar <= 0.0f;
}
