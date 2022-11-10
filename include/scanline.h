#pragma once

#include "../include/vector.h"
#include "../include/matrix.h"
#include "../include/image.h"
#include <vector>

struct Triangle {
    int2 v[3];
    float z[3];
    float4 surface; // surface equation

    int2 min() const { return int2::min(v[0], int2::min(v[1], v[2])); }
    int2 max() const { return int2::max(v[0], int2::max(v[1], v[2])); }
};

struct Edge {
    float x;
    float dx;
    long  y_max; // max y coordinate
    long  id;    // primitive id (triangle id in case of triangle mesh)
    float z;     // current z value on edge
    float dzdx;  // derivation of z w.r.t. x on the surface
    float dzdy;  // derivation of z w.r.t. y on the surface
};

struct SortedEdgeTable {
    int2 min;
    int2 max;
    std::vector<std::vector<Edge>> table;

    SortedEdgeTable(std::vector<Triangle> const& tris, int w, int h);

private:
    void initTable(std::vector<Triangle> const& tris);
};

// Rasterize content in sorted edge table and output to target image.
// - SET: sorted edge table
// - image: target image
// - colors: color array for each triangle
void scanlineFill(SortedEdgeTable const& SET, Image& image, std::vector<colorf> const& colors);

// Convert world space coordinates to clip space coordinates
std::vector<float4> toClipSpace(std::vector<float3> const& vs, float4x4 const& mvp);

// Convert clip space coordinates to Sorted Edge Table Triangles.
// - map coordinates to screen space coordinate: [-1, 1] -> [0, w or h]
// - calculate surface formula for each triangle
std::vector<Triangle> toScreenSpaceTriangles(std::vector<float4> const& vs, float w, float h);
