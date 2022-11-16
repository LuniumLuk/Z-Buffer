/**
 * This is an octree implemented for partition triangle mesh
 * How to use:
 *  1. Create Octree Node with given bounding:
 *      ```
 *      float3 center(0, 0, 0);
 *      float3 halfExtend(1, 1, 1); // half of the bounding extend
 *      auto tree = new Octree(center, halfExtend);
 *      ```
 *  2. Create OctreeData node:
 *      ```
 *      float3 v0, v1, v2;
 *      auto data = new OctreeData(v0, v1, v2);
 *      ```
 *  3. Insert data node:
 *      ```
 *      tree->insert(data);
 *      ```
 */

#pragma once

#include "vector.h"
#include "image.h"
#include "utils.h"
#include <vector>
#include <iostream>

#define THRESHOLD_TO_SUBDIVIDE 4

struct OctreeData {
    float3 v[3];
    float3 min, max;

    OctreeData(float3 const& v0, float3 const& v1, float3 const& v2)
        : v{v0, v1, v2}
        , min(float3::min(float3::min(v[0], v[1]), v[2]))
        , max(float3::max(float3::max(v[0], v[1]), v[2])) {}
};

struct Octree {
    float3 center;
    float3 halfExtent;
    std::vector<OctreeData*> datas;
    /**
     * Octree children are defined as follow:
     *   0 1 2 3 4 5 6 7
     * x - - - - + + + + (w.r.t center of parent node)
     * y - - + + - - + +
     * z - + - + - + - +
     */
    Octree* children[8];

    Octree(float3 const& center, float3 const& halfExtent)
        : center(center)
        , halfExtent(halfExtent) {
        for (int i = 0; i < 8; ++i) children[i] = nullptr;
    }

    Octree(const Octree&) = delete;
    Octree(Octree&&) = delete;

    ~Octree() {
        for (int i = 0; i < datas.size(); ++i) delete datas[i];
        for (int i = 0; i < 8; ++i) if (children[i]) delete children[i];
    }


    bool isLeaf() const { return children[0] == nullptr; };

    void insert(OctreeData* d) {
        if (datas.size() < THRESHOLD_TO_SUBDIVIDE) {
            datas.push_back(d);
            return;
        }
        if (isLeaf()) {
            // Leaf node that contains neighter data nor child,
            // subdivide and insert data into corresponding child.
            for (int i = 0; i < 8; ++i) {
                auto c = center;
                c.x += halfExtent.x * (i & 4 ? .5f : -.5f);
                c.y += halfExtent.y * (i & 2 ? .5f : -.5f);
                c.z += halfExtent.z * (i & 1 ? .5f : -.5f);
                children[i] = new Octree(c, halfExtent * .5f);
            }
        }
        int child = getChildContainingData(d);
        if (child == 8) {
            datas.push_back(d);
        }
        else {
            children[child]->insert(d);
        }
    }

    void drawWireframe(Image & image, colorf const& color) {
        const int2 lines[12] = {
            {0, 1}, {2, 3}, {4, 5}, {6, 7},
            {0, 2}, {1, 3}, {4, 6}, {5, 7},
            {0, 4}, {1, 5}, {2, 6}, {3, 7},
        };

        for (int i = 0; i < 12; ++i) {
            auto v0 = center;
            auto v1 = center;

            v0.x += halfExtent.x * (lines[i][0] & 4 ? 1 : -1);
            v0.y += halfExtent.y * (lines[i][0] & 2 ? 1 : -1);
            v1.x += halfExtent.x * (lines[i][1] & 4 ? 1 : -1);
            v1.y += halfExtent.y * (lines[i][1] & 2 ? 1 : -1);

            auto vs0 = int2(ftoi((v0.x * 0.5f + 0.5f) * image.width), ftoi((v0.y * 0.5f + 0.5f) * image.height));
            auto vs1 = int2(ftoi((v1.x * 0.5f + 0.5f) * image.width), ftoi((v1.y * 0.5f + 0.5f) * image.height));

            image.drawLine(vs0, vs1, color);
        }

        if (!isLeaf()) for (int i = 0; i < 8; ++i) children[i]->drawWireframe(image, color);
    }

private:
    /**
     * Octree children are defined as follow:
     *   0 1 2 3 4 5 6 7
     * x - - - - + + + + (w.r.t center of parent node)
     * y - - + + - - + +
     * z - + - + - + - +
     */
    int getChildContainingData(OctreeData* d) const {
        int child = 0;
        if (d->max.x > center.x) child |= 0b100000;
        if (d->max.y > center.y) child |= 0b010000;
        if (d->max.z > center.z) child |= 0b001000;
        if (d->min.x > center.x) child |= 0b000100;
        if (d->min.y > center.y) child |= 0b000010;
        if (d->min.z > center.z) child |= 0b000001;
        
        switch (child) {
        case 0b000000: return 0;
        case 0b001001: return 1;
        case 0b010010: return 2;
        case 0b011011: return 3;
        case 0b100100: return 4;
        case 0b101101: return 5;
        case 0b110110: return 6;
        case 0b111111: return 7;
        default:       return 8;
        }
    }

};
