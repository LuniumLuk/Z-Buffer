#pragma once

#include "buffer.h"
#include "vector.h"
#include "matrix.h"
#include "utils.h"
#include "image.h"
#include <iostream>

static inline float edgeFunction(const float3& a, const float3& b, const float3& c) {
    return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

static inline bool outsideTest(float3 const& v0, float3 const& v1, float3 const& v2, float3 const& pos, float* w0, float* w1, float* w2) {
    *w0 = edgeFunction(v1, v2, pos);
    *w1 = edgeFunction(v2, v0, pos);
    *w2 = edgeFunction(v0, v1, pos);

    bool has_neg = *w0 < 0 || *w1 < 0 || *w2 < 0;
    bool has_pos = *w0 > 0 || *w1 > 0 || *w2 > 0;
    return has_neg && has_pos;
}

struct ZBSimple {
    int width;
    int height;
    ZBuffer depth;

    ZBSimple(int w, int h)
        : width(w)
        , height(h)
        , depth(w, h) {
        
    }

    void drawMesh(std::vector<float3> const& vertices,
                  std::vector<int3> const& indices, 
                  std::vector<colorf> const& colors,
                  float4x4 const& mvp,
                  Image & image) {
        
        depth.clear(1.0f);
        
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

            // Back face culling.
            auto e01 = v1 - v0;
            auto e02 = v2 - v0;
            auto N = e01.cross(e02);
            if (N.z < 0) continue;

            auto min = float3::min(v0, float3::min(v1, v2));
            auto max = float3::max(v0, float3::max(v1, v2));
            min.x = clamp(min.x, -1.0f, 1.0f);
            max.x = clamp(max.x, -1.0f, 1.0f);
            min.y = clamp(min.y, -1.0f, 1.0f);
            max.y = clamp(max.y, -1.0f, 1.0f);

            v0.z = 1 / v0.z;
            v1.z = 1 / v1.z;
            v2.z = 1 / v2.z;

            v0.x = ftoi((v0.x * 0.5f + 0.5f) * width);
            v1.x = ftoi((v1.x * 0.5f + 0.5f) * width);
            v2.x = ftoi((v2.x * 0.5f + 0.5f) * width);
            v0.y = ftoi((v0.y * 0.5f + 0.5f) * height);
            v1.y = ftoi((v1.y * 0.5f + 0.5f) * height);
            v2.y = ftoi((v2.y * 0.5f + 0.5f) * height);

            auto area = edgeFunction(v0, v1, v2);

            auto x_min = ftoi((min.x * 0.5f + 0.5f) * width);
            auto x_max = ftoi((max.x * 0.5f + 0.5f) * width);
            auto y_min = ftoi((min.y * 0.5f + 0.5f) * height);
            auto y_max = ftoi((max.y * 0.5f + 0.5f) * height);

            x_min = clamp(x_min, 0, width - 1);
            x_max = clamp(x_max, 0, width - 1);
            y_min = clamp(y_min, 0, width - 1);
            y_max = clamp(y_max, 0, width - 1);

            for (int x = x_min; x <= x_max; ++x) {
                for (int y = y_min; y <= y_max; ++y) {
                    auto pos = float3(x, y, 1);
                    
                    float w0, w1, w2;
                    if (outsideTest(v0, v1, v2, pos, &w0, &w1, &w2)) continue;

                    w0 /= area;
                    w1 /= area;
                    w2 /= area;

                    auto denom = (w0 * v0.z + w1 * v1.z + w2 * v2.z);
                    pos.z = 1.0f / denom;

                    if (pos.z < depth.depth(x, y)) {
                        depth.write(x, y, pos.z);
                        image.setPixel(x, y, colors[i]);
                    }
                }
            }
        }
    }

};