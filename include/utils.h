#pragma once

#include <cmath>
#include <algorithm>
#include "vector.h"

// Convert float value in scene to integer value in image space.
inline int ftoi(float x) {
    // Difference of (int)std::floor(x) and static_cast<int>(x):
    // - (int)std::floor(x) cast x toward -INFINITY
    // - static_cast<int>(x) cast x toward 0
    // Although we ignore negative value in image space, -0.5 will still be cast to 0
    // which is not correct theoretically.
    return std::floor(x);
}

// Convert integer value in image space to float value in scene.
inline float itof(int x) {
    return x + 0.5f;
}

template<typename T>
inline T clamp(T x, T min, T max) {
    x = x < min ? min : x;
    x = x > max ? max : x;
    return x;
}

constexpr float PI() { return 3.14159265; }
constexpr float PI_div_two() { return PI() / 2; }
constexpr float PI_div_three() { return PI() / 3; }
constexpr float PI_div_four() { return PI() / 4; }
constexpr float PI_mul_two() { return PI() * 2; }

static inline float edgeFunction2D(const float3& a, const float3& b, const float3& c) {
    return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

static inline bool outsideTest2D(float3 const& v0, float3 const& v1, float3 const& v2, float3 const& pos, float* w0, float* w1, float* w2) {
    *w0 = edgeFunction2D(v1, v2, pos);
    *w1 = edgeFunction2D(v2, v0, pos);
    *w2 = edgeFunction2D(v0, v1, pos);

    bool has_neg = *w0 < 0 || *w1 < 0 || *w2 < 0;
    bool has_pos = *w0 > 0 || *w1 > 0 || *w2 > 0;
    return has_neg && has_pos;
}
