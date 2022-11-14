#include "../include/octree.h"

OctreeNode::OctreeNode(std::vector<Triangle> const& triangles, float3 const& _min, float3 const& _max)
    : min(_min)
    , max(_max)
    , center((_min + _max) / 2) {
    
    // If triangle num is less equal to 2, return a leaf node.
    if (triangles.size() <= 2) {
        for (auto iter = triangles.begin(); iter != triangles.end(); ++iter) {
            mTriangles.push_back(*iter);
        }
        return;
    }
    std::vector<Triangle> subdivision[8];
    for (auto iter = triangles.begin(); iter != triangles.end(); ++iter) {
        OctreeSubdivision p = getPartition(*iter);
        if (p < OctreeSubdivision::Center) {
            subdivision[(int)p].push_back(*iter);
        }
        else {
            mTriangles.push_back(*iter);
        }
    }

    for (int i = 0; i < 8; ++i) {
        OctreeSubdivision p = (OctreeSubdivision)i;
        OctreeNode node = createChildNode(p, subdivision[i]);
        mChildern.emplace_back(node);
    }
}

OctreeSubdivision OctreeNode::getPartition(Triangle const& triangle) const {
    auto t_min = triangle.min();
    auto t_max = triangle.max();

    int p = 0;

    if (t_max.x > center.x) p |= 0b100000;
    if (t_max.y > center.y) p |= 0b010000;
    if (t_max.z > center.z) p |= 0b001000;
    if (t_min.x > center.x) p |= 0b000100;
    if (t_min.y > center.y) p |= 0b000010;
    if (t_min.z > center.z) p |= 0b000001;

    switch (p) {
    case 0b111111: return OctreeSubdivision::RightTopFront;      // +x +y +z
    case 0b110110: return OctreeSubdivision::RightTopBack;       // +x +y -z
    case 0b101101: return OctreeSubdivision::RightBottomFront;   // +x -y +z
    case 0b100100: return OctreeSubdivision::RightBottomBack;    // +x -y -z
    case 0b011011: return OctreeSubdivision::LeftTopFront;       // -x +y +z
    case 0b010010: return OctreeSubdivision::LeftTopBack;        // -x +y -z
    case 0b001001: return OctreeSubdivision::LeftBottomFront;    // -x -y +z
    case 0b000000: return OctreeSubdivision::LeftBottomBack;     // -x -y -z
    default:       return OctreeSubdivision::Center;
    }
}

OctreeNode OctreeNode::createChildNode(OctreeSubdivision p, std::vector<Triangle> const& triangles) const {
    float3 child_min;
    float3 child_max;
    switch (p) {
    case OctreeSubdivision::RightTopFront:
        child_min = float3(center.x, center.y, center.z);
        child_max = float3(max.x, max.y, max.z);
        break;
    case OctreeSubdivision::RightTopBack:
        child_min = float3(center.x, center.y, min.z);
        child_max = float3(max.x, max.y, center.z);
        break;  
    case OctreeSubdivision::RightBottomFront:
        child_min = float3(center.x, min.y, center.z);
        child_max = float3(max.x, center.y, max.z);
        break;  
    case OctreeSubdivision::RightBottomBack:
        child_min = float3(center.x, min.y, min.z);
        child_max = float3(max.x, center.y, center.z);
        break;
    case OctreeSubdivision::LeftTopFront:
        child_min = float3(min.x, center.y, center.z);
        child_max = float3(center.x, max.y, max.z);
        break;
    case OctreeSubdivision::LeftTopBack:
        child_min = float3(min.x, center.y, min.z);
        child_max = float3(center.x, max.y, center.z);
        break;
    case OctreeSubdivision::LeftBottomFront:
        child_min = float3(min.x, min.y, center.z);
        child_max = float3(center.x, center.y, max.z);
        break;
    case OctreeSubdivision::LeftBottomBack:
        child_min = float3(min.x, min.y, min.z);
        child_max = float3(center.x, center.y, center.z);
        break;
    default: break;
    }
    return OctreeNode(triangles, child_min, child_max);
}
