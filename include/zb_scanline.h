#include "utils.h"
#include "vector.h"
#include "matrix.h"
#include "image.h"
#include <vector>
#include <list>

struct ZBScanline {
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

    int width, height;

    ZBScanline(int w, int h)
        : width(w)
        , height(h) {}
    
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

        std::vector<Triangle> triangles;

        for (size_t i = 0; i < indices.size(); ++i) {
            Triangle t;

            auto v0 = ndc[indices[i][0]];
            auto v1 = ndc[indices[i][1]];
            auto v2 = ndc[indices[i][2]];

            // Screen mapping.
            t.v[0] = screenMapping(v0, width, height);
            t.v[1] = screenMapping(v1, width, height);
            t.v[2] = screenMapping(v2, width, height);

            // Store z value.
            t.z[0] = v0.z;
            t.z[1] = v1.z;
            t.z[2] = v2.z;

            float3 e01 = v1 - v0;
            float3 e02 = v2 - v0;
            float3 n = e01.cross(e02);

            // Surface of the triangle is defined by:
            // s_x * x + s_y * y + s_z * z = s_w
            t.surface.x = n.x;
            t.surface.y = n.y;
            t.surface.z = n.z;
            t.surface.w = n.dot(v0);

            triangles.push_back(t);
        }

        SortedEdgeTable SET(triangles, width, height);

        int y_min = SET.min.y;
        int y_max = SET.max.y;
        std::list<Edge> AEL;
        std::vector<float> z_buffer(image.width);
        // Scan the bounding area of the polygon.
        for (int y = y_min; y <= y_max; ++y) {
            for (int x = 0; x < image.width; ++x) {
                z_buffer[x] = 1.0f;
            }

            // Append new edges.
            for (size_t j = 0; j < SET.table[y - y_min].size(); ++j) {
                Edge e = SET.table[y - y_min][j];
                AEL.push_back(e);
            }

            if (y >= 0 && y < image.height) {
                // Sort edges by x.
                AEL.sort(compareEdge);
                assert(AEL.size() % 2 == 0);
                // Fill pixels between every pair of edges.
                auto e0 = AEL.begin();
                auto e1 = std::next(e0);
                while (e0 != AEL.end()) {
                    int x = ftoi(e0->x);
                    int x_max = ftoi(e1->x);
                    float z = e0->z;
                    while (x <= x_max) {
                        if (z >= -1 && z <= 1 && x >= 0 && x < image.width) {
                            if (z < z_buffer[x]) {
                                z_buffer[x] = z;
                                image.setPixel(x, y, colors[e0->id]);
                            }
                        }
                        z += e0->dzdx;
                        ++x;
                    }
                    e0 = std::next(e1);
                    e1 = std::next(e0);
                }
            }

            // Update x in edges.
            for (auto iter = AEL.begin(); iter != AEL.end(); ++iter) {
                iter->x += iter->dx;
                iter->z += iter->dzdx * iter->dx + iter->dzdy;
            }
            // Remove used edges.
            AEL.remove_if([=](Edge e){ return e.y_max == y; });
        }
        AEL.clear();
    }

private:
    int2 screenMapping(float3 const& v, float w, float h) {
        return int2{
            ftoi((v.x + 1.0f) * 0.5f * w),
            ftoi((v.y + 1.0f) * 0.5f * h),
        };
    }

    static bool compareEdge(Edge const& first, Edge const& second) {
        if (first.id != second.id) {
            return first.id < second.id;
        } else {
            return first.x < second.x;
        }
    }

};

ZBScanline::SortedEdgeTable::SortedEdgeTable(std::vector<Triangle> const& tris, int w, int h) {
    int const tri_num = tris.size();
    int const vn = 3; // Here we consider only triangles.
    initTable(tris);
    for (int i = 0; i < tri_num; ++i) {
        for (int j = 0; j < vn; ++j) {
            Edge e;
            int v0 = j;
            int v1 = (j + 1) % vn;
            if (tris[i].v[v0].y == tris[i].v[v1].y) continue;
            if (tris[i].v[v0].y > tris[i].v[v1].y) {
                // Use the vertex with smaller y as v0.
                std::swap(v0, v1);
            }
            e.y_max = tris[i].v[v1].y;
            e.dx = (float)(tris[i].v[v1].x - tris[i].v[v0].x) / (tris[i].v[v1].y - tris[i].v[v0].y);
            e.x = itof(tris[i].v[v0].x);
            e.id = i;
            if (tris[i].surface.z == 0) {
                // The surface is parallel to z-axis.
                e.dzdx = e.dzdy = 0;
            }
            else {
                e.dzdx = -tris[i].surface.x / tris[i].surface.z;
                e.dzdy = -tris[i].surface.y / tris[i].surface.z;
            }
            e.dzdx /= (w / 2);
            e.dzdy /= (h / 2);
            e.z = tris[i].z[v0];

            // Determine whether v0 is at a singularity position that is
            // both edge connected to it is on the same side of the scanline.
            int prev = (v0 - 1 + vn) % vn;
            int next = (v0 + 1 + vn) % vn;
            int y = tris[i].v[v0].y - min.y;
            // If not at singularity position.
            if ((tris[i].v[next].y - tris[i].v[v0].y) * (tris[i].v[prev].y - tris[i].v[v0].y) < 0) {
                // move this vertex up by 1 to avoid handling this case when rasterizing.
                e.x += e.dx;
                e.z += e.dzdx * e.dx + e.dzdy;
                ++y;
            }
            
            table[y].emplace_back(e);
        }
    }
}

void ZBScanline::SortedEdgeTable::initTable(std::vector<Triangle> const& tris) {
    // Get bounding of all triangles.
    min = { std::numeric_limits<long>::max(), std::numeric_limits<long>::max() };
    max = { std::numeric_limits<long>::min(), std::numeric_limits<long>::min() };
    for (size_t i = 0; i < tris.size(); ++i) {
        min = int2::min(min, tris[i].min());
        max = int2::max(max, tris[i].max());
    }
    int table_size = max.y - min.y + 1;
    table.resize(table_size);
}
