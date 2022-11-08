#pragma once

#include <memory>
#include <string>
#include "utils.h"
#include "vector.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../thirdparty/stb_image_write.h"

struct Image {
    using dataType = unsigned char;
    dataType *data;
    int width, height;

    Image(int w, int h)
        : width(w)
        , height(h) {
        int size = w * h * 4;
        data = new dataType[size];
        std::memset(data, 0, size * sizeof(dataType));
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
        if (x < 0 || x >= width) return;
        if (y < 0 || y >= height) return;
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
