#pragma once

#include "vector.h"
#include <vector>

struct Triangle {
    float3 v[3];

    float3 min() const { return float3::min(v[0], float3::min(v[1], v[2])); }
    float3 max() const { return float3::max(v[0], float3::max(v[1], v[2])); }
};

enum struct OctreeSubdivision {
    RightTopFront,      // +x +y +z
    RightTopBack,       // +x +y -z
    RightBottomFront,   // +x -y +z
    RightBottomBack,    // +x -y -z
    LeftTopFront,       // -x +y +z
    LeftTopBack,        // -x +y -z
    LeftBottomFront,    // -x -y +z
    LeftBottomBack,     // -x -y -z
    Center,
};

struct OctreeNode {

    float3 min;
    float3 max;
    float3 center;
    std::vector<Triangle> mTriangles;
    std::vector<OctreeNode> mChildern;

    bool isLeaf() const { return mChildern.size() == 0; };
    bool isBlank() const { return mTriangles.size() == 0 && isLeaf(); };

    OctreeNode(std::vector<Triangle> const& triangles, float3 const& _min, float3 const& _max);

private:
    OctreeSubdivision getPartition(Triangle const& triangle) const;

    OctreeNode createChildNode(OctreeSubdivision p, std::vector<Triangle> const& triangles) const;
};
