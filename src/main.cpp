#include <iostream>
#include <vector>
#include <limits>
#include <cmath>
#include <chrono>
#include "../include/timer.h"
#include "../include/platform.h"
#include "../include/vector.h"
#include "../include/matrix.h"
#include "../include/image.h"
#include "../include/utils.h"
#include "../include/mesh.h"
#include "../include/transform.h"
#include "../include/zb_simple.h"
#include "../include/zb_scanline.h"
#include "../include/zb_hierarchical.h"

using namespace LuGL;

// #define SIMPLE
#define SCANLINE
// #define HIERARCHICAL

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

long scr_w = 512;
long scr_h = 512;
float camera_fov = 0.1f;
float camera_z = -3.0f;
float rotate_x = 0.0f;
float rotate_y = 0.0f;
ZBHierarchical* zb;

int main() {
    initializeApplication();

    const char * title = "Viewer @ LuGL";
    Image image(scr_w, scr_h);
    window = createWindow(title, scr_w, scr_h, image.data);

    setKeyboardCallback(window, keyboardEventCallback);
    setMouseButtonCallback(window, mouseButtonEventCallback);
    setMouseScrollCallback(window, mouseScrollEventCallback);
    setMouseDragCallback(window, mouseDragEventCallback);

    float3 light_dir = float3(1.0, 1.0, -1.0).normalized();
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

    ZBSimple simpleZBuffer(scr_w, scr_h);
    ZBScanline scanlineZBuffer(scr_w, scr_h);
    ZBHierarchical hierarchicalZBuffer(scr_w, scr_h);

    float fixed_delta = 0.16f;
    float from_last_fixed = 0.0f;
    int frame_since_last_fixed = 0;
    Timer t;
    while (!windowShouldClose(window)) {

        auto proj = projection(camera_fov, 0.1, 10);
        auto model = translate(0, 0, 0);
        auto view = lookAt(float3(0, 0, camera_z), float3(0, 0, 0), float3(0, 1, 0));
        auto mvp = proj * view * model;

        image.fill(colorf{0.0, 0.0, 0.0, 1.0});
#if defined(SIMPLE)
        simpleZBuffer.clearDepth();
        for (int x = -1; x <= 1; ++x) for (int y = -1; y <= 1; ++y) for (int z = 1; z >= -1; --z) {
            model = rotateX(rotate_x) * rotateY(rotate_y) * translate(x, y, z);
            mvp = proj * view * model; 
            simpleZBuffer.drawMesh(mesh.vertices, mesh.indices, colors, mvp, image);
        }
#elif defined(SCANLINE)
        model = rotateX(rotate_x) * rotateY(rotate_y);
        mvp = proj * view * model; 
        scanlineZBuffer.drawMesh(mesh.vertices, mesh.indices, colors, mvp, image);
#elif defined(HIERARCHICAL) 
        hierarchicalZBuffer.clearDepth();
        for (int x = -1; x <= 1; ++x) for (int y = -1; y <= 1; ++y) for (int z = 1; z >= -1; --z) {
            model = rotateX(rotate_x) * rotateY(rotate_y) * translate(x, y, z);
            mvp = proj * view * model; 
            hierarchicalZBuffer.drawMesh(mesh.vertices, mesh.indices, colors, mvp, image);
        }
#endif
        // scanlineZBuffer.drawMesh(mesh.vertices, mesh.indices, colors, mvp, image);

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
        rotate_x -= delta_y * 0.01f;

        last_x = x;
        last_y = y;
    }
}