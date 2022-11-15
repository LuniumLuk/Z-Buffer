#pragma once

#include "buffer.h"
#include "vector.h"
#include "matrix.h"
#include "utils.h"
#include "image.h"
#include <iostream>

struct ZBSimple {
    int width;
    int height;
    ZBuffer depth;

    ZBSimple(int w, int h)
        : width(w)
        , height(h)
        , depth(w, h) {
        
    }

    void clearDepth() {
        depth.clear(1.0f);
    }

    void drawMesh(std::vector<float3> const& vertices,
                  std::vector<int3> const& indices, 
                  std::vector<colorf> const& colors,
                  float4x4 const& mvp,
                  Image & image) {
                
        std::vector<float3> ndc;
        for (int i = 0; i < vertices.size(); ++i) {
            auto v = float4(vertices[i], 1.0f);
            v = mvp * v;
            v.w = 1 / v.w;
            v.x *= v.w;
            v.y *= v.w;
            v.z *= v.w;
            ndc.push_back(float3(v));
        }

        for (int i = 0; i < indices.size(); ++i) {
            auto v0 = ndc[indices[i][0]];
            auto v1 = ndc[indices[i][1]];
            auto v2 = ndc[indices[i][2]];

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

            // Perspective-correct interpolation.
            v0.z = 1 / v0.z;
            v1.z = 1 / v1.z;
            v2.z = 1 / v2.z;

            auto area = edgeFunction2D(v0, v1, v2);
            if (area == 0) continue;

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

                    if (pos.z > depth.at(x, y)) continue;

                    depth.write(x, y, pos.z);
                    image.setPixel(x, y, colors[i]);
                }
            }
        }
    }

};