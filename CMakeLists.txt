cmake_minimum_required(VERSION 3.10)
project(ZBuffer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "-x objective-c")

add_executable(viewer src/main.cpp src/image.cpp src/mesh.cpp src/zb_scanline.cpp platform/macos.mm platform/win32.cpp)

target_link_libraries(viewer "-framework Cocoa")