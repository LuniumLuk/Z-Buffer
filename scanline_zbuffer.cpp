#include <iostream>
#include <vector>
#include <list>
#include <cmath>
#include <limits>
#include <algorithm>
#include "include/vector.h"
#include "include/matrix.h"
#include "include/image.h"
#include "include/utils.h"

// This program is a demo of Scanline Z-buffer Algorithm
// -------------------------------------------------------
// to compile and run in vscode in MacOS:
// g++ -std=c++17 -Og scanline_zbuffer.cpp -o out && ./out

struct Triangle {
    int2 v[3];
    colorf color;
    float4 surface; // surface equation

    int2 min() const {
        return int2::min(v[0], int2::min(v[1], v[2]));
    }
    int2 max() const {
        return int2::max(v[0], int2::max(v[1], v[2]));
    }
};

struct Edge {
    float x;
    float dx;
    long  y_max; // max y coordinate
    long  id;    // primitive id (triangle id in case of triangle mesh)
    float z;     // current z
    float dzdx;  // derivation of z w.r.t. x on the surface
    float dzdy;  // derivation of z w.r.t. y on the surface
};

struct SortedEdgeTable {
    int2 min;
    int2 max;
    std::vector<std::vector<Edge>> table;
    std::vector<Triangle> *scene;

    SortedEdgeTable(std::vector<Triangle> *_scene) 
        : scene(_scene) {
        int const tri_num = scene->size();
        int const vn = 3; // Here we consider only triangles.
        initTable();
        for (int i = 0; i < tri_num; ++i) {
            for (int j = 0; j < vn; ++j) {
                Edge e;
                int v0 = j;
                int v1 = (j + 1) % vn;
                if ((*scene)[i].v[v0].y == (*scene)[i].v[v1].y) continue;
                if ((*scene)[i].v[v0].y > (*scene)[i].v[v1].y) {
                    // Use the vertex with smaller y as v0.
                    std::swap(v0, v1);
                }
                e.y_max = (*scene)[i].v[v1].y;
                e.dx = (float)((*scene)[i].v[v1].x - (*scene)[i].v[v0].x) / ((*scene)[i].v[v1].y - (*scene)[i].v[v0].y);
                e.x = itof((*scene)[i].v[v0].x);
                e.id = i;

                // Determine whether v0 is at a singularity position that is
                // both edge connected to it is on the same side of the scanline.
                int prev = (v0 - 1 + vn) % vn;
                int next = (v0 + 1 + vn) % vn;
                int y = (*scene)[i].v[v0].y - min.y;
                // If not at singularity position.
                if (((*scene)[i].v[next].y - (*scene)[i].v[v0].y) * ((*scene)[i].v[prev].y - (*scene)[i].v[v0].y) < 0) {
                    // move this vertex up by 1 to avoid handling this case when rasterizing.
                    e.x += e.dx;
                    ++y;
                }

                table[y].push_back(e);
            }
        }
    }
    ~SortedEdgeTable() = default;

private:
    void initTable() {
        // Get bounding of all triangles in scene.
        min = { std::numeric_limits<long>::max(), std::numeric_limits<long>::max() };
        max = { std::numeric_limits<long>::min(), std::numeric_limits<long>::min() };
        for (int i = 0; i < scene->size(); ++i) {
            min = int2::min(min, (*scene)[i].min());
            max = int2::max(max, (*scene)[i].max());
        }
        int table_size = max.y - min.y + 1;
        table.resize(table_size);
    }
};

struct ActiveEdgeList {
    std::list<Edge> list;
    SortedEdgeTable *SET;

    ActiveEdgeList(SortedEdgeTable *t)
        : SET(t) {}

    void fill(Image& image) {
        int y_min = SET->min.y;
        int y_max = SET->max.y;
        Edge *p;
        // Scan the bounding area of the polygon.
        for (int y = y_min; y <= y_max; ++y) {
            // Append new edges.
            for (int j = 0; j < SET->table[y - y_min].size(); ++j) {
                list.push_back(SET->table[y - y_min][j]);
            }
            // Sort edges by x.
            list.sort(compareEdge);
            assert(list.size() % 2 == 0);
            // Fill pixels between every pair of edges.
            auto e0 = list.begin();
            auto e1 = std::next(e0);
            while (e0 != list.end()) {
                int x = ftoi(e0->x);
                int x_max = ftoi(e1->x);
                while (x <= x_max) {
                    image.setPixel(x, y, (*(SET->scene))[e0->id].color);
                    ++x;
                }
                e0 = std::next(e1);
                e1 = std::next(e0);
            }

            // Update x in edges.
            for (auto iter = list.begin(); iter != list.end(); ++iter) {
                iter->x += iter->dx;
            }
            // Remove used edges.
            list.remove_if([=](Edge e){ return e.y_max == y; });
        }
    }

    static bool compareEdge(Edge const& first, Edge const& second) {
        if (first.id != second.id) {
            return first.id < second.id;
        } else {
            return first.x < second.x;
        }
    }
};

std::vector<float3> vertices = {
    {  0.0, 0.0, 0.0 },
    {  1.0, 0.0, 0.0 },
    {  1.0, 1.0, 0.0 },
    {  0.0, 0.0, 0.0 },
    {  1.0, 1.0, 0.0 },
    {  0.0, 1.0, 0.0 },
    {  0.0, 0.0, 0.0 },
    {  0.0, 1.0, 0.0 },
    { -1.0, 1.0, 0.0 },
    {  0.0, 0.0, 0.0 },
    { -1.0, 1.0, 0.0 },
    { -1.0, 0.0, 0.0 },
};

std::vector<float4> colors = {
    { 1.0, 0.0, 0.0, 1.0 },
    { 0.0, 1.0, 0.0, 1.0 },
    { 0.0, 0.0, 1.0, 1.0 },
    { 0.0, 1.0, 1.0, 1.0 },
};

// std::vector<Triangle> scene = {
//     { {{ 70, 30}, {30, 70}, {70, 110}}, {1.0, 0.0, 1.0, 1.0} },
//     { {{110, 10}, {60, 70}, {80,  90}}, {1.0, 1.0, 0.0, 1.0} },
// };

std::vector<float4> toClipSpace(std::vector<float3> const& vs, float4x4 const& mvp) {
    std::vector<float4> res;
    for (int i = 0; i < vs.size(); ++i) {
        float4 v = float4(vs[i], 1.0f);
        v = mvp * v;
        // Perspective divide.
        v.w = 1 / v.w;
        v.x *= v.w;
        v.y *= v.w;
        v.z *= v.w;
        v.w = 1;
        res.push_back(v);
    }
    return res;
}

inline int2 screenMapping(float4 const& v, float w, float h) {
    return int2{
        ftoi((v.x + 1.0f) * 0.5f * w),
        ftoi((v.y + 1.0f) * 0.5f * h),
    };
}

std::vector<Triangle> toScreenSpaceTriangles(std::vector<float4> const& vs, std::vector<float4> const& cs, float w, float h) {
    assert(vs.size() % 3 == 0);
    size_t const tri_num = vs.size() / 3;
    assert(cs.size() == tri_num);

    std::vector<Triangle> res;

    for (int i = 0; i < tri_num; ++i) {
        int offset = i * 3;
        Triangle t;

        // Screen mapping.
        t.v[0] = screenMapping(vs[offset + 0], w, h);
        t.v[1] = screenMapping(vs[offset + 1], w, h);
        t.v[2] = screenMapping(vs[offset + 2], w, h);

        float3 v0 = float3(vs[offset + 0]);
        float3 v1 = float3(vs[offset + 1]);
        float3 v2 = float3(vs[offset + 2]);

        float3 e01 = v1 - v0;
        float3 e02 = v2 - v0;
        float3 n = e01.cross(e02).normalized();

        // Surface of the triangle is defined by:
        // s_x * x + s_y * y + s_z * z = s_w
        t.surface.x = n.x;
        t.surface.y = n.y;
        t.surface.z = n.z;
        t.surface.w = n.dot(v0);

        t.color = cs[i];

        res.push_back(t);
    }

    return res;
}

void printMatrix(float4x4 const& m) {
    std::cout << m[0][0] << ", "
              << m[1][0] << ", "
              << m[2][0] << ", "
              << m[3][0] << "\n"
              << m[0][1] << ", "
              << m[1][1] << ", "
              << m[2][1] << ", "
              << m[3][1] << "\n"
              << m[0][2] << ", "
              << m[1][2] << ", "
              << m[2][2] << ", "
              << m[3][2] << "\n"
              << m[0][3] << ", "
              << m[1][3] << ", "
              << m[2][3] << ", "
              << m[3][3] << "\n";
}

int main() {

    printMatrix(projection());

    return 0;

    float w = 200;
    float h = 200;

    float4x4 model = float4x4::identity();
    float4x4 view = float4x4::identity();
    view = rotateY(PI() * 0.1) * view;
    // view = rotateX(PI() * 0.02) * view;
    float4x4 proj = projection();

    float4x4 mvp = proj * view * model;

    // float4x4 mvp = proj * model;

    auto vs = toClipSpace(vertices, mvp);
    for (int i = 0; i < vs.size(); ++i) {
        std::cout << vs[i][0] << ", " 
                  << vs[i][1] << ", " 
                  << vs[i][2] << ", " 
                  << vs[i][3] << "\n"; 
    }
    auto triangles = toScreenSpaceTriangles(vs, colors, w, h);

    for (int i = 0; i < triangles.size(); ++i) {
        std::cout << "Triangle[" << i << "]\n";
        for (int j = 0; j < 3; ++j) {
            std::cout << "v[" << j << "] = (" << triangles[i].v[j].x << ", " << triangles[i].v[j].y << ")\n";
        }
    }

    SortedEdgeTable SET(&triangles);
    ActiveEdgeList AEL(&SET);
    Image image(w, h);

    image.fill(colorf{0.0, 0.0, 0.0, 1.0});
    AEL.fill(image);
    image.writePNG("result.png");

    return 0;
}