#pragma once

#include <cassert>
#include <vector>

struct HierarchicalZBuffer {

    std::vector<float*> mip;
    int width;
    int height;
    std::vector<int> mip_w;
    std::vector<int> mip_h;

    HierarchicalZBuffer(int w, int h)
        : width(w)
        , height(h) {

        while (w > 0 && h > 0) {
            mip_w.push_back(w);
            mip_h.push_back(h);
            float* buffer = new float[w * h];
            mip.push_back(buffer);

            w /= 2;
            h /= 2;
        }
    }

    float sample(int x, int y, int level) const {
        assert(level >= 0 && level < mip.size());
        assert(x >= 0 && x < width);
        assert(y >= 0 && y < height);

        x /= (1 << level);
        y /= (1 << level);

        int offset = y * mip_w[level] + x;
        assert(offset < (mip_w[level] * mip_h[level]));

        return mip[level][offset];
    }

    void clear(float z) {
        for (int i = 0; i < mip.size(); ++i) {
            int size = mip_w[i] * mip_h[i];
            for (int j = 0; j < size; ++j) {
                mip[i][j] = z;
            }
        }
    }

    void write(int x, int y, float z) {
        assert(x >= 0 && x < width);
        assert(y >= 0 && y < height);

        int offset = y * width + x;
        for (int i = 0; i < mip.size(); ++i) {
            int offset = y * mip_w[i] + x;
            if (mip[i][offset] > z) {
                mip[i][offset] = z;
            }
            x /= 2;
            y /= 2;
        }
    }

};


struct ZBuffer {

    float* buffer;
    int width;
    int height;

    ZBuffer(int w, int h)
        : width(w)
        , height(h) {

        buffer = new float[width * height];
    }

    float depth(int x, int y) const {
        assert(x >= 0 && x < width);
        assert(y >= 0 && y < height);
        int offset = y * width + x;
        assert(offset < width * height);
        return buffer[offset];
    }

    void clear(float z) {
        int size = width * height;
        for (int i = 0; i < size; ++i) {
            buffer[i] = z;
        }
    }

    void write(int x, int y, float z) {
        assert(x >= 0 && x < width);
        assert(y >= 0 && y < height);
        int offset = y * width + x;
        assert(offset < width * height);
        buffer[offset] = z;
    }

};
