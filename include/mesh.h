#pragma once

#include <vector>
#include <string>
#include <cstring>
#include "vector.h"

#define MAX_LINE_NUM 256

struct TriangleMesh {
    std::vector<float3> vertices;
    std::vector<int3> indices;
    float3 center;
    float3 min;
    float3 max;

    TriangleMesh() = delete;
    TriangleMesh(std::string const& path);
};
