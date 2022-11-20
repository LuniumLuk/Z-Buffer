#pragma once

#include <string>
#include <iostream>

enum struct DrawMode {
    Single,
    Matrix3x3,
    Matrix5x5,
};

enum struct ZBufferAlgorithm {
    SimpleZBuffer,
    ScanlineZBuffer,
    HierarchicalZBuffer,
    OctreeZBuffer,
    OctreeZBufferFixed,
};

enum struct RenderMode {
    RealTime,
    Benchmark,
};

enum struct ProjectionMode {
    Perspective,
    Orthogonal,
};

struct Arguments {
    std::string model;
    ZBufferAlgorithm algorithm = ZBufferAlgorithm::SimpleZBuffer;
    int draw_count[2] = { 1, 1 };
    RenderMode render_mode = RenderMode::RealTime;
    int render_count = 0;
    ProjectionMode proj_mode = ProjectionMode::Perspective;
};

void printHelp() {
    std::cout << "-- Z-Buffer Help Info ---------------------------\n";
    std::cout << "Options:\n";
    std::cout << " -i              Model to load, .obj format.\n";
    std::cout << " -z              Z-Buffer algorithm, the following options available:\n";
    std::cout << "     simple      Simple Z-Buffer;\n";
    std::cout << "     scanline    Scanline Z-Buffer;\n";
    std::cout << "     hiez        Hierarchical Z-Buffer;\n";
    std::cout << "     octz        Hierarchical Z-Buffer with Octree Acceleration;\n";
    std::cout << "     octzf       Hierarchical Z-Buffer with Octree Acceleration (static octree);\n";
    std::cout << " -c              Model render count, the following options available:\n";
    std::cout << "     1 n         Render 1 * 1 * n models;\n";
    std::cout << "     3 n         Render 3 * 3 * n models;\n";
    std::cout << "     5 n         Render 5 * 5 * n models;\n";
    std::cout << " -m              Model model, the following options available:\n";
    std::cout << "     r           Realtime mode;\n";
    std::cout << "     b n         Benchmark mode, render n frames and output timer result;\n";
    std::cout << " -p              Projection model, the following options available:\n";
    std::cout << "     p           Perspective mode;\n";
    std::cout << "     o           Orthogonal mode;\n";
}

bool parse(int argc, char* argv[], Arguments * args) {
    if (argc < 2) {
        printHelp();
        return false;
    }

    bool has_model = false;
    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "-i") == 0 && (i < argc - 1)) {
            has_model = true;
            args->model = std::string(argv[i + 1]);
            i += 2;
        }
        else if (strcmp(argv[i], "-z") == 0 && (i < argc - 1)) {
            i += 1;
            if (strcmp(argv[i], "simple") == 0) {
                args->algorithm = ZBufferAlgorithm::SimpleZBuffer;
                i += 1;
            }
            else if (strcmp(argv[i], "scanline") == 0) {
                args->algorithm = ZBufferAlgorithm::ScanlineZBuffer;
                i += 1;
            }
            else if (strcmp(argv[i], "hiez") == 0) {
                args->algorithm = ZBufferAlgorithm::HierarchicalZBuffer;
                i += 1;
            }
            else if (strcmp(argv[i], "octz") == 0) {
                args->algorithm = ZBufferAlgorithm::OctreeZBuffer;
                i += 1;
            }
            else if (strcmp(argv[i], "octzf") == 0) {
                args->algorithm = ZBufferAlgorithm::OctreeZBufferFixed;
                i += 1;
            }
        }
        else if (strcmp(argv[i], "-c") == 0 && (i < argc - 2)) {
            args->draw_count[0] = atoi(argv[i + 1]);
            args->draw_count[1] = atoi(argv[i + 2]);
            i += 3;
        }
        else if (strcmp(argv[i], "-m") == 0 && (i < argc - 1)) {
            i += 1;
            if (strcmp(argv[i], "r") == 0 && (i < argc)) {
                args->render_mode = RenderMode::RealTime;
                i += 1;
            }
            else if (strcmp(argv[i], "b") == 0 && (i < argc - 1)) {
                args->render_mode = RenderMode::Benchmark;
                args->render_count = atoi(argv[i + 1]);
                i += 2;
            }
        }
        else if (strcmp(argv[i], "-p") == 0 && (i < argc - 1)) {
            i += 1;
            if (strcmp(argv[i], "p") == 0 && (i < argc)) {
                args->proj_mode = ProjectionMode::Perspective;
                i += 1;
            }
            else if (strcmp(argv[i], "o") == 0 && (i < argc)) {
                args->proj_mode = ProjectionMode::Orthogonal;
                i += 1;
            }
        }
        else {
            i += 1;
        }
    }
    
    if (!has_model) {
        std::cout << "Please specifies input model!\n";
        printHelp();
    }
    return has_model;
}