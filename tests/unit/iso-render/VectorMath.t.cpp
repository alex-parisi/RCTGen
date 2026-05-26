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
    constexpr Vector2 kV = vector2(1.5f, -2.0f);
    EXPECT_FLOAT_EQ(kV.x, 1.5f);
    EXPECT_FLOAT_EQ(kV.y, -2.0f);
}

TEST(Vector2, AddSubtract) {
    constexpr Vector2 kA = vector2(1.0f, 2.0f);
    constexpr Vector2 kB = vector2(3.0f, 5.0f);
    constexpr Vector2 kSum = vector2_add(kA, kB);
    constexpr Vector2 kDiff = vector2_sub(kB, kA);
    EXPECT_FLOAT_EQ(kSum.x, 4.0f);
    EXPECT_FLOAT_EQ(kSum.y, 7.0f);
    EXPECT_FLOAT_EQ(kDiff.x, 2.0f);
    EXPECT_FLOAT_EQ(kDiff.y, 3.0f);
}

TEST(Vector2, ScalarMultiply) {
    constexpr Vector2 kV = vector2_mult(vector2(2.0f, -3.0f), 1.5f);
    EXPECT_FLOAT_EQ(kV.x, 3.0f);
    EXPECT_FLOAT_EQ(kV.y, -4.5f);
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
    constexpr Vector3 kV = vector3(1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(kV.x, 1.0f);
    EXPECT_FLOAT_EQ(kV.y, 2.0f);
    EXPECT_FLOAT_EQ(kV.z, 3.0f);
}

TEST(Vector3, FromScalar) {
    constexpr Vector3 kV = vector3_from_scalar(2.5f);
    EXPECT_FLOAT_EQ(kV.x, 2.5f);
    EXPECT_FLOAT_EQ(kV.y, 2.5f);
    EXPECT_FLOAT_EQ(kV.z, 2.5f);
}

TEST(Vector3, AddSubtract) {
    constexpr Vector3 kSum = vector3_add(vector3(1, 2, 3), vector3(4, 5, 6));
    expectVecNear(kSum, vector3(5, 7, 9));
    constexpr Vector3 kDiff = vector3_sub(vector3(4, 5, 6), vector3(1, 2, 3));
    expectVecNear(kDiff, vector3(3, 3, 3));
}

TEST(Vector3, ScalarMultiply) {
    constexpr Vector3 kV = vector3_mult(vector3(1, -2, 3), 2.0f);
    expectVecNear(kV, vector3(2, -4, 6));
}

TEST(Vector3, Dot) {
    EXPECT_FLOAT_EQ(vector3_dot(vector3(1, 2, 3), vector3(4, -5, 6)), 1 * 4 + 2 * -5 + 3 * 6);
}

TEST(Vector3, Cross) {
    constexpr Vector3 kX = vector3(1, 0, 0);
    constexpr Vector3 kY = vector3(0, 1, 0);
    constexpr Vector3 kZ = vector3(0, 0, 1);
    expectVecNear(vector3_cross(kX, kY), kZ);
    expectVecNear(vector3_cross(kY, kZ), kX);
    expectVecNear(vector3_cross(kZ, kX), kY);
    // Anti-commutative.
    expectVecNear(vector3_cross(kY, kX), vector3_mult(kZ, -1.0f));
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
    constexpr Matrix3 kIdentity = matrix_identity();
    for (std::size_t r = 0; r < 3; ++r)
        for (std::size_t c = 0; c < 3; ++c)
            EXPECT_FLOAT_EQ(kIdentity(r, c), r == c ? 1.0f : 0.0f);
}

TEST(Matrix3, IdentityDeterminantIsOne) {
    EXPECT_FLOAT_EQ(matrix_determinant(matrix_identity()), 1.0f);
}

TEST(Matrix3, DeterminantOfDiagonal) {
    constexpr Matrix3 kM = matrix(2, 0, 0, 0, 3, 0, 0, 0, 4);
    EXPECT_FLOAT_EQ(matrix_determinant(kM), 24.0f);
}

TEST(Matrix3, IdentityTimesVectorIsVector) {
    constexpr Vector3 kV = vector3(1.5f, -2.0f, 7.0f);
    expectVecNear(matrix_vector(matrix_identity(), kV), kV);
}

TEST(Matrix3, MultiplyByIdentity) {
    constexpr Matrix3 kM = matrix(1, 2, 3, 4, 5, 6, 7, 8, 9);
    constexpr Matrix3 kProduct = matrix_mult(kM, matrix_identity());
    for (std::size_t i = 0; i < 9; ++i)
        EXPECT_FLOAT_EQ(kProduct.entries[i], kM.entries[i]);
}

TEST(Matrix3, Transpose) {
    constexpr Matrix3 kM = matrix(1, 2, 3, 4, 5, 6, 7, 8, 9);
    constexpr Matrix3 kT = matrix_transpose(kM);
    for (std::size_t r = 0; r < 3; ++r)
        for (std::size_t c = 0; c < 3; ++c)
            EXPECT_FLOAT_EQ(kT(r, c), kM(c, r));
}

TEST(Matrix3, InverseTimesOriginalIsIdentity) {
    constexpr Matrix3 kM = matrix(2, 0, 1, 1, 3, 0, 0, 1, 2);
    Matrix3 product = matrix_mult(kM, matrix_inverse(kM));
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
    constexpr Transform kT = transform(matrix_identity(), vector3(0, 0, 0));
    expectVecNear(transform_vector(kT, vector3(1, 2, 3)), vector3(1, 2, 3));
}

TEST(Transform, TranslationOnly) {
    constexpr Transform kT = transform(matrix_identity(), vector3(10, 20, 30));
    expectVecNear(transform_vector(kT, vector3(1, 2, 3)), vector3(11, 22, 33));
}

TEST(Transform, ComposeAppliesRhsFirst) {
    constexpr Transform kA = transform(matrix_identity(), vector3(1, 0, 0));
    constexpr Transform kB = transform(matrix_identity(), vector3(0, 2, 0));
    constexpr Transform kAB = transform_compose(kA, kB);
    expectVecNear(transform_vector(kAB, vector3(0, 0, 0)), vector3(1, 2, 0));
}