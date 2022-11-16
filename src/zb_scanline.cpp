#include "../include/zb_scanline.h"

using Edge = ZBScanline::SortedEdgeTable::Edge;
static bool compareEdge(Edge const& first, Edge const& second) {
    if (first.id != second.id) {
        return first.id < second.id;
    } else {
        return first.x < second.x;
    }
}

void ZBScanline::drawMesh(TriangleMesh const& mesh,
                          std::vector<colorf> const& colors,
                          float4x4 const& mvp,
                          Image & image) {

    std::vector<float3> ndc;
    for (int i = 0; i < mesh.vertices.size(); ++i) {
        auto v = float4(mesh.vertices[i], 1.0f);
        v = mvp * v;
        v.w = 1 / v.w;
        v.x *= v.w;
        v.y *= v.w;
        v.z *= v.w;
        ndc.push_back(float3(v));
    }

    std::vector<Triangle> triangles;

    // Setup triangles.
    for (size_t i = 0; i < mesh.indices.size(); ++i) {
        Triangle t;

        auto v0 = ndc[mesh.indices[i][0]];
        auto v1 = ndc[mesh.indices[i][1]];
        auto v2 = ndc[mesh.indices[i][2]];

        // Screen mapping.
        v0.x = (v0.x * 0.5f + 0.5f) * width;
        v1.x = (v1.x * 0.5f + 0.5f) * width;
        v2.x = (v2.x * 0.5f + 0.5f) * width;
        v0.y = (v0.y * 0.5f + 0.5f) * height;
        v1.y = (v1.y * 0.5f + 0.5f) * height;
        v2.y = (v2.y * 0.5f + 0.5f) * height;

        t.v[0] = int2(ftoi(v0.x), ftoi(v0.y));
        t.v[1] = int2(ftoi(v1.x), ftoi(v1.y));
        t.v[2] = int2(ftoi(v2.x), ftoi(v2.y));

        // Cull subpixel triangles.
        auto area = edgeFunction2D(v0, v1, v2);
        if (area == 0) continue;

        // Store z value.
        t.z[0] = v0.z;
        t.z[1] = v1.z;
        t.z[2] = v2.z;

        // Back-face culling.
        float3 e01 = v1 - v0;
        float3 e02 = v2 - v0;
        float3 n = e01.cross(e02);

        if (n.z < 0) continue;

        // Surface of the triangle is defined by:
        // s_x * x + s_y * y + s_z * z = s_w
        t.surface.x = n.x;
        t.surface.y = n.y;
        t.surface.z = n.z;
        t.surface.w = n.dot(v0);
        t.id = i;

        triangles.push_back(t);
    }

    SortedEdgeTable SET(triangles, width, height);

    int y_min = SET.min.y;
    int y_max = SET.max.y;
    std::list<SortedEdgeTable::Edge> AEL;
    std::vector<float> z_buffer(image.width);
    // Scan the bounding area of the polygon.
    for (int y = y_min; y <= y_max; ++y) {
        for (int x = 0; x < image.width; ++x) {
            z_buffer[x] = 1.0f;
        }

        // Append new edges.
        for (size_t j = 0; j < SET.table[y - y_min].size(); ++j) {
            SortedEdgeTable::Edge e = SET.table[y - y_min][j];
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
        AEL.remove_if([=](SortedEdgeTable::Edge e){ return e.y_max == y; });
    }
    AEL.clear();
}

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
            e.id = tris[i].id;
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
    min = { std::numeric_limits<int>::max(), std::numeric_limits<int>::max() };
    max = { std::numeric_limits<int>::min(), std::numeric_limits<int>::min() };
    for (size_t i = 0; i < tris.size(); ++i) {
        min = int2::min(min, tris[i].min());
        max = int2::max(max, tris[i].max());
    }
    int table_size = max.y - min.y + 1;
    table.resize(table_size);
}
