#include <iostream>
#include <vector>
#include <list>
#include <cmath>
#include <cassert>
#include <limits>
#include <algorithm>
#include <chrono>
#include <cmath>
#include "../include/timer.h"
#include "../include/platform.h"
#include "../include/vector.h"
#include "../include/matrix.h"
#include "../include/image.h"
#include "../include/utils.h"

using namespace LuGL;

static void keyboardEventCallback(AppWindow *window, KEY_CODE key, bool pressed);
static void mouseButtonEventCallback(AppWindow *window, MOUSE_BUTTON button, bool pressed);
static void mouseScrollEventCallback(AppWindow *window, float offset);
static void mouseDragEventCallback(AppWindow *window, float x, float y);

static AppWindow *window;
unsigned char buffer[512 * 512 * 3];

// This program is a demo of Scanline Z-buffer Algorithm
// -------------------------------------------------------
// to compile and run in vscode in MacOS:
// g++ -std=c++17 -Og scanline_zbuffer.cpp -o out && ./out

struct Triangle {
    int2 v[3];
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

    SortedEdgeTable(std::vector<Triangle> const& scene) {
        int const tri_num = scene.size();
        int const vn = 3; // Here we consider only triangles.
        initTable(scene);
        for (int i = 0; i < tri_num; ++i) {
            for (int j = 0; j < vn; ++j) {
                Edge e;
                int v0 = j;
                int v1 = (j + 1) % vn;
                if (scene[i].v[v0].y == scene[i].v[v1].y) continue;
                if (scene[i].v[v0].y > scene[i].v[v1].y) {
                    // Use the vertex with smaller y as v0.
                    std::swap(v0, v1);
                }
                e.y_max = scene[i].v[v1].y;
                e.dx = (float)(scene[i].v[v1].x - scene[i].v[v0].x) / (scene[i].v[v1].y - scene[i].v[v0].y);
                e.x = itof(scene[i].v[v0].x);
                e.id = i;

                // Determine whether v0 is at a singularity position that is
                // both edge connected to it is on the same side of the scanline.
                int prev = (v0 - 1 + vn) % vn;
                int next = (v0 + 1 + vn) % vn;
                int y = scene[i].v[v0].y - min.y;
                // If not at singularity position.
                if ((scene[i].v[next].y - scene[i].v[v0].y) * (scene[i].v[prev].y - scene[i].v[v0].y) < 0) {
                    // move this vertex up by 1 to avoid handling this case when rasterizing.
                    e.x += e.dx;
                    ++y;
                }
                
                assert(y < table.size());
                table[y].push_back(e);
            }
        }
    }
    ~SortedEdgeTable() = default;

private:
    void initTable(std::vector<Triangle> const& scene) {
        // Get bounding of all triangles in scene.
        min = { std::numeric_limits<long>::max(), std::numeric_limits<long>::max() };
        max = { std::numeric_limits<long>::min(), std::numeric_limits<long>::min() };
        for (size_t i = 0; i < scene.size(); ++i) {
            min = int2::min(min, scene[i].min());
            max = int2::max(max, scene[i].max());
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

    void fill(Image& image, std::vector<colorf> const& colors) {
        int y_min = SET->min.y;
        int y_max = SET->max.y;
        // Scan the bounding area of the polygon.
        for (int y = y_min; y <= y_max; ++y) {
            // Append new edges.
            for (size_t j = 0; j < SET->table[y - y_min].size(); ++j) {
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
                    image.setPixel(x, y, colors[e0->id]);
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

std::vector<float3> const g_vertices = {
    {  0.0,  0.0, 0.0 },
    {  1.0,  0.0, 0.0 },
    {  1.0,  1.0, 0.0 },
    {  0.0,  0.0, 0.0 },
    {  1.0,  1.0, 0.0 },
    {  0.0,  1.0, 0.0 },
    {  0.0,  0.0, 0.0 },
    {  0.0,  1.0, 0.0 },
    { -1.0,  1.0, 0.0 },
    {  0.0,  0.0, 0.0 },
    { -1.0,  1.0, 0.0 },
    { -1.0,  0.0, 0.0 },
    {  0.0,  0.0, 0.0 },
    { -1.0,  0.0, 0.0 },
    { -1.0, -1.0, 0.0 },
    {  0.0,  0.0, 0.0 },
    { -1.0, -1.0, 0.0 },
    {  0.0, -1.0, 0.0 },
    {  0.0,  0.0, 0.0 },
    {  0.0, -1.0, 0.0 },
    {  1.0, -1.0, 0.0 },
    {  0.0,  0.0, 0.0 },
    {  1.0, -1.0, 0.0 },
    {  1.0,  0.0, 0.0 },
};

std::vector<float4> const g_colors = {
    { 0.0, 1.0, 0.0, 1.0 },
    { 0.2, 0.8, 0.2, 1.0 },
    { 0.4, 0.6, 0.4, 1.0 },
    { 0.6, 0.4, 0.6, 1.0 },
    { 0.8, 0.2, 0.8, 1.0 },
    { 1.0, 0.0, 1.0, 1.0 },
    { 0.8, 0.2, 0.8, 1.0 },
    { 0.6, 0.4, 0.6, 1.0 },
};

std::vector<float4> toClipSpace(std::vector<float3> const& vs, float4x4 const& mvp) {
    std::vector<float4> res;
    for (size_t i = 0; i < vs.size(); ++i) {
        float4 v(vs[i], 1.0f);
        v = mvp * v;
        // Perspective divide.
        if (v.w == 0) v.w = 1;
        else v.w = 1 / v.w;
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

std::vector<Triangle> toScreenSpaceTriangles(std::vector<float4> const& vs, float w, float h) {
    assert(vs.size() % 3 == 0);
    size_t const tri_num = vs.size() / 3;

    std::vector<Triangle> res;

    for (size_t i = 0; i < tri_num; ++i) {
        size_t offset = i * 3;
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

        res.push_back(t);
    }

    return res;
}

long scr_w = 512;
long scr_h = 512;
float rotate_x = 0.0f;
float rotate_y = 0.0f;

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
    initializeApplication();

    const char * title = "Viewer @ LuGL";
    Image image(scr_w, scr_h);
    window = createWindow(title, scr_w, scr_h, image.data);

    setKeyboardCallback(window, keyboardEventCallback);
    setMouseButtonCallback(window, mouseButtonEventCallback);
    setMouseScrollCallback(window, mouseScrollEventCallback);
    setMouseDragCallback(window, mouseDragEventCallback);


    Timer t;
    while (!windowShouldClose(window)) {

        auto proj = projection();
        auto model = rotateX(rotate_x) * rotateY(rotate_y);
        auto view = translate(0, 0, -2);
        auto mvp = proj * view * model; 

        auto vs = toClipSpace(g_vertices, mvp);
        auto triangles = toScreenSpaceTriangles(vs, scr_w, scr_h);
        SortedEdgeTable SET(triangles);
        ActiveEdgeList AEL(&SET);

        image.fill(colorf{0.0, 0.0, 0.0, 1.0});
        AEL.fill(image, g_colors);

        swapBuffer(window);
        pollEvent();
        t.update();
    }

    terminateApplication();
    return 0;
}

void keyboardEventCallback(AppWindow *window, KEY_CODE key, bool pressed) {
    __unused_variable(window);
    if (pressed)
    {
        switch (key)
        {
            case KEY_A:
                break;
            case KEY_S:
                break;
            case KEY_D:
                break;
            case KEY_W:
                break;
            case KEY_ESCAPE:
                destroyWindow(window);
                break;
            case KEY_SPACE:
                rotate_x = 0.0f;
                rotate_y = 0.0f;
                break;
            default:
                return;
        }
    }
}
bool first_drag = false;
void mouseButtonEventCallback(AppWindow *window, MOUSE_BUTTON button, bool pressed) {
    __unused_variable(window);
    __unused_variable(button);
    __unused_variable(pressed);
    if (button == BUTTON_L && pressed) {
        first_drag = true;
    }
}
void mouseScrollEventCallback(AppWindow *window, float offset) {
    __unused_variable(window);
    __unused_variable(offset);
}
float last_x;
float last_y;
void mouseDragEventCallback(AppWindow *window, float x, float y) {
    __unused_variable(window);
    if (first_drag) {
        last_x = x;
        last_y = y;
        first_drag = false;
    }
    else {
        float delta_x = x - last_x;
        float delta_y = y - last_y;

        rotate_y += delta_x * 0.01f;
        rotate_x += delta_y * 0.01f;

        last_x = x;
        last_y = y;
    }
}