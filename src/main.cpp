#include <iostream>
#include "platform/platform.h"

using namespace LuGL;

static void keyboardEventCallback(AppWindow *window, KEY_CODE key, bool pressed);
static void mouseButtonEventCallback(AppWindow *window, MOUSE_BUTTON button, bool pressed);
static void mouseScrollEventCallback(AppWindow *window, float offset);
static void mouseDragEventCallback(AppWindow *window, float x, float y);

static AppWindow *window;
unsigned char buffer[512 * 512 * 3];

int main() {
    initializeApplication();

    const char * title = "Viewer @ Lu Renderer";
    window = createWindow(title, 512, 512, buffer);

    setKeyboardCallback(window, keyboardEventCallback);
    setMouseButtonCallback(window, mouseButtonEventCallback);
    setMouseScrollCallback(window, mouseScrollEventCallback);
    setMouseDragCallback(window, mouseDragEventCallback);

    while (!windowShouldClose(window)) {
        size_t buffer_size = 512 * 512;
        for (size_t i = 0; i < buffer_size; ++i) {
            size_t offset = i * 3;
            buffer[offset + 0] = 255;
            buffer[offset + 0] = 0;
            buffer[offset + 0] = 255;
        }

        swapBuffer(window);
        pollEvent();
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
                break;
            case KEY_SPACE:
                break;
            default:
                return;
        }
    }
}
void mouseButtonEventCallback(AppWindow *window, MOUSE_BUTTON button, bool pressed) {
    __unused_variable(window);
    __unused_variable(button);
    __unused_variable(pressed);
}
void mouseScrollEventCallback(AppWindow *window, float offset) {
    __unused_variable(window);
    __unused_variable(offset);
}
void mouseDragEventCallback(AppWindow *window, float x, float y) {
    __unused_variable(window);
    __unused_variable(x);
    __unused_variable(y);
}