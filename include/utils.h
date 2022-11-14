#pragma once

#include <cmath>
#include <algorithm>

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
