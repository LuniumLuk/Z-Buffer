# scanline

CG course project - scanline algorithms

`polygon_scanline.cpp` - Polygon Scanline Rasterization Algorithm using Sorted Edge Table and Active Edge List

How to compile & run: `g++ -std=c++17 -Og polygon_scanline.cpp -o out && ./out`

`scanline_zbuffer.cpp` - Scanline Z-Buffer Rasterization Algorithm using Sorted Edge Table and Active Edge List

How to compile & run: `g++ -std=c++17 -Og scanline_zbuffer.cpp -o out && ./out`

## Viewer App

### Compile

Using hand written **makefile**

MacOS:

1. Compile in zsh `make`

Windows

1. Install MinGW 
2. Compile in cmd `mingw32-make`

### Run

MacOS:

```bash
./viewer
```

Win32:

```bash
viewer
```