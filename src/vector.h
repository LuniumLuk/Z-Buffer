#pragma once

template<typename T>
struct Vector2 {
    using dataType = T;
    dataType x, y;
};

template<typename T>
struct Vector3 {
    using dataType = T;
    dataType x, y, z;
};

template<typename T>
struct Vector4 {
    using dataType = T;
    union { dataType x, r; };
    union { dataType y, g; };
    union { dataType z, b; };
    union { dataType w, a; };
};

using float2 = Vector2<float>;
using int2 = Vector2<long>;
using float3 = Vector3<float>;
using int3 = Vector3<long>;
using float4 = Vector4<float>;
using int4 = Vector4<long>;
using colorf = Vector4<float>;