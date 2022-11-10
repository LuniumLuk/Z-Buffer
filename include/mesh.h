#pragma once

#include <vector>
#include <string>
#include "vector.h"

#define MAX_LINE_NUM 256

struct TriangleMesh {
    std::vector<float3> vertices;
    std::vector<int3> indices;

    TriangleMesh() = delete;
    TriangleMesh(std::string const& path);

    float3 center() const;
    float3 max() const;
    float3 min() const;
};
