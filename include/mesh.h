#pragma once

#include <vector>
#include <string>
#include <iostream>
#include "vector.h"

#define MAX_LINE_NUM 256

struct TriangleMesh {

    std::vector<float3> vertices;
    std::vector<int3> indices;

    TriangleMesh() = delete;
    TriangleMesh(std::string const& path) {
        FILE *fp;
        fp = fopen(path.c_str(), "rb");
        if (fp == nullptr) {
            std::cout << "Failed to open file: " << path << std::endl;
            return;
        }

        char line_buffer[MAX_LINE_NUM];

        float x, y, z;
        long v1, v2, v3, vn1, vn2, vn3, vt1, vt2, vt3;
        long num;
        while (fgets(line_buffer, MAX_LINE_NUM, fp)) {
            // Processing vertex positions.
            if (strncmp(line_buffer, "v ", 2) == 0) {
                num = sscanf(line_buffer, "v %f %f %f", &x, &y, &z);
                if (num == 3) {
                    vertices.emplace_back(x, y, z);
                }
            }
            // Processing face indices.
            else if (strncmp(line_buffer, "f ", 2) == 0) {
                // Face with position/texcoord/normal indices.
                num = sscanf(line_buffer, "f %lu/%lu/%lu %lu/%lu/%lu %lu/%lu/%lu", &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3);
                if (num == 9) {
                    indices.emplace_back(v1-1, v2-1, v3-1);
                    continue;
                }
                // Face with position//normal indices.
                num = sscanf(line_buffer, "f %lu//%lu %lu//%lu %lu//%lu", &v1, &vn1, &v2, &vn2, &v3, &vn3);
                if (num == 6) {
                    indices.emplace_back(v1-1, v2-1, v3-1);
                    continue;
                }
                // Face with position/texcoord indices.
                num = sscanf(line_buffer, "f %lu/%lu %lu/%lu %lu/%lu", &v1, &vt1, &v2, &vt2, &v3, &vt3);
                if (num == 6) {
                    indices.emplace_back(v1-1, v2-1, v3-1);
                    continue;
                }
                // Face with only position indices.
                num = sscanf(line_buffer, "f %lu %lu %lu", &v1, &v2, &v3);
                if (num == 3) {
                    indices.emplace_back(v1-1, v2-1, v3-1);
                    continue;
                }
            }
        }
        fclose(fp);
    }

    float3 center() const {
        float3 c;
        for (size_t i = 0; i < vertices.size(); ++i) {
            c.x += vertices[i].x;
            c.y += vertices[i].y;
            c.z += vertices[i].z;
        }
        c.x /= vertices.size();
        c.y /= vertices.size();
        c.z /= vertices.size();
        return c;
    }

    float3 max() const {
        float3 max{ std::numeric_limits<float>::min() };
        for (size_t i = 0; i < vertices.size(); ++i) {
            max = float3::max(max, vertices[i]);
        }
        return max;
    }

    float3 min() const {
        float3 min{ std::numeric_limits<float>::max() };
        for (size_t i = 0; i < vertices.size(); ++i) {
            min = float3::min(min, vertices[i]);
        }
        return min;
    }
};