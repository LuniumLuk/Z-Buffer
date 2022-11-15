#pragma once

#include <cassert>
#include <vector>
#include <iostream>
#include "vector.h"

struct HierarchicalZBuffer {

    std::vector<float*> mip;
    std::vector<std::vector<int2>> mip_childern;
    int width;
    int height;
    std::vector<int> mip_w;
    std::vector<int> mip_h;

    int maxLevel() const { return mip.size() - 1; }

    HierarchicalZBuffer(int w, int h)
        : width(w)
        , height(h) {
        
        int level = 0;
        while (w > 0 && h > 0) {
            mip_w.push_back(w);
            mip_h.push_back(h);
            float* buffer = new float[w * h];
            mip.push_back(buffer);

            std::vector<int2> childern(w * h);
            if (level > 0) {
                for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
                    auto offset = (y * 2) * mip_w[level - 1] + (x * 2);
                    auto c = int2(offset, offset + mip_w[level - 1]);
                    offset = y * mip_w[level] + x;
                    childern[offset] = c;
                }
            }
            mip_childern.push_back(childern);

            w /= 2;
            h /= 2;
            ++level;
        }
    }
    ~HierarchicalZBuffer() {
        for (int i = 0; i < mip.size(); ++i) {
            delete[] mip[i];
        }
    }

    float at(int x, int y, int level) const {
        assert(level >= 0 && level < mip.size());
        assert(x >= 0 && x < width);
        assert(y >= 0 && y < height);

        x /= (1 << level);
        y /= (1 << level);

        int offset = y * mip_w[level] + x;

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

    void writeBaseLevel(int x, int y, float z) {
        assert(x >= 0 && x < width);
        assert(y >= 0 && y < height);

        int offset = y * mip_w[0] + x;
        mip[0][offset] = z;
    }

    void write(int x, int y, float z) {
        assert(x >= 0 && x < width);
        assert(y >= 0 && y < height);

        int offset = y * mip_w[0] + x;
        mip[0][offset] = z;

        for (int i = 1; i < mip.size(); ++i) {
            x /= 2;
            y /= 2;

            offset = y * mip_w[i] + x;
            if (z > mip[i][offset]) {
                mip[i][offset] = z;
                continue;
            }
            auto prev = i - 1;
            auto max_z = std::max(std::max(mip[prev][mip_childern[i][offset][0] + 0],
                                           mip[prev][mip_childern[i][offset][0] + 1]),
                                  std::max(mip[prev][mip_childern[i][offset][1] + 0],
                                           mip[prev][mip_childern[i][offset][1] + 1]));
            if (max_z == mip[i][offset]) break;
            mip[i][offset] = max_z;
        }
    }

    // Too slow, even per mesh is unacceptable.
    void update() {
        for (int i = 1; i < mip.size(); ++i) {
            for (int x = 0; x < mip_w[i]; ++x) for (int y = 0; y < mip_h[i]; ++y) {
                auto prev = i - 1;
                auto offset = y * mip_w[i] + x;
                auto max_z = std::max(std::max(mip[prev][mip_childern[i][offset][0] + 0],
                                               mip[prev][mip_childern[i][offset][0] + 1]),
                                      std::max(mip[prev][mip_childern[i][offset][1] + 0],
                                               mip[prev][mip_childern[i][offset][1] + 1]));

                mip[i][offset] = max_z;
            }
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
    ~ZBuffer() {
        delete[] buffer;
    }

    float at(int x, int y) const {
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
