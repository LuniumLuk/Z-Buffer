#pragma once

#include "matrix.h"

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
    float sin = sinf(angle);
    float cos = cosf(angle);
    float4x4 m;
    m[0][0] = 1;
    m[1][1] = cos;
    m[2][1] = sin;
    m[1][2] = -sin;
    m[2][2] = cos;
    m[3][3] = 1;
    return m;  
}

inline float4x4 rotateY(float angle) {
    float sin = sinf(angle);
    float cos = cosf(angle);
    float4x4 m;
    m[0][0] = cos;
    m[2][0] = sin;
    m[1][1] = 1;
    m[0][2] = -sin;
    m[2][2] = cos;
    m[3][3] = 1;
    return m;
}
