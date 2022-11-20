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
#include "../include/argparser.h"

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
 *  4. Hierarchical Z-Buffer acelerated by Object-Space Octree
 * -------------------------------------------------------
 * To compile:
        use command: 'make' in MacOS
        or 'mingw32-make' in Window
 * To Run:
        -- Z-Buffer Help Info ---------------------------
        Options:
        -i              Model to load, .obj format.
        -z              Z-Buffer algorithm, the following options available:
            simple      Simple Z-Buffer;
            scanline    Scanline Z-Buffer;
            hiez        Hierarchical Z-Buffer;
            octz        Hierarchical Z-Buffer with Octree Acceleration;
        -c              Model render count, the following options available:
            1 n         Render 1 * 1 * n models;
            3 n         Render 3 * 3 * n models;
            5 n         Render 5 * 5 * n models;
        -m              Model model, the following options available:
            r           Realtime mode;
            b n         Benchmark mode, render n frames and output timer result;
        -p              Projection model, the following options available:
            p           Perspective mode;
            o           Orthogonal mode;
 * Samples:
        ./viewer -i meshes/spot.obj
        ./viewer -i meshes/spot.obj -c 3 3
        ./viewer -i meshes/spot.obj -c 3 3 -z hiez
        ./viewer -i meshes/spot.obj -c 5 3 -z scanline -p o -m b 10
 */

Arguments args;

float camera_fov = PI() * 0.2;
float camera_z = -6.0f;
float rotate_x = 0.0f;
float rotate_y = 0.0f;

int main(int argc, char* argv[]) {
    if (!parse(argc, argv, &args)) {
        return 0;
    }

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
    TriangleMesh mesh{ args.model };
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

    int c = args.draw_count[0] / 2;
    int n = args.draw_count[1];
    auto proj = float4x4::identity();

    // Benchmark variables.
    int counter = 0;
    double total_elapsed = 0.0;

    SETUP_FPS();
    Timer t;
    while (!windowShouldClose(window)) {

/////////////////////////////////////////////////////////////////////////////////////////////
////////////// R E N D E R   L O O P
/////////////////////////////////////////////////////////////////////////////////////////////

        // Transform setup.
        auto aspect_ratio = (float)scr_h / scr_w;
        switch (args.proj_mode) {
        case ProjectionMode::Perspective:
            proj = perspective(camera_fov, (float)scr_h / scr_w); break;
        case ProjectionMode::Orthogonal:
            proj = ortho(-2, 2, -2 * aspect_ratio, 1 * aspect_ratio, -1000, 1000); break;
        }
        auto model = translate(0, 0, 0);
        auto view = lookAt(mesh.center + float3(0, 0, camera_z), mesh.center, float3(0, 1, 0));
        auto mvp = proj * view * model;
    
        // Clear framebuffer.
        image.fill(colorf{0, 0, 0, 1});

        // In benchmark mode, we only record the runtime of the algorithm.
        if (args.render_mode == RenderMode::Benchmark) t.update();

        switch (args.algorithm) {
        case ZBufferAlgorithm::SimpleZBuffer:
            simpleZBuffer.clearDepth();
            for (int x = -c; x <= c; ++x) for (int y = -c; y <= c; ++y) for (int z = -1; z <= n - 2; ++z) {
                model = rotateX(rotate_x) * rotateY(rotate_y) * translate(x, y, z);
                mvp = proj * view * model; 
                simpleZBuffer.drawMesh(mesh, colors, mvp, image);
            }
            break;
        case ZBufferAlgorithm::ScanlineZBuffer:
            for (int x = -c; x <= c; ++x) for (int y = -c; y <= c; ++y) for (int z = -1; z <= n - 2; ++z) {
                model = rotateX(rotate_x) * rotateY(rotate_y) * translate(x, y, z);
                mvp = proj * view * model; 
                scanlineZBuffer.drawMesh(mesh, colors, mvp, image);
            }
            break;
        case ZBufferAlgorithm::HierarchicalZBuffer:
            hierarchicalZBuffer.clearDepth();
            for (int x = -c; x <= c; ++x) for (int y = -c; y <= c; ++y) for (int z = -1; z <= n - 2; ++z) {
                model = rotateX(rotate_x) * rotateY(rotate_y) * translate(x, y, z);
                mvp = proj * view * model; 
                hierarchicalZBuffer.drawMesh(mesh, colors, mvp, image);
            }
            break;
        case ZBufferAlgorithm::OctreeZBuffer:
            octreeZBuffer.clearDepth();
            for (int x = -c; x <= c; ++x) for (int y = -c; y <= c; ++y) for (int z = -1; z <= n - 2; ++z) {
                model = rotateX(rotate_x) * rotateY(rotate_y) * translate(x, y, z);
                mvp = proj * view * model; 
                octreeZBuffer.drawMesh(mesh, colors, mvp, image);
            }
            break;
        case ZBufferAlgorithm::OctreeZBufferFixed:
            octreeZBuffer.clearDepth();
            octreeZBuffer.fixed = true;
            unsigned int id = 0;
            for (int x = -c; x <= c; ++x) for (int y = -c; y <= c; ++y) for (int z = -1; z <= n - 2; ++z) {
                model = rotateX(rotate_x) * rotateY(rotate_y) * translate(x, y, z);
                mvp = proj * view * model; 
                octreeZBuffer.drawMesh(mesh, colors, mvp, image, id);
                ++id;
            }
            break;
        }

        // Benchmark functionality, to record runtime of each render stage.
        // In Benchmark mode, program will automatically terminate at render_count frames
        // and output elapsed runtime per frame.
        if (args.render_mode == RenderMode::Benchmark) {
            t.update();
            total_elapsed += t.deltaTime();
            counter++;
            if (counter == args.render_count) {
                std::cout << "Elapsed time per frame: " << total_elapsed * 1000 / args.render_count << "ms\n";
                destroyWindow(window);
            }
        }

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
            break;
        default:
            return;
        }
    }
}
bool first_drag = false;
void mouseButtonEventCallback(AppWindow *window, MOUSE_BUTTON button, bool pressed) {
    __unused_variable(window);
    if (args.algorithm == ZBufferAlgorithm::OctreeZBufferFixed) return;
    if (button == BUTTON_L && pressed) {
        first_drag = true;
    }
}
void mouseScrollEventCallback(AppWindow *window, float offset) {
    __unused_variable(window);
    if (args.algorithm == ZBufferAlgorithm::OctreeZBufferFixed) return;
    camera_fov += offset * 0.01f;
    camera_fov = clamp(camera_fov, 0.02f, 0.5f);
}
float last_x;
float last_y;
void mouseDragEventCallback(AppWindow *window, float x, float y) {
    __unused_variable(window);
    if (args.algorithm == ZBufferAlgorithm::OctreeZBufferFixed) return;
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