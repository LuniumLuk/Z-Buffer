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

inline float4x4 ortho(float left, float right, float bottom, float top, float near, float far) {
    float4x4 m;
    m[0][0] = 2 / (right - left);
    m[1][1] = 2 / (top - bottom);
    m[2][2] = -2 / (far - near);
    // m[0][3] = -(right + left) / (right - left);
    // m[1][3] = -(top + bottom) / (top - bottom);
    // m[2][3] = -(far + near) / (far - near);
    m[3][0] = -(right + left) / (right - left);
    m[3][1] = -(top + bottom) / (top - bottom);
    m[3][2] = -(far + near) / (far - near);
    m[3][3] = 1;
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

inline float4x4 lookAt(float3 const& eye, float3 const& at, float3 const& up) {
    float3 forward = (eye - at).normalized();
    float3 left = up.cross(forward).normalized();
    float3 upward = forward.cross(left);
    float4x4 m(
        left.x,     left.y,     left.z,     -(left.x * eye.x + left.y * eye.y + left.z * eye.z),
        upward.x,   upward.y,   upward.z,   -(upward.x * eye.x + upward.y * eye.y + upward.z * eye.z),
        forward.x,  forward.y,  forward.z,  -(forward.x * eye.x + forward.y * eye.y + forward.z * eye.z),
        0.0f,       0.0f,       0.0f,       1.0f
    );
    return m;
}