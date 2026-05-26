#include <gtest/gtest.h>

#include <cmath>
#include <numbers>

#include "VectorMath.hpp"

using namespace RCTGen;

namespace {
    constexpr float kEps = 1e-5f;

    void expectVecNear(Vector3 a, Vector3 b, float eps = kEps) {
        EXPECT_NEAR(a.x, b.x, eps);
        EXPECT_NEAR(a.y, b.y, eps);
        EXPECT_NEAR(a.z, b.z, eps);
    }
} // namespace

TEST(Vector2, ConstructorAndFields) {
    constexpr Vector2 v = vector2(1.5f, -2.0f);
    EXPECT_FLOAT_EQ(v.x, 1.5f);
    EXPECT_FLOAT_EQ(v.y, -2.0f);
}

TEST(Vector2, AddSubtract) {
    constexpr Vector2 a = vector2(1.0f, 2.0f);
    constexpr Vector2 b = vector2(3.0f, 5.0f);
    constexpr Vector2 sum = vector2_add(a, b);
    constexpr Vector2 diff = vector2_sub(b, a);
    EXPECT_FLOAT_EQ(sum.x, 4.0f);
    EXPECT_FLOAT_EQ(sum.y, 7.0f);
    EXPECT_FLOAT_EQ(diff.x, 2.0f);
    EXPECT_FLOAT_EQ(diff.y, 3.0f);
}

TEST(Vector2, ScalarMultiply) {
    constexpr Vector2 v = vector2_mult(vector2(2.0f, -3.0f), 1.5f);
    EXPECT_FLOAT_EQ(v.x, 3.0f);
    EXPECT_FLOAT_EQ(v.y, -4.5f);
}

TEST(Vector2, Dot) {
    EXPECT_FLOAT_EQ(vector2_dot(vector2(1.0f, 2.0f), vector2(3.0f, 4.0f)), 11.0f);
    EXPECT_FLOAT_EQ(vector2_dot(vector2(1.0f, 0.0f), vector2(0.0f, 1.0f)), 0.0f);
}

TEST(Vector2, Norm) {
    EXPECT_FLOAT_EQ(vector2_norm(vector2(3.0f, 4.0f)), 5.0f);
    EXPECT_FLOAT_EQ(vector2_norm(vector2(0.0f, 0.0f)), 0.0f);
}

TEST(Vector3, ConstructorAndFields) {
    constexpr Vector3 v = vector3(1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(v.x, 1.0f);
    EXPECT_FLOAT_EQ(v.y, 2.0f);
    EXPECT_FLOAT_EQ(v.z, 3.0f);
}

TEST(Vector3, FromScalar) {
    constexpr Vector3 v = vector3_from_scalar(2.5f);
    EXPECT_FLOAT_EQ(v.x, 2.5f);
    EXPECT_FLOAT_EQ(v.y, 2.5f);
    EXPECT_FLOAT_EQ(v.z, 2.5f);
}

TEST(Vector3, AddSubtract) {
    constexpr Vector3 sum = vector3_add(vector3(1, 2, 3), vector3(4, 5, 6));
    expectVecNear(sum, vector3(5, 7, 9));
    constexpr Vector3 diff = vector3_sub(vector3(4, 5, 6), vector3(1, 2, 3));
    expectVecNear(diff, vector3(3, 3, 3));
}

TEST(Vector3, ScalarMultiply) {
    constexpr Vector3 v = vector3_mult(vector3(1, -2, 3), 2.0f);
    expectVecNear(v, vector3(2, -4, 6));
}

TEST(Vector3, Dot) {
    EXPECT_FLOAT_EQ(vector3_dot(vector3(1, 2, 3), vector3(4, -5, 6)), 1 * 4 + 2 * -5 + 3 * 6);
}

TEST(Vector3, Cross) {
    constexpr Vector3 x = vector3(1, 0, 0);
    constexpr Vector3 y = vector3(0, 1, 0);
    constexpr Vector3 z = vector3(0, 0, 1);
    expectVecNear(vector3_cross(x, y), z);
    expectVecNear(vector3_cross(y, z), x);
    expectVecNear(vector3_cross(z, x), y);
    // Anti-commutative.
    expectVecNear(vector3_cross(y, x), vector3_mult(z, -1.0f));
}

TEST(Vector3, Norm) {
    EXPECT_FLOAT_EQ(vector3_norm(vector3(3, 4, 0)), 5.0f);
    EXPECT_FLOAT_EQ(vector3_norm(vector3(0, 0, 0)), 0.0f);
    EXPECT_FLOAT_EQ(vector3_norm(vector3(2, 3, 6)), 7.0f);
}

TEST(Vector3, Normalize) {
    Vector3 n = vector3_normalize(vector3(0, 5, 0));
    expectVecNear(n, vector3(0, 1, 0));
    Vector3 m = vector3_normalize(vector3(3, 4, 0));
    EXPECT_NEAR(vector3_norm(m), 1.0f, kEps);
}

TEST(Matrix3, IdentityIsIdentity) {
    constexpr Matrix3 I = matrix_identity();
    for (std::size_t r = 0; r < 3; ++r)
        for (std::size_t c = 0; c < 3; ++c)
            EXPECT_FLOAT_EQ(I(r, c), r == c ? 1.0f : 0.0f);
}

TEST(Matrix3, IdentityDeterminantIsOne) {
    EXPECT_FLOAT_EQ(matrix_determinant(matrix_identity()), 1.0f);
}

TEST(Matrix3, DeterminantOfDiagonal) {
    constexpr Matrix3 m = matrix(2, 0, 0, 0, 3, 0, 0, 0, 4);
    EXPECT_FLOAT_EQ(matrix_determinant(m), 24.0f);
}

TEST(Matrix3, IdentityTimesVectorIsVector) {
    constexpr Vector3 v = vector3(1.5f, -2.0f, 7.0f);
    expectVecNear(matrix_vector(matrix_identity(), v), v);
}

TEST(Matrix3, MultiplyByIdentity) {
    constexpr Matrix3 m = matrix(1, 2, 3, 4, 5, 6, 7, 8, 9);
    constexpr Matrix3 r = matrix_mult(m, matrix_identity());
    for (std::size_t i = 0; i < 9; ++i)
        EXPECT_FLOAT_EQ(r.entries[i], m.entries[i]);
}

TEST(Matrix3, Transpose) {
    constexpr Matrix3 m = matrix(1, 2, 3, 4, 5, 6, 7, 8, 9);
    constexpr Matrix3 t = matrix_transpose(m);
    for (std::size_t r = 0; r < 3; ++r)
        for (std::size_t c = 0; c < 3; ++c)
            EXPECT_FLOAT_EQ(t(r, c), m(c, r));
}

TEST(Matrix3, InverseTimesOriginalIsIdentity) {
    constexpr Matrix3 m = matrix(2, 0, 1, 1, 3, 0, 0, 1, 2);
    Matrix3 product = matrix_mult(m, matrix_inverse(m));
    for (std::size_t r = 0; r < 3; ++r)
        for (std::size_t c = 0; c < 3; ++c)
            EXPECT_NEAR(product(r, c), r == c ? 1.0f : 0.0f, kEps);
}

TEST(Matrix3, IndexOperator) {
    Matrix3 m = matrix(0, 1, 2, 3, 4, 5, 6, 7, 8);
    EXPECT_FLOAT_EQ(m(0, 0), 0.0f);
    EXPECT_FLOAT_EQ(m(1, 2), 5.0f);
    EXPECT_FLOAT_EQ(m(2, 1), 7.0f);
    m(2, 2) = 99.0f;
    EXPECT_FLOAT_EQ(m.entries[8], 99.0f);
}

TEST(Matrix3, IndexMacroMatchesAccessor) {
    Matrix3 m = matrix(0, 1, 2, 3, 4, 5, 6, 7, 8);
    EXPECT_FLOAT_EQ(MATRIX_INDEX(m, 1, 1), m(1, 1));
    EXPECT_FLOAT_EQ(MATRIX_INDEX(m, 2, 0), m(2, 0));
}

TEST(Rotate, XByPiOverTwoMapsYToZ) {
    Matrix3 m = rotate_x(static_cast<float>(std::numbers::pi) / 2.0f);
    expectVecNear(matrix_vector(m, vector3(0, 1, 0)), vector3(0, 0, 1));
}

TEST(Rotate, YByPiOverTwoMapsZToX) {
    Matrix3 m = rotate_y(static_cast<float>(std::numbers::pi) / 2.0f);
    expectVecNear(matrix_vector(m, vector3(0, 0, 1)), vector3(1, 0, 0));
}

TEST(Rotate, ZByPiOverTwoMapsXToY) {
    Matrix3 m = rotate_z(static_cast<float>(std::numbers::pi) / 2.0f);
    expectVecNear(matrix_vector(m, vector3(1, 0, 0)), vector3(0, 1, 0));
}

TEST(Rotate, ZeroAngleIsIdentity) {
    Matrix3 mx = rotate_x(0.0f);
    Matrix3 my = rotate_y(0.0f);
    Matrix3 mz = rotate_z(0.0f);
    for (std::size_t r = 0; r < 3; ++r)
        for (std::size_t c = 0; c < 3; ++c) {
            const float expected = r == c ? 1.0f : 0.0f;
            EXPECT_NEAR(mx(r, c), expected, kEps);
            EXPECT_NEAR(my(r, c), expected, kEps);
            EXPECT_NEAR(mz(r, c), expected, kEps);
        }
}

TEST(Transform, IdentityTransformIsIdentity) {
    constexpr Transform t = transform(matrix_identity(), vector3(0, 0, 0));
    expectVecNear(transform_vector(t, vector3(1, 2, 3)), vector3(1, 2, 3));
}

TEST(Transform, TranslationOnly) {
    constexpr Transform t = transform(matrix_identity(), vector3(10, 20, 30));
    expectVecNear(transform_vector(t, vector3(1, 2, 3)), vector3(11, 22, 33));
}

TEST(Transform, ComposeAppliesRhsFirst) {
    constexpr Transform a = transform(matrix_identity(), vector3(1, 0, 0));
    constexpr Transform b = transform(matrix_identity(), vector3(0, 2, 0));
    constexpr Transform ab = transform_compose(a, b);
    expectVecNear(transform_vector(ab, vector3(0, 0, 0)), vector3(1, 2, 0));
}