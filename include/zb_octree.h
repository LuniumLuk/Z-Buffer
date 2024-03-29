#pragma once

#include "buffer.h"
#include "vector.h"
#include "matrix.h"
#include "utils.h"
#include "mesh.h"
#include "image.h"
#include "octree.h"
#include "timer.h"

struct ZBOctree {
    int width;
    int height;
    HierarchicalZBuffer depth;
    Octree* octree;
    std::vector<Octree*> octree_cache;
    bool fixed = false;

    ZBOctree(int w, int h)
        : width(w)
        , height(h)
        , depth(w, h)
        , octree(nullptr) {}

    ~ZBOctree() {
        if (octree) delete octree;
    }

    void clearDepth() {
        depth.clear(1.0f);
    }
    
    void drawMesh(TriangleMesh const& mesh,
                  std::vector<colorf> const& colors,
                  float4x4 const& mvp,
                  Image & image,
                  unsigned int transform_id = 0,
                  bool display_octree = false,
                  colorf const& octree_color = colorf(1.0f)) {
        
        std::vector<float3> ndc;
        float3 min = float3(std::numeric_limits<float>::max());
        float3 max = float3(std::numeric_limits<float>::min());
        for (int i = 0; i < mesh.vertices.size(); ++i) {
            auto v = float4(mesh.vertices[i], 1.0f);
            v = mvp * v;
            v.w = 1 / v.w;
            v.x *= v.w;
            v.y *= v.w;
            v.z *= v.w;
            min = float3::min(min, v);
            max = float3::max(max, v);
            ndc.push_back(float3(v));
        }

        // Cull mesh if out of screen.
        if (min.x > 1 || max.x < -1
         || min.y > 1 || max.y < -1 
         || min.z > 1 || max.z <  0 ) return;


        if (octree && !fixed) {
            delete octree; // Delete previous octree.
            octree = nullptr;
        }
        else if (fixed) {
            octree = nullptr;
            if (octree_cache.size() <= transform_id) {
                octree_cache.resize(transform_id + 1, nullptr);
            }
            if (octree_cache[transform_id]) {
                octree = octree_cache[transform_id];
            }
        }

        if (!octree) {
            octree = new Octree((max + min) / 2, (max - min) / 2);
            for (int i = 0; i < mesh.indices.size(); ++i) {
                auto v0 = ndc[mesh.indices[i][0]];
                auto v1 = ndc[mesh.indices[i][1]];
                auto v2 = ndc[mesh.indices[i][2]];
                auto d = new OctreeData(v0, v1, v2, i);
                octree->insert(d);
            }
        }

        if (fixed) {
            octree_cache[transform_id] = octree;
        }

        drawOctree(octree, colors, image);

        if (display_octree) {
            octree->drawWireframe(image, octree_color);
        }
    }

public:
    void drawOctree(Octree * tree,
                    std::vector<colorf> const& colors,
                    Image & image) {

        if (!tree) return;

        // Render triangle at the center of the node.
        for (int i = 0; i < tree->datas.size(); ++i) {
            auto data = tree->datas[i];
            auto v0 = data->v[0];
            auto v1 = data->v[1];
            auto v2 = data->v[2];

            // Back-face culling.
            auto e01 = v1 - v0;
            auto e02 = v2 - v0;
            auto N = e01.cross(e02);
            if (N.z < 0) continue;

            // Screen mapping.
            v0.x = ftoi((v0.x * 0.5f + 0.5f) * width);
            v1.x = ftoi((v1.x * 0.5f + 0.5f) * width);
            v2.x = ftoi((v2.x * 0.5f + 0.5f) * width);
            v0.y = ftoi((v0.y * 0.5f + 0.5f) * height);
            v1.y = ftoi((v1.y * 0.5f + 0.5f) * height);
            v2.y = ftoi((v2.y * 0.5f + 0.5f) * height);

            auto min = float3::min(v0, float3::min(v1, v2));
            auto max = float3::max(v0, float3::max(v1, v2));

            auto x_min = clamp(ftoi(min.x), 0, width - 1);
            auto x_max = clamp(ftoi(max.x), 0, width - 1);
            auto y_min = clamp(ftoi(min.y), 0, height - 1);
            auto y_max = clamp(ftoi(max.y), 0, height - 1);

            // Cull triangle if out of screen.
            if (x_min > width  || x_max < 0
             || y_min > height || y_max < 0) continue;

            // Perspective-correct interpolation.
            v0.z = 1 / v0.z;
            v1.z = 1 / v1.z;
            v2.z = 1 / v2.z;

            auto area = edgeFunction2D(v0, v1, v2);
            if (area == 0) continue;

            // auto level = getMinBoundingLevel(x_min, x_max, y_min, y_max);
            // if (min.z > depth.at((x_min + x_max) / 2, (y_min + y_max) / 2, level)) continue;

            for (int x = x_min; x <= x_max; ++x) {
                for (int y = y_min; y <= y_max; ++y) {
                    auto pos = float3(x, y, 1);
                    
                    float w0, w1, w2;
                    if (outsideTest2D(v0, v1, v2, pos, &w0, &w1, &w2)) continue;

                    w0 /= area;
                    w1 /= area;
                    w2 /= area;

                    auto denom = (w0 * v0.z + w1 * v1.z + w2 * v2.z);
                    pos.z = 1.0f / denom;

                    if (pos.z < 0 || pos.z > 1) continue;

                    if (pos.z > depth.at(x, y, 0)) continue;
                    
                    depth.write(x, y, pos.z);
                    image.setPixel(x, y, colors[data->id]);
                }
            }
        }

        if (!tree->isLeaf()) {
            for (int i = 0; i < 8; ++i) {
                if (!tree->children[i]) continue;

                if (depthTestOctree(tree->children[i])) {
                    drawOctree(tree->children[i], colors, image);
                }
                else {
                    // Skip next child if this child is in the
                    // negative half of z-axis
                    if (i % 2 == 0) ++i;
                }
            }
        }
    }

    bool depthTestOctree(Octree * tree) {
        auto x_min = clamp(ftoi(((tree->center.x - tree->halfExtent.x) * 0.5f + 0.5f) * width), 0, width - 1);
        auto x_max = clamp(ftoi(((tree->center.x + tree->halfExtent.x) * 0.5f + 0.5f) * width), 0, width - 1);
        auto y_min = clamp(ftoi(((tree->center.y - tree->halfExtent.y) * 0.5f + 0.5f) * height), 0, height - 1);
        auto y_max = clamp(ftoi(((tree->center.y + tree->halfExtent.y) * 0.5f + 0.5f) * height), 0, height - 1);
        auto z_min = tree->center.z - tree->halfExtent.z;

        auto level = getMinBoundingLevel(x_min, x_max, y_min, y_max);
        if (z_min > depth.at((x_min + x_max) / 2, (y_min + y_max) / 2, level)) return false;
        
        return true;
    }

    int getMinBoundingLevel(int x_min, int x_max, int y_min, int y_max) {
        auto x = (x_min + x_max) / 2;
        auto y = (y_min + y_max) / 2;

        auto x_bound_min = 0;
        auto x_bound_max = width - 1;
        auto y_bound_min = 0;
        auto y_bound_max = height - 1;

        // Here approximate level is not practical since
        // level is not only associated with bounding extend
        // but also highly associated with [x, y].
        auto level = depth.maxLevel();
        while (level > 0
            && x_bound_min <= x_min && x_bound_max >= x_max
            && y_bound_min <= y_min && y_bound_max >= y_max) {
            --level;
            
            auto w = width / depth.mip_w[level];
            auto h = height / depth.mip_h[level];
            x_bound_min = x & ~(w - 1);
            x_bound_max = (x + w - 1) & ~(w - 1);
            y_bound_min = y & ~(h - 1);
            y_bound_max = (y + h - 1) & ~(h - 1);
        }
        return level;
    }

};