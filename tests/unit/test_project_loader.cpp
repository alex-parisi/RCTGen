#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>

#include <jansson.h>

#include "Json.hpp"
#include "Model.hpp"
#include "ProjectLoader.hpp"

using namespace RCTGen;

namespace {

JsonRef parse(const char* src) {
    json_error_t err;
    return adoptJson(json_loads(src, 0, &err));
}

constexpr const char* kSimpleModel = R"({
    "mesh_index": 0,
    "position": [1.0, 2.0, 3.0],
    "orientation": [0.0, 90.0, 0.0]
})";

} // namespace

// ---------- loadLights ----------

TEST(LoadLights, NullErrors) {
    auto r = loadLights(nullptr);
    EXPECT_FALSE(r.has_value());
}

TEST(LoadLights, NonArrayErrors) {
    JsonRef j = parse(R"({"type": "diffuse"})");
    auto r = loadLights(j.get());
    EXPECT_FALSE(r.has_value());
}

TEST(LoadLights, EmptyArrayProducesEmpty) {
    JsonRef j = parse("[]");
    auto r = loadLights(j.get());
    ASSERT_TRUE(r.has_value());
    EXPECT_TRUE(r->empty());
}

TEST(LoadLights, SingleDiffuseLight) {
    JsonRef j = parse(R"([
        {"type": "diffuse", "shadow": false,
         "direction": [1.0, 0.0, 0.0], "strength": 0.5}
    ])");
    auto r = loadLights(j.get());
    ASSERT_TRUE(r.has_value()) << r.error();
    ASSERT_EQ(r->size(), 1u);
    EXPECT_EQ((*r)[0].type, LIGHT_DIFFUSE);
    EXPECT_EQ((*r)[0].shadow, 0u);
    EXPECT_FLOAT_EQ((*r)[0].intensity, 0.5f);
    // direction is normalized: (1,0,0) stays (1,0,0)
    EXPECT_FLOAT_EQ((*r)[0].direction.x, 1.0f);
    EXPECT_FLOAT_EQ((*r)[0].direction.y, 0.0f);
    EXPECT_FLOAT_EQ((*r)[0].direction.z, 0.0f);
}

TEST(LoadLights, SpecularWithShadow) {
    JsonRef j = parse(R"([
        {"type": "specular", "shadow": true,
         "direction": [0.0, 1.0, 0.0], "strength": 0.8}
    ])");
    auto r = loadLights(j.get());
    ASSERT_TRUE(r.has_value()) << r.error();
    ASSERT_EQ(r->size(), 1u);
    EXPECT_EQ((*r)[0].type, LIGHT_SPECULAR);
    EXPECT_EQ((*r)[0].shadow, 1u);
}

TEST(LoadLights, DirectionGetsNormalized) {
    JsonRef j = parse(R"([
        {"type": "diffuse", "shadow": false,
         "direction": [3.0, 4.0, 0.0], "strength": 1.0}
    ])");
    auto r = loadLights(j.get());
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->size(), 1u);
    const auto& d = (*r)[0].direction;
    EXPECT_NEAR(std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z), 1.0, 1e-5);
    EXPECT_NEAR(d.x, 0.6f, 1e-5);
    EXPECT_NEAR(d.y, 0.8f, 1e-5);
}

TEST(LoadLights, MultipleLightsPreserveOrder) {
    JsonRef j = parse(R"([
        {"type": "diffuse",  "shadow": false, "direction": [1, 0, 0], "strength": 0.1},
        {"type": "specular", "shadow": true,  "direction": [0, 1, 0], "strength": 0.2},
        {"type": "diffuse",  "shadow": false, "direction": [0, 0, 1], "strength": 0.3}
    ])");
    auto r = loadLights(j.get());
    ASSERT_TRUE(r.has_value()) << r.error();
    ASSERT_EQ(r->size(), 3u);
    EXPECT_FLOAT_EQ((*r)[0].intensity, 0.1f);
    EXPECT_FLOAT_EQ((*r)[1].intensity, 0.2f);
    EXPECT_FLOAT_EQ((*r)[2].intensity, 0.3f);
    EXPECT_EQ((*r)[1].type, LIGHT_SPECULAR);
}

TEST(LoadLights, UnknownTypeErrors) {
    JsonRef j = parse(R"([
        {"type": "ambient", "shadow": false, "direction": [1, 0, 0], "strength": 0.5}
    ])");
    auto r = loadLights(j.get());
    EXPECT_FALSE(r.has_value());
    EXPECT_NE(r.error().find("ambient"), std::string::npos);
}

TEST(LoadLights, MissingTypeErrors) {
    JsonRef j = parse(R"([
        {"shadow": false, "direction": [1, 0, 0], "strength": 0.5}
    ])");
    EXPECT_FALSE(loadLights(j.get()).has_value());
}

TEST(LoadLights, MissingShadowErrors) {
    JsonRef j = parse(R"([
        {"type": "diffuse", "direction": [1, 0, 0], "strength": 0.5}
    ])");
    EXPECT_FALSE(loadLights(j.get()).has_value());
}

TEST(LoadLights, NonBooleanShadowErrors) {
    JsonRef j = parse(R"([
        {"type": "diffuse", "shadow": 1, "direction": [1, 0, 0], "strength": 0.5}
    ])");
    EXPECT_FALSE(loadLights(j.get()).has_value());
}

TEST(LoadLights, BadDirectionErrors) {
    JsonRef j = parse(R"([
        {"type": "diffuse", "shadow": false, "direction": [1, 0], "strength": 0.5}
    ])");
    EXPECT_FALSE(loadLights(j.get()).has_value());
}

TEST(LoadLights, MissingStrengthErrors) {
    JsonRef j = parse(R"([
        {"type": "diffuse", "shadow": false, "direction": [1, 0, 0]}
    ])");
    EXPECT_FALSE(loadLights(j.get()).has_value());
}

TEST(LoadLights, NonObjectElementsSkipped) {
    JsonRef j = parse(R"([
        "not-an-object",
        {"type": "diffuse", "shadow": false, "direction": [1, 0, 0], "strength": 0.5}
    ])");
    auto r = loadLights(j.get());
    ASSERT_TRUE(r.has_value()) << r.error();
    EXPECT_EQ(r->size(), 1u);
}

// ---------- loadModel ----------

TEST(LoadModel, NullJsonErrors) {
    Model m;
    EXPECT_FALSE(loadModel(m, nullptr, 1, 1).has_value());
}

TEST(LoadModel, ScalarMeshAndSingleFrame) {
    JsonRef j = parse(kSimpleModel);
    Model m;
    auto r = loadModel(m, j.get(), 1, 1);
    ASSERT_TRUE(r.has_value()) << r.error();
    ASSERT_EQ(m.meshes.size(), 1u);
    EXPECT_EQ(m.meshes[0][0].mesh_index, 0);
    EXPECT_FLOAT_EQ(m.meshes[0][0].position.x, 1.0f);
    EXPECT_FLOAT_EQ(m.meshes[0][0].position.y, 2.0f);
    EXPECT_FLOAT_EQ(m.meshes[0][0].position.z, 3.0f);
    EXPECT_FLOAT_EQ(m.meshes[0][0].orientation.y, 90.0f);
}

TEST(LoadModel, ScalarObjectWrappedAutomatically) {
    // Single object (not in an array) — should be wrapped into a 1-element list.
    JsonRef j = parse(kSimpleModel);
    Model m;
    auto r = loadModel(m, j.get(), 1, 1);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(m.meshes.size(), 1u);
}

TEST(LoadModel, MultipleMeshObjects) {
    JsonRef j = parse(R"([
        {"mesh_index": 0, "position": [0, 0, 0], "orientation": [0, 0, 0]},
        {"mesh_index": 1, "position": [1, 0, 0], "orientation": [0, 0, 0]}
    ])");
    Model m;
    auto r = loadModel(m, j.get(), 2, 1);
    ASSERT_TRUE(r.has_value()) << r.error();
    ASSERT_EQ(m.meshes.size(), 2u);
    EXPECT_EQ(m.meshes[0][0].mesh_index, 0);
    EXPECT_EQ(m.meshes[1][0].mesh_index, 1);
    EXPECT_FLOAT_EQ(m.meshes[1][0].position.x, 1.0f);
}

TEST(LoadModel, MinusOneMeshIndexAllowed) {
    JsonRef j = parse(R"([
        {"mesh_index": -1, "position": [0, 0, 0], "orientation": [0, 0, 0]}
    ])");
    Model m;
    auto r = loadModel(m, j.get(), 1, 1);
    ASSERT_TRUE(r.has_value()) << r.error();
    EXPECT_EQ(m.meshes[0][0].mesh_index, -1);
}

TEST(LoadModel, MeshIndexBelowMinusOneErrors) {
    JsonRef j = parse(R"([
        {"mesh_index": -2, "position": [0, 0, 0], "orientation": [0, 0, 0]}
    ])");
    Model m;
    EXPECT_FALSE(loadModel(m, j.get(), 1, 1).has_value());
}

TEST(LoadModel, MeshIndexOutOfRangeErrors) {
    JsonRef j = parse(R"([
        {"mesh_index": 5, "position": [0, 0, 0], "orientation": [0, 0, 0]}
    ])");
    Model m;
    EXPECT_FALSE(loadModel(m, j.get(), 1, 1).has_value());
}

TEST(LoadModel, MissingMeshIndexErrors) {
    JsonRef j = parse(R"([
        {"position": [0, 0, 0], "orientation": [0, 0, 0]}
    ])");
    Model m;
    EXPECT_FALSE(loadModel(m, j.get(), 1, 1).has_value());
}

TEST(LoadModel, MissingPositionErrors) {
    JsonRef j = parse(R"([
        {"mesh_index": 0, "orientation": [0, 0, 0]}
    ])");
    Model m;
    EXPECT_FALSE(loadModel(m, j.get(), 1, 1).has_value());
}

TEST(LoadModel, MissingOrientationErrors) {
    JsonRef j = parse(R"([
        {"mesh_index": 0, "position": [0, 0, 0]}
    ])");
    Model m;
    EXPECT_FALSE(loadModel(m, j.get(), 1, 1).has_value());
}

TEST(LoadModel, ScalarMeshIndexBroadcastsAcrossFrames) {
    // mesh_index is scalar (broadcasts), position/orientation are scalar (also broadcast).
    JsonRef j = parse(R"([
        {"mesh_index": 0, "position": [1, 0, 0], "orientation": [0, 0, 0]}
    ])");
    Model m;
    auto r = loadModel(m, j.get(), 1, 4);
    ASSERT_TRUE(r.has_value()) << r.error();
    ASSERT_EQ(m.meshes.size(), 1u);
    for (int f = 0; f < 4; f++) {
        EXPECT_EQ(m.meshes[0][f].mesh_index, 0) << "frame " << f;
        EXPECT_FLOAT_EQ(m.meshes[0][f].position.x, 1.0f) << "frame " << f;
    }
}

TEST(LoadModel, PerFrameMeshIndexAndVectors) {
    JsonRef j = parse(R"([
        {
            "mesh_index": [0, 1, 0, -1],
            "position": [[0, 0, 0], [1, 0, 0], [2, 0, 0], [3, 0, 0]],
            "orientation": [[0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0]]
        }
    ])");
    Model m;
    auto r = loadModel(m, j.get(), 2, 4);
    ASSERT_TRUE(r.has_value()) << r.error();
    EXPECT_EQ(m.meshes[0][0].mesh_index, 0);
    EXPECT_EQ(m.meshes[0][1].mesh_index, 1);
    EXPECT_EQ(m.meshes[0][2].mesh_index, 0);
    EXPECT_EQ(m.meshes[0][3].mesh_index, -1);
    EXPECT_FLOAT_EQ(m.meshes[0][2].position.x, 2.0f);
}

TEST(LoadModel, MeshIndexFrameCountMismatchErrors) {
    JsonRef j = parse(R"([
        {
            "mesh_index": [0, 1, 0],
            "position": [0, 0, 0],
            "orientation": [0, 0, 0]
        }
    ])");
    Model m;
    // 3 indices but 4 frames expected (and 3 != 1).
    EXPECT_FALSE(loadModel(m, j.get(), 1, 4).has_value());
}

TEST(LoadModel, PositionFrameCountMismatchErrors) {
    JsonRef j = parse(R"([
        {
            "mesh_index": 0,
            "position": [[0, 0, 0], [1, 0, 0]],
            "orientation": [0, 0, 0]
        }
    ])");
    Model m;
    // 2-element position array, but neither 3 (scalar vector) nor numFrames (4).
    EXPECT_FALSE(loadModel(m, j.get(), 1, 4).has_value());
}
