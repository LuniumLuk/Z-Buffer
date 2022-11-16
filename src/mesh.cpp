#include "../include/mesh.h"
#include <iostream>

TriangleMesh::TriangleMesh(std::string const& path)
    : center(0)
    , min(std::numeric_limits<float>::max())
    , max(std::numeric_limits<float>::min()) {
    FILE *fp;
    fp = fopen(path.c_str(), "rb");
    if (fp == nullptr) {
        std::cout << "Failed to open file: " << path << std::endl;
        return;
    }

    char line_buffer[MAX_LINE_NUM];

    float x, y, z;
    int v1, v2, v3, vn1, vn2, vn3, vt1, vt2, vt3;
    int num;
    while (fgets(line_buffer, MAX_LINE_NUM, fp)) {
        // Processing vertex positions.
        if (strncmp(line_buffer, "v ", 2) == 0) {
            num = sscanf(line_buffer, "v %f %f %f", &x, &y, &z);
            if (num == 3) {
                auto v = float3(x, y, z);
                min = float3::min(min, v);
                max = float3::max(max, v);
                center = center + v;
                vertices.push_back(v);
            }
        }
        // Processing face indices.
        else if (strncmp(line_buffer, "f ", 2) == 0) {
            // Face with position/texcoord/normal indices.
            num = sscanf(line_buffer, "f %d/%d/%d %d/%d/%d %d/%d/%d", &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3);
            if (num == 9) {
                indices.emplace_back(v1-1, v2-1, v3-1);
                continue;
            }
            // Face with position//normal indices.
            num = sscanf(line_buffer, "f %d//%d %d//%d %d//%d", &v1, &vn1, &v2, &vn2, &v3, &vn3);
            if (num == 6) {
                indices.emplace_back(v1-1, v2-1, v3-1);
                continue;
            }
            // Face with position/texcoord indices.
            num = sscanf(line_buffer, "f %d/%d %d/%d %d/%d", &v1, &vt1, &v2, &vt2, &v3, &vt3);
            if (num == 6) {
                indices.emplace_back(v1-1, v2-1, v3-1);
                continue;
            }
            // Face with only position indices.
            num = sscanf(line_buffer, "f %d %d %d", &v1, &v2, &v3);
            if (num == 3) {
                indices.emplace_back(v1-1, v2-1, v3-1);
                continue;
            }
        }
    }
    center = center / vertices.size();
    fclose(fp);
}

