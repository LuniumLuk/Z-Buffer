#include <iostream>
#include <vector>
#include <list>
#include <cmath>
#include <limits>
#include <algorithm>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "thirdparty/stb_image_write.h"
#include "src/vector.h"
#include "src/utils.h"

// This program is a demo of Polygon Scanline Rasterization Algorithm
// using Sorted Edge Table and Active Edge List structure
// this algorithm runs by two steps:
//  1. generate sorted edge table
//  2. fill the polygon using active edge list
// -------------------------------------------------------
// to compile and run in vscode in MacOS:
// g++ -std=c++17 -Og polygon_scanline.cpp -o out && ./out

struct Edge {
    long y_max;
    float x;
    float dx;
};

struct SortedEdgeTable {
    int2 min;
    int2 max;
    std::vector<std::vector<Edge>> table;

    SortedEdgeTable(std::vector<int2> const& p) {
        int const vn = p.size();
        initTable(p);
        for (int i = 0; i < vn; ++i) {
            Edge e;
            int v0 = i;
            int v1 = (i + 1) % vn;
            if (p[v0].y == p[v1].y) continue;
            if (p[v0].y > p[v1].y) {
                // Use the vertex with smaller y as v0.
                std::swap(v0, v1);
            }
            e.y_max = p[v1].y;
            e.dx = (float)(p[v1].x - p[v0].x) / (p[v1].y - p[v0].y);
            e.x = itof(p[v0].x);

            // Determine whether v0 is at a singularity position that is
            // both edge connected to it is on the same side of the scanline.
            int prev = (v0 - 1 + vn) % vn;
            int next = (v0 + 1 + vn) % vn;
            int y = p[v0].y - min.y;
            // If not at singularity position.
            if ((p[next].y - p[v0].y) * (p[prev].y - p[v0].y) < 0) {
                // move this vertex up by 1 to avoid handling this case when rasterizing.
                e.x += e.dx;
                ++y;
            }

            table[y].push_back(e);
        }
    }
    ~SortedEdgeTable() = default;

private:
    void initTable(std::vector<int2> const& p) {
        // Get bounding of polygon.
        long x_min = std::numeric_limits<long>::max();
        long x_max = std::numeric_limits<long>::min();
        long y_min = std::numeric_limits<long>::max();
        long y_max = std::numeric_limits<long>::min();
        for (int i = 0; i < p.size(); ++i) {
            x_min = std::min(x_min, p[i].x);
            x_max = std::max(x_max, p[i].x);
            y_min = std::min(y_min, p[i].y);
            y_max = std::max(y_max, p[i].y);
        }
        if (y_max - y_min == 0) {
            std::cout << "Warning :: Invalid polygon shape." << std::endl;
        }
        int table_size = y_max - y_min + 1;
        table.resize(table_size);
        min.x = x_min;
        max.x = x_max;
        min.y = y_min;
        max.y = y_max;
    }
};

struct Image {
    using dataType = unsigned char;
    dataType *data;
    int width, height;

    Image(int w, int h)
        : width(w)
        , height(h) {
        int size = w * h * 4;
        data = new dataType[size];
        memset(data, 0, size * sizeof(dataType));
    }
    ~Image() {
        delete data;
    }

    void fill(colorf const& color) {
        int size = width * height;
        for (int i = 0; i < size; ++i) {
            int offset = i * 4;
            data[offset + 0] = clamp(color.r, 0, 1) * 255;
            data[offset + 1] = clamp(color.g, 0, 1) * 255;
            data[offset + 2] = clamp(color.b, 0, 1) * 255;
            data[offset + 3] = clamp(color.a, 0, 1) * 255;
        }
    }

    void setPixel(int x, int y, colorf const& color) {
        int offset = (x + y * width) * 4;
        data[offset + 0] = clamp(color.r, 0, 1) * 255;
        data[offset + 1] = clamp(color.g, 0, 1) * 255;
        data[offset + 2] = clamp(color.b, 0, 1) * 255;
        data[offset + 3] = clamp(color.a, 0, 1) * 255;
    }

    void writePNG(std::string const& path) {
        int stride = width * 4;
        stride += (stride % 4) ? (4 - stride % 4) : 0;
        int buffer_size = stride * height;
        stbi_flip_vertically_on_write(true);
        stbi_write_png(path.c_str(), width, height, 4, data, stride);
    }
};

struct ActiveEdgeList {
    std::list<Edge> list;
    SortedEdgeTable *SET;

    ActiveEdgeList(SortedEdgeTable *t)
        : SET(t) {}

    void fill(Image& image, colorf const& color) {
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
                    image.setPixel(x, y, color);
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
        return first.x < second.x;
    }
};

std::vector<int2> polygon = {
    {  30,  40 },
    {  20, 100 },
    {  50,  70 },
    { 120, 110 },
    { 150,  50 },
    { 120,  20 },
    {  70,  20 },
};

int main() {
    SortedEdgeTable SET(polygon);
    ActiveEdgeList AEL(&SET);
    Image image(200, 200);

    image.fill(colorf{0.0, 0.0, 0.0, 1.0});
    AEL.fill(image, colorf{1.0, 1.0, 1.0, 1.0});
    image.writePNG("result.png");

    return 0;
}