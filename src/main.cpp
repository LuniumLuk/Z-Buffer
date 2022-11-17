#include <iostream>
#include <vector>
#include <cmath>
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
#include "../include/zb_octree.h"

// A simple window API that support frame buffer swapping.
// I transported this window API from my software renderer project:
// https://github.com/LuniumLuk/soft-renderer
using namespace LuGL;

// Generate random float from 0 to 1.
#define rnd() (static_cast<float>(rand())/static_cast<float>(RAND_MAX))

static void keyboardEventCallback(AppWindow *window, KEY_CODE key, bool pressed);
static void mouseButtonEventCallback(AppWindow *window, MOUSE_BUTTON button, bool pressed);
static void mouseScrollEventCallback(AppWindow *window, float offset);
static void mouseDragEventCallback(AppWindow *window, float x, float y);

int const scr_w = 512;
int const scr_h = 512;

static AppWindow *window;

/**
 * This program is a demo of various Z-buffer algorithms.
 *  1. Simple Z-Buffer
 *  2. Scanline Z-Buffer (only able to render single mesh depth-correctly)
 *  3. Hierarchical Z-Buffer
 *  4. Hierarchical Z-Buffer accelerated by Object-Space Octree
 * -------------------------------------------------------
 * To compile and run using command: 'make run'
 * or 'mingw32-make run' in window
 */


float camera_fov = PI() * 0.2;
float camera_z = -6.0f;
float rotate_x = 0.0f;
float rotate_y = 0.0f;
bool scene_updated = false;
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

/////////////////////////////////////////////////////////////////////////////////////////////
////////////// S C E N E   S E T U P
/////////////////////////////////////////////////////////////////////////////////////////////

    float3 light_dir = float3(1.0, 1.0, -1.0).normalized();
    TriangleMesh mesh{ "meshes/armadillo.obj" };
    std::vector<colorf> colors;
    // Shade per triangle.
    for (size_t i = 0; i < mesh.indices.size(); ++i) {
        auto v0 = mesh.vertices[mesh.indices[i][0]];
        auto v1 = mesh.vertices[mesh.indices[i][1]];
        auto v2 = mesh.vertices[mesh.indices[i][2]];

        auto e01 = v1 - v0;
        auto e02 = v2 - v0;
        auto normal = e01.cross(e02).normalized();
        // Shade by diffuse lighting.
        auto shading = light_dir.dot(normal);
        shading = clamp(shading, 0.05f, 1.0f);

        colors.emplace_back(float3(shading), 1.0f);
    }

    std::cout << "Triangles: " << mesh.indices.size() << std::endl;

    ZBSimple simpleZBuffer(scr_w, scr_h);
    ZBScanline scanlineZBuffer(scr_w, scr_h);
    ZBHierarchical hierarchicalZBuffer(scr_w, scr_h);
    ZBOctree octreeZBuffer(scr_w, scr_h);
    octreeZBuffer.refresh = false;

#if defined(BENCHMARK)
    int counter = 0;
    double total_elapsed = 0.0;
#endif

    SETUP_FPS();
    Timer t;
    while (!windowShouldClose(window)) {

/////////////////////////////////////////////////////////////////////////////////////////////
////////////// R E N D E R   L O O P
/////////////////////////////////////////////////////////////////////////////////////////////

        auto aspect_ratio = (float)scr_h / scr_w;
        // Perspective projection.
        auto proj = perspective(camera_fov, (float)scr_h / scr_w);
        // Orthogonal projection.
        // auto proj = ortho(-2, 2, -2 * aspect_ratio, 1 * aspect_ratio, -1000, 1000);
        auto model = translate(0, 0, 0);
        auto view = lookAt(mesh.center + float3(0, 0, camera_z), mesh.center, float3(0, 1, 0));
        auto mvp = proj * view * model;
    
// #define SIMPLE
// #define SCANLINE
// #define HIERARCHICAL
#define OCTREE

        image.fill(colorf{0.1, 0.2, 0.3, 1.0});
        t.update();
#if defined(SIMPLE)
        simpleZBuffer.clearDepth();
        model = rotateX(rotate_x) * rotateY(rotate_y);
        mvp = proj * view * model; 
        simpleZBuffer.drawMesh(mesh, colors, mvp, image);
        // for (int x = -1; x <= 1; ++x) for (int y = -1; y <= 1; ++y) for (int z = -1; z <= 1; ++z) {
        //     model = rotateX(rotate_x) * rotateY(rotate_y) * translate(x, y, z);
        //     mvp = proj * view * model; 
        //     simpleZBuffer.drawMesh(mesh, colors, mvp, image);
        // }
#elif defined(SCANLINE)
        model = rotateX(rotate_x) * rotateY(rotate_y);
        mvp = proj * view * model; 
        scanlineZBuffer.drawMesh(mesh, colors, mvp, image);
#elif defined(HIERARCHICAL) 
        hierarchicalZBuffer.clearDepth();
        model = rotateX(rotate_x) * rotateY(rotate_y);
        mvp = proj * view * model; 
        hierarchicalZBuffer.drawMesh(mesh, colors, mvp, image);
        // for (int x = -1; x <= 1; ++x) for (int y = -1; y <= 1; ++y) for (int z = -1; z <= 78; ++z) {
        //     model = rotateX(rotate_x) * rotateY(rotate_y) * translate(x, y, z);
        //     mvp = proj * view * model; 
        //     hierarchicalZBuffer.drawMesh(mesh, colors, mvp, image);
        // }
#elif defined(OCTREE) 
        if (scene_updated) {
            octreeZBuffer.clearOctree();
            scene_updated = false;
        }
        octreeZBuffer.clearDepth();
        model = rotateX(rotate_x) * rotateY(rotate_y);
        mvp = proj * view * model;
        octreeZBuffer.drawMesh(mesh, colors, mvp, image);
        // for (int x = -1; x <= 1; ++x) for (int y = -1; y <= 1; ++y) for (int z = -1; z <= 8; ++z) {
        //     model = rotateX(rotate_x) * rotateY(rotate_y) * translate(x, y, z);
        //     mvp = proj * view * model; 
        //     octreeZBuffer.drawMesh(mesh, colors, mvp, image);
        // }
#endif

/**
 * Benchmark functionality, to see runtime of each render stage
 * enable by using 'make EXTRA=-BENCHMARK' option
 * or handcode '#define BENCHMARK' in the top of this file
 */
#if defined(BENCHMARK)
        t.update();
        total_elapsed += t.deltaTime();
        counter++;
        const long iter_times = 100;
        if (counter == iter_times) {
            std::cout << "Total Render Pass Average: " << g_render_time / g_counter * 1000 << "ms\n";
            std::cout << "Build Octree Pass Average: " << g_octree_time / g_counter * 1000 << "ms\n";
            std::cout << "Total Octree Visited: " << g_total_octree / iter_times << "\n";
            std::cout << total_elapsed * 1000 / iter_times << "ms\n";
            destroyWindow(window);
        }
#endif

        UPDATE_FPS();
        swapBuffer(window);
        pollEvent();
    }

    terminateApplication();
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
////////////// I N P U T   C A L L B A C K S
/////////////////////////////////////////////////////////////////////////////////////////////

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
            scene_updated = true;
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
        scene_updated = true;
    }
}