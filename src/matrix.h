#pragma once

#include "vector.h"
#include <cmath>

template<typename T>
struct Matrix4 {
    using dataType = T;
    Vector4<T> col[4];

    Matrix4() = default;

    Matrix4(
        T m00, T m01, T m02, T m03, 
        T m10, T m11, T m12, T m13, 
        T m20, T m21, T m22, T m23, 
        T m30, T m31, T m32, T m33 )
        : col{{m00, m10, m20, m30},
              {m01, m11, m21, m31},
              {m02, m12, m22, m32},
              {m03, m13, m23, m33}} {}
    
    Vector4<T>& operator[] (size_t c) {
        return col[c];
    }

    Vector4<T> const operator[] (size_t c) const {
        return col[c];
    }
    
    Vector4<T> row(size_t r) const {
        return Vector4<T>{
            col[0][r],
            col[1][r],
            col[2][r],
            col[3][r],
        };
    }

    Matrix4 operator*(Matrix4 const& other) const {
        return Matrix4{
            row(0).dot(other.col[0]),
            row(0).dot(other.col[1]),
            row(0).dot(other.col[2]),
            row(0).dot(other.col[3]),
            row(1).dot(other.col[0]),
            row(1).dot(other.col[1]),
            row(1).dot(other.col[2]),
            row(1).dot(other.col[3]),
            row(2).dot(other.col[0]),
            row(2).dot(other.col[1]),
            row(2).dot(other.col[2]),
            row(2).dot(other.col[3]),
            row(3).dot(other.col[0]),
            row(3).dot(other.col[1]),
            row(3).dot(other.col[2]),
            row(3).dot(other.col[3]),
        };
    }

    Vector4<T> operator*(Vector4<T> const& other) const {
        return Vector4<T>{
            row(0).dot(other),
            row(1).dot(other),
            row(2).dot(other),
            row(3).dot(other),
        };
    }

    static Matrix4 IDENTITY;
};

template<typename T>
Matrix4<T> Matrix4<T>::IDENTITY = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1,
};

using float4x4 = Matrix4<float>;
using int4x4 = Matrix4<long>;

inline float4x4 projection(float x = 0.1, float near = 0.1, float far = 100.0) {
    float4x4 m;
    m[0][0] = near / x;
    m[1][1] = near / x;
    m[2][2] = -(far + near) / (far - near);
    m[3][2] = -(2 * far * near) / (far - near);
    m[2][3] = -1;
    return m;
}

inline float4x4 translate(float x, float y, float z) {
    float4x4 m;
    m[0][0] = 1;
    m[1][1] = 1;
    m[2][2] = 1;
    m[3][3] = 1;
    m[3][0] = x;
    m[3][1] = y;
    m[3][2] = z;
    return m;
}

inline float4x4 rotateX(float angle) {
    float4x4 m;
    float sin = std::sinf(angle);
    float cos = std::cosf(angle);
    m[0][0] = 1;
    m[1][1] = cos;
    m[2][1] = sin;
    m[1][2] = -sin;
    m[2][2] = cos;
    m[3][3] = 1;
    return m;
}

inline float4x4 rotateY(float angle) {
    float4x4 m;
    float sin = std::sinf(angle);
    float cos = std::cosf(angle);
    m[0][0] = cos;
    m[2][0] = sin;
    m[1][1] = 1;
    m[0][2] = -sin;
    m[2][2] = cos;
    m[3][3] = 1;
    return m;
}
