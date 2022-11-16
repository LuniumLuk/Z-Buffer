#include "../include/image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../thirdparty/stb_image_write.h"
#include "../include/utils.h"
#include <memory>

Image::Image(int w, int h)
    : width(w)
    , height(h) {
    int size = w * h * channel();
    data = new dataType[size];
    memset(data, 0, size * sizeof(dataType));
}
Image::~Image() {
    delete data;
}

void Image::fill(colorf const& color) {
    int size = width * height;
    for (int i = 0; i < size; ++i) {
        int offset = i * channel();
        data[offset + 0] = clamp(color.r, 0.0f, 1.0f) * 255;
        data[offset + 1] = clamp(color.g, 0.0f, 1.0f) * 255;
        data[offset + 2] = clamp(color.b, 0.0f, 1.0f) * 255;
    }
}

void Image::setPixel(int x, int y, colorf const& color) {
    if (x < 0 || x >= width) return;
    if (y < 0 || y >= height) return;
    int offset = (x + (height - 1 - y) * width) * channel();
    data[offset + 0] = clamp(color.r, 0.0f, 1.0f) * 255;
    data[offset + 1] = clamp(color.g, 0.0f, 1.0f) * 255;
    data[offset + 2] = clamp(color.b, 0.0f, 1.0f) * 255;
}

void Image::writePNG(std::string const& path) {
    int stride = width * channel();
    stride += (stride % 4) ? (4 - stride % 4) : 0;
    stbi_flip_vertically_on_write(false);
    stbi_write_png(path.c_str(), width, height, channel(), data, stride);
}

void writeDepthToPNG(std::string const& path, int width, int height, float* depth) {
    int stride = width * 1;
    stride += (stride % 4) ? (4 - stride % 4) : 0;
    stbi_flip_vertically_on_write(true);
    unsigned char* data = new unsigned char[width * height];
    for (int j = 0; j < width * height; ++j) {
        data[j] = (unsigned char)(depth[j] * 255);
    }
    stbi_write_png(path.c_str(), width, height, 1, data, stride);
}

void Image::drawLine(int2 const& v0, int2 const& v1, colorf const& color) {

    auto x0 = clamp(v0.x, 0, width - 1);
    auto x1 = clamp(v1.x, 0, width - 1);
    auto y0 = clamp(v0.y, 0, height - 1);
    auto y1 = clamp(v1.y, 0, height - 1);

    int dx = abs(x1 - x0);
    int dy = -abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy; 
    int e2;

    while (true) {
        setPixel(x0, y0, color);

        if (x0 == x1 && y0 == y1) break;

        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}
