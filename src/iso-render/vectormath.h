#pragma once

#include <array>
#include <cmath>
#include <cstddef>

struct vector2_t
{
    float x;
    float y;

    constexpr vector2_t operator+(vector2_t rhs) const noexcept { return {x + rhs.x, y + rhs.y}; }
    constexpr vector2_t operator-(vector2_t rhs) const noexcept { return {x - rhs.x, y - rhs.y}; }
    constexpr vector2_t operator*(float s) const noexcept { return {x * s, y * s}; }
    constexpr float dot(vector2_t rhs) const noexcept { return x * rhs.x + y * rhs.y; }
    [[nodiscard]] float norm() const noexcept { return std::sqrt(x * x + y * y); }
};

struct vector3_t
{
    float x;
    float y;
    float z;

    static constexpr vector3_t splat(float v) noexcept { return {v, v, v}; }

    constexpr vector3_t operator+(vector3_t rhs) const noexcept { return {x + rhs.x, y + rhs.y, z + rhs.z}; }
    constexpr vector3_t operator-(vector3_t rhs) const noexcept { return {x - rhs.x, y - rhs.y, z - rhs.z}; }
    constexpr vector3_t operator*(float s) const noexcept { return {x * s, y * s, z * s}; }
    constexpr float dot(vector3_t rhs) const noexcept { return x * rhs.x + y * rhs.y + z * rhs.z; }
    constexpr vector3_t cross(vector3_t rhs) const noexcept
    {
        return {y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x};
    }
    [[nodiscard]] float norm() const noexcept { return std::sqrt(dot(*this)); }
    [[nodiscard]] vector3_t normalized() const noexcept
    {
        const float n = norm();
        return *this * (1.0f / n);
    }
};

struct matrix_t
{
    std::array<float, 9> entries;

    constexpr float& operator()(std::size_t row, std::size_t col) noexcept { return entries[3 * row + col]; }
    constexpr float operator()(std::size_t row, std::size_t col) const noexcept { return entries[3 * row + col]; }

    [[nodiscard]] constexpr float determinant() const noexcept
    {
        return (*this)(0, 0) * ((*this)(1, 1) * (*this)(2, 2) - (*this)(1, 2) * (*this)(2, 1))
             - (*this)(0, 1) * ((*this)(1, 0) * (*this)(2, 2) - (*this)(1, 2) * (*this)(2, 0))
             + (*this)(0, 2) * ((*this)(1, 0) * (*this)(2, 1) - (*this)(1, 1) * (*this)(2, 0));
    }
};

struct transform_t
{
    matrix_t matrix;
    vector3_t translation;
};

// Backward-compatibility helper macro; new code should prefer m(row,col).
#define MATRIX_INDEX(m, row, col) ((m).entries[3 * (row) + (col)])

// Constructor-style helpers (constexpr where possible).
constexpr vector2_t vector2(float x, float y) noexcept { return {x, y}; }
constexpr vector3_t vector3(float x, float y, float z) noexcept { return {x, y, z}; }
constexpr vector3_t vector3_from_scalar(float a) noexcept { return vector3_t::splat(a); }

constexpr vector2_t vector2_add(vector2_t a, vector2_t b) noexcept { return a + b; }
constexpr vector2_t vector2_sub(vector2_t a, vector2_t b) noexcept { return a - b; }
constexpr vector2_t vector2_mult(vector2_t a, float b) noexcept { return a * b; }
constexpr float vector2_dot(vector2_t a, vector2_t b) noexcept { return a.dot(b); }
inline float vector2_norm(vector2_t a) noexcept { return a.norm(); }

constexpr vector3_t vector3_add(vector3_t a, vector3_t b) noexcept { return a + b; }
constexpr vector3_t vector3_sub(vector3_t a, vector3_t b) noexcept { return a - b; }
constexpr vector3_t vector3_mult(vector3_t a, float b) noexcept { return a * b; }
constexpr float vector3_dot(vector3_t a, vector3_t b) noexcept { return a.dot(b); }
constexpr vector3_t vector3_cross(vector3_t a, vector3_t b) noexcept { return a.cross(b); }
inline float vector3_norm(vector3_t a) noexcept { return a.norm(); }
inline vector3_t vector3_normalize(vector3_t a) noexcept { return a.normalized(); }

constexpr matrix_t matrix(float a, float b, float c, float d, float e, float f, float g, float h, float i) noexcept
{
    return matrix_t{{a, b, c, d, e, f, g, h, i}};
}
constexpr matrix_t matrix_identity() noexcept { return matrix(1, 0, 0, 0, 1, 0, 0, 0, 1); }
constexpr float matrix_determinant(matrix_t m) noexcept { return m.determinant(); }

constexpr matrix_t matrix_inverse(matrix_t m) noexcept
{
    const float d = m.determinant();
    return matrix(
        (m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1)) / d,
        (m(0, 2) * m(2, 1) - m(2, 2) * m(0, 1)) / d,
        (m(0, 1) * m(1, 2) - m(1, 1) * m(0, 2)) / d,
        (m(1, 2) * m(2, 0) - m(2, 2) * m(1, 0)) / d,
        (m(0, 0) * m(2, 2) - m(2, 0) * m(0, 2)) / d,
        (m(0, 2) * m(1, 0) - m(1, 2) * m(0, 0)) / d,
        (m(1, 0) * m(2, 1) - m(2, 0) * m(1, 1)) / d,
        (m(0, 1) * m(2, 0) - m(2, 1) * m(0, 0)) / d,
        (m(0, 0) * m(1, 1) - m(1, 0) * m(0, 1)) / d);
}

constexpr matrix_t matrix_transpose(matrix_t m) noexcept
{
    return matrix(m(0, 0), m(1, 0), m(2, 0),
                  m(0, 1), m(1, 1), m(2, 1),
                  m(0, 2), m(1, 2), m(2, 2));
}

constexpr matrix_t matrix_mult(matrix_t a, matrix_t b) noexcept
{
    matrix_t result{{}};
    for (std::size_t col = 0; col < 3; ++col)
        for (std::size_t row = 0; row < 3; ++row)
            result(row, col) = a(row, 0) * b(0, col) + a(row, 1) * b(1, col) + a(row, 2) * b(2, col);
    return result;
}

constexpr vector3_t matrix_vector(matrix_t m, vector3_t v) noexcept
{
    return vector3(m(0, 0) * v.x + m(0, 1) * v.y + m(0, 2) * v.z,
                   m(1, 0) * v.x + m(1, 1) * v.y + m(1, 2) * v.z,
                   m(2, 0) * v.x + m(2, 1) * v.y + m(2, 2) * v.z);
}

constexpr transform_t transform(matrix_t m, vector3_t v) noexcept { return {m, v}; }

constexpr transform_t transform_compose(transform_t a, transform_t b) noexcept
{
    return {matrix_mult(a.matrix, b.matrix), matrix_vector(a.matrix, b.translation) + a.translation};
}

constexpr vector3_t transform_vector(transform_t t, vector3_t v) noexcept
{
    return matrix_vector(t.matrix, v) + t.translation;
}

inline matrix_t rotate_x(float theta) noexcept
{
    const float c = std::cos(theta);
    const float s = std::sin(theta);
    return matrix(1, 0, 0, 0, c, -s, 0, s, c);
}
inline matrix_t rotate_y(float theta) noexcept
{
    const float c = std::cos(theta);
    const float s = std::sin(theta);
    return matrix(c, 0, s, 0, 1, 0, -s, 0, c);
}
inline matrix_t rotate_z(float theta) noexcept
{
    const float c = std::cos(theta);
    const float s = std::sin(theta);
    return matrix(c, -s, 0, s, c, 0, 0, 0, 1);
}
