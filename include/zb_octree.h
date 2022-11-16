#pragma once

#include "buffer.h"
#include "vector.h"
#include "matrix.h"
#include "utils.h"
#include "mesh.h"
#include "image.h"
#include "octree.h"
#include "timer.h"

#if defined(BENCHMARK)
long g_counter = 0;
double g_octree_time = 0.0;
double g_render_time = 0.0;
#endif

struct ZBOctree {
    int width;
    int height;
    HierarchicalZBuffer depth;
    Octree* octree;
#if defined(BENCHMARK)
    Timer timer0, timer1;
#endif

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
                  bool display_octree = false,
                  colorf const& octree_color = colorf(1.0f)) {
        
#if defined(BENCHMARK)
        timer0.update();
#endif
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

#if defined(BENCHMARK)
        timer1.update();
#endif
        // Additional pass to build octree.
        if (octree) delete octree; // Delete previous octree.
        octree = new Octree((max + min) / 2, (max - min) / 2);
        for (int i = 0; i < mesh.indices.size(); ++i) {
            auto v0 = ndc[mesh.indices[i][0]];
            auto v1 = ndc[mesh.indices[i][1]];
            auto v2 = ndc[mesh.indices[i][2]];
            auto d = new OctreeData(v0, v1, v2);
            octree->insert(d);
        }
#if defined(BENCHMARK)
        timer1.update();
        g_octree_time += timer1.deltaTime();
#endif
        // Cull mesh if out of screen.
        if (min.x > 1 || max.x < -1
         || min.y > 1 || max.y < -1 
         || min.z > 1 || max.z <  0 ) return;

        for (int i = 0; i < mesh.indices.size(); ++i) {
            auto v0 = ndc[mesh.indices[i][0]];
            auto v1 = ndc[mesh.indices[i][1]];
            auto v2 = ndc[mesh.indices[i][2]];

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

            auto level = getMinBoundingLevel(x_min, x_max, y_min, y_max);
            if (min.z > depth.at((x_min + x_max) / 2, (y_min + y_max) / 2, level)) continue;

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
                    image.setPixel(x, y, colors[i]);
                }
            }
        }

        if (display_octree) {
            octree->drawWireframe(image, octree_color);
        }
#if defined(BENCHMARK)
        timer0.update();
        g_render_time += timer0.deltaTime();
        g_counter++;
#endif
    }

public:
    int getMinBoundingLevel(int x_min, int x_max, int y_min, int y_max) {
        auto x = (x_min + x_max) / 2;
        auto y = (y_min + y_max) / 2;

        auto x_bound_min = 0;
        auto x_bound_max = width - 1;
        auto y_bound_min = 0;
        auto y_bound_max = height - 1;

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