#pragma once

#include "buffer.h"
#include "vector.h"
#include "matrix.h"

#define PERSPECTIVE_DEVIDE(v) v.w=1/v.w;v.x*=v.w;v.y*=v.w;v.z*=v.w

struct ZBHierarchical {
    int width;
    int height;
    HierarchicalZBuffer depthBuffer;

    ZBHierarchical(int w, int h)
        : width(w)
        , height(h)
        , depthBuffer(w, h) {
        
    }

    void drawMesh(std::vector<float3> const& vertices,
                  std::vector<int3> const& indices, 
                  std::vector<colorf> const& colors,
                  float4x4 const& mvp) {
        
        for (int i = 0; i < indices.size(); ++i) {
            auto v0 = float4(vertices[indices[i][0]], 1.0f);
            auto v1 = float4(vertices[indices[i][1]], 1.0f);
            auto v2 = float4(vertices[indices[i][2]], 1.0f);

            v0 = mvp * v0;
            v1 = mvp * v1;
            v2 = mvp * v2;

            PERSPECTIVE_DEVIDE(v0);
            PERSPECTIVE_DEVIDE(v1);
            PERSPECTIVE_DEVIDE(v2);

            
        }

    }

};