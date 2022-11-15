#include "utils.h"
#include "vector.h"
#include "matrix.h"
#include "image.h"
#include <vector>
#include <list>

/*
 * * * Scanline Z-Buffer Implementation * * *
 * A modification to scanline conversion by adding support for z-buffering
 * How to use:
 * - Init with screen width and height:
 *      auto rasterizer = ZBScanline(w, h);
 * - Pass vertices, indices etc. to render mesh to image
 *      rasterizer.drawMesh(
 *          vertices,   // std::vector<float3>  size == vertex num   # vertex positions.
 *          indices,    // std::vector<int3>    size == triangle num # triangle indices.
 *          colors,     // std::vector<colorf>  size == triangle num # color per triangle.
 *          mvp,        // float4x4 # Model View Projection matrix.
 *          image       // Image    # Image as render target, for detail please refer to limage.h'.
 *      )
 */

struct ZBScanline {

    // Triangle struct as input for sorted edge table
    struct Triangle {
        int2 v[3];
        float z[3];
        float4 surface; // surface equation
        int id;

        int2 min() const { return int2::min(v[0], int2::min(v[1], v[2])); }
        int2 max() const { return int2::max(v[0], int2::max(v[1], v[2])); }
    };
    
    // Sorted Edge Table
    struct SortedEdgeTable {

        // Edge struct for sorted edge table
        struct Edge {
            float x;
            float dx;
            long  y_max; // max y coordinate
            long  id;    // primitive id (triangle id in case of triangle mesh)
            float z;     // current z value on edge
            float dzdx;  // derivation of z w.r.t. x on the surface
            float dzdy;  // derivation of z w.r.t. y on the surface
        };

        int2 min;
        int2 max;
        std::vector<std::vector<Edge>> table;

        SortedEdgeTable(std::vector<Triangle> const& tris, int w, int h);

    private:
        void initTable(std::vector<Triangle> const& tris);
    };

    int width, height;

    ZBScanline(int w, int h)
        : width(w)
        , height(h) {}
    
    void drawMesh(std::vector<float3> const& vertices,
                  std::vector<int3> const& indices, 
                  std::vector<colorf> const& colors,
                  float4x4 const& mvp,
                  Image & image);
};
