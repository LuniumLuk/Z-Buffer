#include <iostream>
#include <iomanip>
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
#include "../include/mesh.h"

using namespace LuGL;

#define rnd() (static_cast<float>(rand())/static_cast<float>(RAND_MAX))

static void keyboardEventCallback(AppWindow *window, KEY_CODE key, bool pressed);
static void mouseButtonEventCallback(AppWindow *window, MOUSE_BUTTON button, bool pressed);
static void mouseScrollEventCallback(AppWindow *window, float offset);
static void mouseDragEventCallback(AppWindow *window, float x, float y);

static AppWindow *window;
unsigned char buffer[512 * 512 * 3];

// This program is a demo of Scanline Z-buffer Algorithm
// -------------------------------------------------------
// to compile and run using make:
// >>> make run

struct Triangle {
    int2 v[3];
    float z[3];
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
    float z;     // current z value on edge
    float dzdx;  // derivation of z w.r.t. x on the surface
    float dzdy;  // derivation of z w.r.t. y on the surface
};

struct SortedEdgeTable {
    int2 min;
    int2 max;
    std::vector<std::vector<Edge>> table;

    SortedEdgeTable(std::vector<Triangle> const& tris, int w, int h) {
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

private:
    void initTable(std::vector<Triangle> const& tris) {
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
};

bool compareEdge(Edge const& first, Edge const& second) {
    if (first.id != second.id) {
        return first.id < second.id;
    } else {
        return first.x < second.x;
    }
}

void scanlineFill(SortedEdgeTable const& SET, Image& image, std::vector<colorf> const& colors) {
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
                    if (z >= 0 && z <= 1 && x >= 0 && x < image.width) {
                        if (z < z_buffer[x]) {
                            z_buffer[x] = z;
                            // colorf depth{ z, z, z, 1.0f };
                            image.setPixel(x, y, colors[e0->id]);
                            // image.setPixel(x, y, depth);
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

        // Store z value.
        t.z[0] = vs[offset + 0].z;
        t.z[1] = vs[offset + 1].z;
        t.z[2] = vs[offset + 2].z;

        float3 v0 = float3(vs[offset + 0]);
        float3 v1 = float3(vs[offset + 1]);
        float3 v2 = float3(vs[offset + 2]);

        float3 e01 = v1 - v0;
        float3 e02 = v2 - v0;
        float3 n = e01.cross(e02);

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
float camera_fov = 0.1f;
float camera_z = -3.0f;
float rotate_x = 0.0f;
float rotate_y = 0.0f;

void printMatrix4(float4x4 const& m) {
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

    float3 light_dir = float3(1.0, 1.0, 1.0).normalized();
    TriangleMesh mesh{ "meshes/spot.obj" };
    std::vector<float3> triangles;
    std::vector<colorf> colors;
    // Shade each triangle.
    for (size_t i = 0; i < mesh.indices.size(); ++i) {
        auto v0 = mesh.vertices[mesh.indices[i][0]];
        auto v1 = mesh.vertices[mesh.indices[i][1]];
        auto v2 = mesh.vertices[mesh.indices[i][2]];
        triangles.push_back(v0);
        triangles.push_back(v1);
        triangles.push_back(v2);

        auto e01 = v1 - v0;
        auto e02 = v2 - v0;
        auto normal = e01.cross(e02).normalized();
        // Shade by simple diffuse.
        auto shading = light_dir.dot(normal);
        shading = clamp(shading, 0.05f, 1.0f);

        colors.emplace_back(float3(shading), 1.0f);
    }

    float fixed_delta = 0.16f;
    float from_last_fixed = 0.0f;
    int frame_since_last_fixed = 0;
    Timer t;
    while (!windowShouldClose(window)) {

        auto proj = projection(camera_fov, 0.1, 10);
        auto model = rotateX(rotate_x) * rotateY(rotate_y);
        auto view = translate(0, 0, camera_z);
        auto mvp = proj * view * model; 

        auto vs = toClipSpace(triangles, mvp);
        auto clip = toScreenSpaceTriangles(vs, scr_w, scr_h);

        SortedEdgeTable SET(clip, scr_w, scr_h);

        image.fill(colorf{0.0, 0.0, 0.0, 1.0});
        scanlineFill(SET, image, colors);

        // Record time and FPS.
        t.update();
        from_last_fixed += t.deltaTime();
        ++frame_since_last_fixed;
        if (from_last_fixed > fixed_delta) {
            int fps = std::round(frame_since_last_fixed / from_last_fixed);
            std::string title = "Viewer @ LuGL FPS: " + std::to_string(fps);
            setWindowTitle(window, title.c_str());
            from_last_fixed = 0.0f;
            frame_since_last_fixed = 0;
        }

        swapBuffer(window);
        pollEvent();
    }

    terminateApplication();
    return 0;
}

void keyboardEventCallback(AppWindow *window, KEY_CODE key, bool pressed) {
    __unused_variable(window);
    if (pressed) {
        switch (key) {
        case KEY_A:
            break;
        case KEY_S:
            camera_z -= 0.02f;
            camera_z = clamp(camera_z, -4.0f, -0.5f);
            break;
        case KEY_D:
            break;
        case KEY_W:
            camera_z += 0.02f;
            camera_z = clamp(camera_z, -4.0f, -0.5f);
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
    if (button == BUTTON_L && pressed) {
        first_drag = true;
    }
}
void mouseScrollEventCallback(AppWindow *window, float offset) {
    __unused_variable(window);
    camera_fov += offset * 0.01f;
    camera_fov = clamp(camera_fov, 0.02f, 0.5f);
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