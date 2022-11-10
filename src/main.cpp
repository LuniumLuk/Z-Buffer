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
#include "../include/scanline.h"

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