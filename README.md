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

## Performance

Projection投影，绘制3\*3\*N个spot模型

| N | 10 | 20 | 50 | 100 | 500 |
| --- | --- | --- | --- | --- | --- |
| 普通Z-Buffer | 370.745ms | 635.732ms | 1208.74ms | 1960.48ms | 7372.03ms |
| 层次Z-Buffer | 307.885ms | 491.697ms | 974.108ms | 1733.21ms | 7010.83ms |
| 加速比 | 1.20x | 1.29x | 1.24x | 1.13x | 1.05x |

绘制相同的场景下，**实际光栅化的三角形数**：

| N | 10 | 20 | 50 | 100 | 500 |
| --- | --- | --- | --- | --- | --- |
| 总三角形数 | 1054080 | 2108160 | 5270400 | 10540800 | 52704000 |
| 普通Z-Buffer | 361744 | 703562 | 1593262 | 2679368 | 4571622 |
| 层次Z-Buffer | 130644 | 187236 | 273096 | 359554 | 588452 |
| 剔除百分比（相对于普通Z-Buffer） | 63.8% | 73.4% | 82.9% | 86.6% | 87.1% |

下面是使用Projection投影，-O3编译结果，绘制5\*5\*N个spot模型：

| N | 20 | 50 | 100 | 200 | 400 |
| --- | --- | --- | --- | --- | --- |
| 普通Z-Buffer | 294.382ms | 516.215ms | 710.22ms | 1004.43ms | 1334.09ms |
| 层次Z-Buffer | 199.325ms | 358.15ms | 568.025ms | 920.318ms | 1275.59ms |
| 加速比 | 1.48x | 1.44x | 1.25x | 1.09x | 1.05 |

下面是使用Orthogonal投影，-O3编译结果，绘制5\*5\*N个spot模型并且使用正交投影：

| N | 20 | 50 | 100 | 200 | 400 |
| --- | --- | --- | --- | --- | --- |
| 普通Z-Buffer | 144.598ms | 341.896ms | 700.132ms | 1352.11ms | 2801.64ms |
| 层次Z-Buffer | 85.1411ms | 200.048ms | 389.271ms | 761.57ms | 1555.26ms |
| 加速比 | 1.70x | 1.71x | 1.80x | 1.78x | 1.80x |