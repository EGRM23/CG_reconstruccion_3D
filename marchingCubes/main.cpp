#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <omp.h>  // Necesario para las directivas de OpenMP
#include <dirent.h> // Para leer el directorio

using namespace std;

int GRID_RESOLUTION = 100;  // Valor por defecto
int THREADS_TO_USE = -1;    // -1 = automático (mitad de los núcleos)

struct Point3D {
    float x, y, z;
    Point3D(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
    
    Point3D operator-(const Point3D& other) const {
        return Point3D(x - other.x, y - other.y, z - other.z);
    }
    
    Point3D cross(const Point3D& other) const {
        return Point3D(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }
    
    Point3D normalized() const {
        float len = std::sqrt(x*x + y*y + z*z);
        if (len > 0) {
            return Point3D(x/len, y/len, z/len);
        }
        return *this;
    }
    void print() const
    {
        cout<< "x: "<< x << "\ny: " << y << "\nz: "<< z << endl;
    }
};

Point3D meshCentroid(0, 0, 0);

struct Triangle {
    Point3D vertices[3];
};

const int edgeTable[256] = {
    0x000, 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
    0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
    0x190, 0x099, 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
    0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
    0x230, 0x339, 0x033, 0x13a, 0x636, 0x73f, 0x435, 0x53c,
    0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
    0x3a0, 0x2a9, 0x1a3, 0x0aa, 0x7a6, 0x6af, 0x5a5, 0x4ac,
    0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
    0x460, 0x569, 0x663, 0x76a, 0x066, 0x16f, 0x265, 0x36c,
    0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
    0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0x0ff, 0x3f5, 0x2fc,
    0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
    0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x055, 0x15c,
    0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
    0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0x0cc,
    0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
    0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
    0x0cc, 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
    0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
    0x15c, 0x055, 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
    0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
    0x2fc, 0x3f5, 0x0ff, 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
    0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
    0x36c, 0x265, 0x16f, 0x066, 0x76a, 0x663, 0x569, 0x460,
    0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
    0x4ac, 0x5a5, 0x6af, 0x7a6, 0x0aa, 0x1a3, 0x2a9, 0x3a0,
    0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
    0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x033, 0x339, 0x230,
    0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
    0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x099, 0x190,
    0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
    0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x000
};



const int triTable[256][16] = {
{-1, -1, -1, -1, -1, -1, -1, -1,-1, -1, -1, -1, -1, -1, -1, -1},
{0, 8,  3,  -1, -1, -1, -1, -1, -1, -1,-1, -1, -1, -1, -1, -1},
{0, 1,  9,  -1, -1, -1, -1, -1, -1, -1, -1, -1,-1, -1, -1, -1},
{1, 8,  3,  9,  8,  1,  -1, -1, -1, -1, -1, -1, -1, -1,-1, -1},
{1, 2,  10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8,  3,  1,  2,  10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 2,  10, 0,  2,  9,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 8,  3,  2,  10, 8,  10, 9,  8,  -1, -1, -1, -1, -1, -1, -1},
{3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 11, 2,  8,  11, 0,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 9,  0,  2,  3,  11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 11, 2,  1,  9,  11, 9,  8,  11, -1, -1, -1, -1, -1, -1, -1},
{3, 10, 1,  11, 10, 3,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 10, 1,  0,  8,  10, 8,  11, 10, -1, -1, -1, -1, -1, -1, -1},
{3, 9,  0,  3,  11, 9,  11, 10, 9,  -1, -1, -1, -1, -1, -1, -1},
{9, 8, 10, 10, 8,  11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4,  7, 8,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4,  3, 0,  7,  3,  4,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0,  1,  9, 8,  4,  7,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4,  1,  9,  4, 7,  1,  7,  3,  1,  -1, -1, -1, -1, -1, -1, -1},
{1,  2,  10, 8,  4, 7,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3,  4,  7,  3,  0,  4, 1,  2,  10, -1, -1, -1, -1, -1, -1, -1},
{9,  2,  10, 9,  0,  2,  8, 4,  7,  -1, -1, -1, -1, -1, -1, -1},
{2,  10, 9,  2,  9,  7,  2,  7, 3,  7,  9,  4,  -1, -1, -1, -1},
{8,  4,  7,  3,  11, 2,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{11, 4,  7,  11, 2,  4,  2,  0,  4,  -1, -1, -1, -1, -1, -1, -1},
{9,  0,  1,  8,  4,  7,  2,  3,  11, -1, -1, -1, -1, -1, -1, -1},
{4,  7,  11, 9,  4,  11, 9,  11, 2,  9,  2,  1, -1, -1, -1, -1},
{3,  10, 1,  3,  11, 10, 7,  8,  4,  -1, -1, -1, -1, -1, -1, -1},
{1,  11, 10, 1,  4,  11, 1,  0,  4,  7,  11, 4,  -1, -1, -1, -1},
{4,  7,  8,  9,  0,  11, 9,  11, 10, 11, 0,  3,  -1, -1, -1, -1},
{4,  7,  11, 4,  11, 9,  9,  11, 10, -1, -1, -1, -1, -1, -1, -1},
{9,  5,  4,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9,  5,  4,  0,  8,  3,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0,  5,  4,  1,  5,  0,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{8,  5,  4,  8,  3,  5,  3,  1,  5,  -1, -1, -1, -1, -1, -1, -1},
{1,  2,  10, 9,  5,  4,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3,  0,  8,  1,  2,  10, 4,  9,  5,  -1, -1, -1, -1, -1, -1, -1},
{5,  2,  10, 5,  4,  2,  4,  0,  2,  -1, -1, -1, -1, -1, -1, -1},
{2,  10, 5,  3, 2,  5,  3,  5,  4,  3,  4,  8,  -1, -1, -1, -1},
{9,  5,  4,  2,  3,  11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0,  11, 2,  0,  8,  11, 4,  9,  5,  -1, -1, -1, -1, -1, -1, -1},
{0,  5,  4,  0,  1,  5,  2,  3, 11, -1, -1, -1, -1, -1, -1, -1},
{2,  1,  5,  2,  5,  8,  2,  8,  11, 4,  8,  5,  -1, -1, -1, -1},
{10, 3,  11, 10, 1,  3,  9,  5,  4,  -1, -1, -1, -1, -1, -1, -1},
{4,  9,  5,  0,  8,  1,  8,  10, 1,  8,  11, 
                10, -1, -1, -1, -1},
{5,  4,  0,  5,  0,  11, 5,  11, 10, 11, 0,  3,
                    -1, -1, -1, -1},
{5,  4,  8,  5,  8,  10, 10, 8,  11, -1, -1, -1,
                    -1, -1, -1, -1},
{9,  7,  8,  5,  7,  9,  -1, -1, -1, -1, -1, -1,
                    -1, -1, -1, -1},
{9,  3,  0,  9,  5,  3,  5,  7,  3,  -1, -1, -1, -1,
                        -1, -1, -1},
{0,  7,  8,  0,  1,  7,  1,  5,  7,  -1, -1, -1, -1, -1,
                            -1, -1},
{1,  5,  3,  3,  5,  7,  -1, -1, -1, -1, -1, -1, -1, -1,
                            -1, -1},
{9,  7,  8,  9,  5,  7,  10, 1,  2,  -1, -1, -1, -1, -1, -1,
                                -1},
{10, 1,  2,  9,  5,  0,  5,  3,  0,  5,  7,  3,  -1, -1, -1, -1},
{
8,  0,  2,  8,  2,  5,  8,  5,  7,  10, 5,  2,  -1, -1, -1, -1},
{2,  10,
5,  2,  5,  3,  3,  5,  7,  -1, -1, -1, -1, -1, -1, -1},
{7,  9,  5,  7,
8,  9,  3,  11, 2,  -1, -1, -1, -1, -1, -1, -1},
{9,  5,  7,  9,  7,
2,  9,  2,  0,  2,  7,  11, -1, -1, -1, -1},
{2,  3,  11, 0,  1,  8,  1,
7,  8,  1,  5,  7,  -1, -1, -1, -1},
{11, 2,  1,  11, 1,  7,  7,  1,  5,
        -1, -1, -1, -1, -1, -1, -1},
{9,  5,  8,  8,  5,  7,  10, 1,  3,  10,
            3,  11, -1, -1, -1, -1},
{5,  7,  0,  5,  0,  9,  7,  11, 0,  1,  0,  10,
                    11, 10, 0,  -1},
{11, 10, 0,  11, 0,  3,  10, 5,  0,  8,  0,  7,  5,  7,
                            0,  -1},
{11, 10, 5,  7,  11, 5,  -1, -1, -1, -1, -1, -1, -1, -1,
                            -1, -1},
{10, 6,  5,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                        -1, -1, -1},
{0,  8,  3,  5,  10, 6,  -1, -1, -1, -1, -1, -1, -1,
                        -1, -1, -1},
{9,  0,  1,  5,  10, 6,  -1, -1, -1, -1, -1, -1, -1,
                        -1, -1, -1},
{1,  8,  3,  1,  9,  8,  5,  10, 6,  -1, -1, -1, -1, -1,
                            -1, -1},
{1,  6,  5,  2,  6,  1,  -1, -1, -1, -1, -1, -1, -1, -1,
                            -1, -1},
{1,  6,  5,  1,  2,  6,  3,  0,  8,  -1, -1, -1, -1, -1, -1,
                                -1},
{9,  6,  5,  9,  0,  6,  0,  2,  6,  -1, -1, -1, -1, -1, -1, -1},
{5,  9,  8,  5,  8,  2,  5,  2,  6,  3,  2,  8,  -1, -1, -1, -1},
{2,  3,  11, 10,
6,  5,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{11, 0,  8,  11,
2,  0,  10, 6,  5,  -1, -1, -1, -1, -1, -1, -1},
{0,  1,  9,  2,  3,
11, 5,  10, 6,  -1, -1, -1, -1, -1, -1, -1},
{5,  10, 6,  1,  9,  2,
9,  11, 2,  9,  8,  11, -1, -1, -1, -1},
{6,  3,  11, 6,  5,  3,  5,  1,
    3,  -1, -1, -1, -1, -1, -1, -1},
{0,  8,  11, 0,  11, 5,  0,  5,  1,
        5,  11, 6,  -1, -1, -1, -1},
{3,  11, 6,  0,  3,  6,  0,  6,  5,  0,  5,
                9,  -1, -1, -1, -1},
{6,  5,  9,  6,  9,  11, 11, 9,  8,  -1, -1, -1,
                    -1, -1, -1, -1},
{5,  10, 6,  4,  7,  8,  -1, -1, -1, -1, -1, -1,
                    -1, -1, -1, -1},
{4,  3,  0,  4,  7,  3,  6,  5,  10, -1, -1, -1, -1,
                        -1, -1, -1},
{1,  9,  0,  5,  10, 6,  8,  4,  7,  -1, -1, -1, -1, -1,
                            -1, -1},
{10, 6,  5,  1,  9,  7,  1,  7,  3,  7,  9,  4,  -1, -1, -1, -1},
{6,  1,  2,  6,
5,  1,  4,  7,  8,  -1, -1, -1, -1, -1, -1, -1},
{1,  2,  5,  5,  2,  6,
3,  0,  4,  3,  4,  7,  -1, -1, -1, -1},
{8,  4,  7,  9,  0,  5,  0,  6,  5,
        0,  2,  6,  -1, -1, -1, -1},
{7,  3,  9,  7,  9,  4,  3,  2,  9,  5,  9,  6,
                    2,  6,  9,  -1},
{3,  11, 2,  7,  8,  4,  10, 6,  5,  -1, -1, -1, -1,
                        -1, -1, -1},
{5,  10, 6,  4,  7,  2,  4,  2,  0,  2,  7,  11, -1, -1,
                            -1, -1},
{0,  1,  9,  4,  7,  8,  2,  3,  11, 5,  10, 6,  -1, -1, -1, -1},
{9,  2,  1,  9,
11, 2,  9,  4,  11, 7,  11, 4,  5,  10, 6,  -1},
{8,  4,  7,  3,  11, 5,
3,  5,  1,  5,  11, 6,  -1, -1, -1, -1},
{5,  1,  11, 5,  11, 6,  1,  0,
    11, 7,  11, 4,  0,  4,  11, -1},
{0,  5,  9,  0,  6,  5,  0,  3,  6,  11,
            6,  3,  8,  4,  7,  -1},
{6,  5,  9,  6,  9,  11, 4,  7,  9,  7,  11, 9,  -1,
                        -1, -1, -1},
{10, 4,  9,  6,  4,  10, -1, -1, -1, -1, -1, -1, -1,
                        -1, -1, -1},
{4,  10, 6,  4,  9,  10, 0,  8,  3,  -1, -1, -1, -1, -1,
                            -1, -1},
{10, 0,  1,  10, 6,  0,  6,  4,  0,  -1, -1, -1, -1, -1, -1,
                                -1},
{8,  3,  1,  8,  1,  6,  8,  6,  4,  6,  1,  10, -1, -1, -1, -1},
{
1,  4,  9,  1,  2,  4,  2,  6,  4,  -1, -1, -1, -1, -1, -1, -1},
{3,  0,
8,  1,  2,  9,  2,  4,  9,  2,  6,  4,  -1, -1, -1, -1},
{0,  2,  4,  4,  2,
6,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{8,  3,  2,  8,  2,  4,
4,  2,  6,  -1, -1, -1, -1, -1, -1, -1},
{10, 4,  9,  10, 6,  4,  11,
2,  3,  -1, -1, -1, -1, -1, -1, -1},
{0,  8,  2,  2,  8,  11, 4,  9,
    10, 4,  10, 6,  -1, -1, -1, -1},
{3,  11, 2,  0,  1,  6,  0,  6,  4,  6,
            1,  10, -1, -1, -1, -1},
{6,  4,  1,  6,  1,  10, 4,  8,  1,  2,  1,  11,
                    8,  11, 1,  -1},
{9,  6,  4,  9,  3,  6,  9,  1,  3,  11, 6,  3,  -1, -1,
                            -1, -1},
{8,  11, 1,  8,  1,  0,  11, 6,  1,  9,  1,  4,  6,  4,  1,  -1},
{3,  11, 6,  3,  6,  0,  0,  6,  4,  -1, -1, -1, -1, -1, -1, -1},
{6,  4,  8,
11, 6,  8,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{7,  10, 6,
7,  8,  10, 8,  9,  10, -1, -1, -1, -1, -1, -1, -1},
{0,  7,  3,  0,
10, 7,  0,  9,  10, 6,  7,  10, -1, -1, -1, -1},
{10, 6,  7,  1,  10,
7,  1,  7,  8,  1,  8,  0,  -1, -1, -1, -1},
{10, 6,  7,  10, 7,  1,  1,
7,  3,  -1, -1, -1, -1, -1, -1, -1},
{1,  2,  6,  1,  6,  8,  1,  8,  9,
        8,  6,  7,  -1, -1, -1, -1},
{2,  6,  9,  2,  9,  1,  6,  7,  9,  0,  9,  3,
                    7,  3,  9,  -1},
{7,  8,  0,  7,  0,  6,  6,  0,  2,  -1, -1, -1, -1, -1,
                            -1, -1},
{7,  3,  2,  6,  7,  2,  -1, -1, -1, -1, -1, -1, -1, -1,
                            -1, -1},
{2,  3,  11, 10, 6,  8,  10, 8,  9,  8,  6,  7,  -1, -1, -1,
                                -1},
{2,  0,  7,  2,  7,  11, 0,  9,  7,  6,  7,  10, 9,  10, 7,  -1},
{
1,  8,  0,  1,  7,  8,  1,  10, 7,  6,  7,  10, 2,  3,  11, -1},
{11, 2,
1,  11, 1,  7,  10, 6,  1,  6,  7,  1,  -1, -1, -1, -1},
{8,  9,  6,  8,
6,  7,  9,  1,  6,  11, 6,  3,  1,  3,  6,  -1},
{0,  9,  1,  11, 6,  7,  -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1},
{7,  8,  0,  7,  0,  6,  3,  11,
    0,  11, 6,  0,  -1, -1, -1, -1},
{7,  11, 6,  -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1},
{7,  6,  11, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1},
{3,  0,  8,  11, 7,  6,  -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1},
{0,  1,  9,  11, 7,  6,  -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1},
{8,  1,  9,  8,  3,  1,  11, 7,
    6,  -1, -1, -1, -1, -1, -1, -1},
{10, 1,  2,  6,  11, 7,  -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1},
{1,  2,  10, 3,  0,  8,  6,  11, 7,
        -1, -1, -1, -1, -1, -1, -1},
{2,  9,  0,  2,  10, 9,  6,  11, 7,  -1,
            -1, -1, -1, -1, -1, -1},
{6,  11, 7,  2,  10, 3,  10, 8,  3,  10, 9,
                8,  -1, -1, -1, -1},
{7,  2,  3,  6,  2,  7,  -1, -1, -1, -1, -1, -1,
                    -1, -1, -1, -1},
{7,  0,  8,  7,  6,  0,  6,  2,  0,  -1, -1, -1, -1,
                        -1, -1, -1},
{2,  7,  6,  2,  3,  7,  0,  1,  9,  -1, -1, -1, -1, -1,
                            -1, -1},
{1,  6,  2,  1,  8,  6,  1,  9,  8,  8,  7,  6,  -1, -1, -1, -1},
{10, 7,  6,  10,
1,  7,  1,  3,  7,  -1, -1, -1, -1, -1, -1, -1},
{10, 7,  6,  1,  7,
10, 1,  8,  7,  1,  0,  8,  -1, -1, -1, -1},
{0,  3,  7,  0,  7,  10, 0,
10, 9,  6,  10, 7,  -1, -1, -1, -1},
{7,  6,  10, 7,  10, 8,  8,  10,
    9,  -1, -1, -1, -1, -1, -1, -1},
{6,  8,  4,  11, 8,  6,  -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1},
{3,  6,  11, 3,  0,  6,  0,  4,  6,
        -1, -1, -1, -1, -1, -1, -1},
{8,  6,  11, 8,  4,  6,  9,  0,  1,  -1,
            -1, -1, -1, -1, -1, -1},
{9,  4,  6,  9,  6,  3,  9,  3,  1,  11, 3,  6,
                    -1, -1, -1, -1},
{6,  8,  4,  6,  11, 8,  2,  10, 1,  -1, -1, -1, -1,
                        -1, -1, -1},
{1,  2,  10, 3,  0,  11, 0,  6,  11, 0,  4,  6,  -1, -1,
                            -1, -1},
{4,  11, 8,  4,  6,  11, 0,  2,  9,  2,  10, 9,  -1, -1, -1,
                                -1},
{10, 9,  3,  10, 3,  2,  9,  4,  3,  11, 3,  6,  4,  6,  3,  -1},
{
8,  2,  3,  8,  4,  2,  4,  6,  2,  -1, -1, -1, -1, -1, -1, -1},
{0,  4,
2,  4,  6,  2,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1,  9,  0,
2,  3,  4,  2,  4,  6,  4,  3,  8,  -1, -1, -1, -1},
{1,  9,  4,  1,  4,  2,
2,  4,  6,  -1, -1, -1, -1, -1, -1, -1},
{8,  1,  3,  8,  6,  1,  8,  4,
    6,  6,  10, 1,  -1, -1, -1, -1},
{10, 1,  0,  10, 0,  6,  6,  0,  4,
        -1, -1, -1, -1, -1, -1, -1},
{4,  6,  3,  4,  3,  8,  6,  10, 3,  0,
            3,  9,  10, 9,  3,  -1},
{10, 9,  4,  6,  10, 4,  -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1},
{4,  9,  5,  7,  6,  11, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1},
{0,  8,  3,  4,  9,  5,  11, 7,  6,  -1, -1, -1,
                    -1, -1, -1, -1},
{5,  0,  1,  5,  4,  0,  7,  6,  11, -1, -1, -1, -1,
                        -1, -1, -1},
{11, 7,  6,  8,  3,  4,  3,  5,  4,  3,  1,  5,  -1, -1, -1,
                                -1},
{9,  5,  4,  10, 1,  2,  7,  6,  11, -1, -1, -1, -1, -1, -1, -1},
{6,  11, 7,
1,  2,  10, 0,  8,  3,  4,  9,  5,  -1, -1, -1, -1},
{7,  6,  11, 5,  4,
10, 4,  2,  10, 4,  0,  2,  -1, -1, -1, -1},
{3,  4,  8,  3,  5,  4,  3,
2,  5,  10, 5,  2,  11, 7,  6,  -1},
{7,  2,  3,  7,  6,  2,  5,  4,  9,  -1,
            -1, -1, -1, -1, -1, -1},
{9,  5,  4,  0,  8,  6,  0,  6,  2,  6,  8,  7,
                    -1, -1, -1, -1},
{3,  6,  2,  3,  7,  6,  1,  5,  0,  5,  4,  0,  -1, -1,
                            -1, -1},
{6,  2,  8,  6,  8,  7,  2,  1,  8,  4,  8,  5,  1,  5,  8,  -1},
{
9,  5,  4,  10, 1,  6,  1,  7,  6,  1,  3,  7,  -1, -1, -1, -1},
{1,  6,
10, 1,  7,  6,  1,  0,  7,  8,  7,  0,  9,  5,  4,  -1},
{4,  0,  10, 4,  10,
5,  0,  3,  10, 6,  10, 7,  3,  7,  10, -1},
{7,  6,  10, 7,  10, 8,  5,
4,  10, 4,  8,  10, -1, -1, -1, -1},
{6,  9,  5,  6,  11, 9,  11, 8,
    9,  -1, -1, -1, -1, -1, -1, -1},
{3,  6,  11, 0,  6,  3,  0,  5,  6,
        0,  9,  5,  -1, -1, -1, -1},
{0,  11, 8,  0,  5,  11, 0,  1,  5,  5,  6,
                11, -1, -1, -1, -1},
{6,  11, 3,  6,  3,  5,  5,  3,  1,  -1, -1, -1,
                    -1, -1, -1, -1},
{1,  2,  10, 9,  5,  11, 9,  11, 8,  11, 5,  6,  -1,
                        -1, -1, -1},
{0,  11, 3,  0,  6,  11, 0,  9,  6,  5,  6,  9,  1,  2,  10,
                                -1},
{11, 8,  5,  11, 5,  6,  8,  0,  5,  10, 5,  2,  0,  2,  5,  -1},
{
6,  11, 3,  6,  3,  5,  2,  10, 3,  10, 5,  3,  -1, -1, -1, -1},
{5,  8,
9,  5,  2,  8,  5,  6,  2,  3,  8,  2,  -1, -1, -1, -1},
{9,  5,  6,  9,  6,
0,  0,  6,  2,  -1, -1, -1, -1, -1, -1, -1},
{1,  5,  8,  1,  8,  0,  5,
6,  8,  3,  8,  2,  6,  2,  8,  -1},
{1,  5,  6,  2,  1,  6,  -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1},
{1,  3,  6,  1,  6,  10, 3,  8,  6,  5,
            6,  9,  8,  9,  6,  -1},
{10, 1,  0,  10, 0,  6,  9,  5,  0,  5,  6,  0,  -1,
                        -1, -1, -1},
{0,  3,  8,  5,  6,  10, -1, -1, -1, -1, -1, -1, -1,
                        -1, -1, -1},
{10, 5,  6,  -1, -1, -1, -1, -1, -1, -1, -1, -1,
                    -1, -1, -1, -1},
{11, 5,  10, 7,  5,  11, -1, -1, -1, -1, -1, -1,
                    -1, -1, -1, -1},
{11, 5,  10, 11, 7,  5,  8,  3,  0,  -1, -1, -1,
                    -1, -1, -1, -1},
{5,  11, 7,  5,  10, 11, 1,  9,  0,  -1, -1, -1,
                    -1, -1, -1, -1},
{10, 7,  5,  10, 11, 7,  9,  8,  1,  8,  3,  1,  -1,
                        -1, -1, -1},
{11, 1,  2,  11, 7,  1,  7,  5,  1,  -1, -1, -1, -1, -1,
                            -1, -1},
{0,  8,  3,  1,  2,  7,  1,  7,  5,  7,  2,  11, -1, -1, -1, -1},
{9,  7,  5,  9,
2,  7,  9,  0,  2,  2,  11, 7,  -1, -1, -1, -1},
{7,  5,  2,  7,  2,  11,
5,  9,  2,  3,  2,  8,  9,  8,  2,  -1},
{2,  5,  10, 2,  3,  5,  3,  7,  5,
        -1, -1, -1, -1, -1, -1, -1},
{8,  2,  0,  8,  5,  2,  8,  7,  5,  10,
            2,  5,  -1, -1, -1, -1},
{9,  0,  1,  5,  10, 3,  5,  3,  7,  3,  10, 2,
                    -1, -1, -1, -1},
{9,  8,  2,  9,  2,  1,  8,  7,  2,  10, 2,  5,  7,  5,
                            2,  -1},
{1,  3,  5,  3,  7,  5,  -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                -1},
{0,  8,  7,  0,  7,  1,  1,  7,  5,  -1, -1, -1, -1, -1, -1, -1},
{9,  0,  3,  9,  3,  5,  5,  3,  7,  -1, -1, -1, -1, -1, -1, -1},
{9,  8,  7,  5,
9,  7,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{5,  8,  4,  5,
10, 8,  10, 11, 8,  -1, -1, -1, -1, -1, -1, -1},
{5,  0,  4,  5,
11, 0,  5,  10, 11, 11, 3,  0,  -1, -1, -1, -1},
{0,  1,  9,  8,  4,
10, 8,  10, 11, 10, 4,  5,  -1, -1, -1, -1},
{10, 11, 4,  10, 4,
5,  11, 3,  4,  9,  4,  1,  3,  1,  4,  -1},
{2,  5,  1,  2,  8,  5,  2,  11,
    8,  4,  5,  8,  -1, -1, -1, -1},
{0,  4,  11, 0,  11, 3,  4,  5,  11, 2,
            11, 1,  5,  1,  11, -1},
{0,  2,  5,  0,  5,  9,  2,  11, 5,  4,  5,  8,
                    11, 8,  5,  -1},
{9,  4,  5,  2,  11, 3,  -1, -1, -1, -1, -1, -1, -1,
                        -1, -1, -1},
{2,  5,  10, 3,  5,  2,  3,  4,  5,  3,  8,  4,  -1, -1, -1,
                                -1},
{5,  10, 2,  5,  2,  4,  4,  2,  0,  -1, -1, -1, -1, -1, -1, -1},
{3,  10, 2,
3,  5,  10, 3,  8,  5,  4,  5,  8,  0,  1,  9,  -1},
{5,  10, 2,  5,  2,  4,
1,  9,  2,  9,  4,  2,  -1, -1, -1, -1},
{8,  4,  5,  8,  5,  3,  3,  5,  1,
        -1, -1, -1, -1, -1, -1, -1},
{0,  4,  5,  1,  0,  5,  -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1},
{8,  4,  5,  8,  5,  3,  9,  0,  5,  0,  3,
                5,  -1, -1, -1, -1},
{9,  4,  5,  -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1},
{4,  11, 7,  4,  9,  11, 9,  10, 11, -1, -1,
                -1, -1, -1, -1, -1},
{0,  8,  3,  4,  9,  7,  9,  11, 7,  9,  10, 11,
                    -1, -1, -1, -1},
{1,  10, 11, 1,  11, 4,  1,  4,  0,  7,  4,  11, -1,
                        -1, -1, -1},
{3,  1,  4,  3,  4,  8,  1,  10, 4,  7,  4,  11, 10, 11, 4,
                                -1},
{4,  11, 7,  9,  11, 4,  9,  2,  11, 9,  1,  2,  -1, -1, -1, -1},
{9,  7,  4,  9,  11, 7,  9,  1,  11, 2,  11, 1,  0,  8,  3,  -1},
{11, 7,  4,  11,
4,  2,  2,  4,  0,  -1, -1, -1, -1, -1, -1, -1},
{11, 7,  4,  11, 4,
2,  8,  3,  4,  3,  2,  4,  -1, -1, -1, -1},
{2,  9,  10, 2,  7,  9,  2,
3,  7,  7,  4,  9,  -1, -1, -1, -1},
{9,  10, 7,  9,  7,  4,  10, 2,  7,
        8,  7,  0,  2,  0,  7,  -1},
{3,  7,  10, 3,  10, 2,  7,  4,  10, 1,  10,
                0,  4,  0,  10, -1},
{1,  10, 2,  8,  7,  4,  -1, -1, -1, -1, -1, -1,
                    -1, -1, -1, -1},
{4,  9,  1,  4,  1,  7,  7,  1,  3,  -1, -1, -1, -1,
                        -1, -1, -1},
{4,  9,  1,  4,  1,  7,  0,  8,  1,  8,  7,  1,  -1, -1, -1,
                                -1},
{4,  0,  3,  7,  4,  3,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4,  8,  7,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9,  10,
8,  10, 11, 8,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3,  0,
9,  3,  9,  11, 11, 9,  10, -1, -1, -1, -1, -1, -1, -1},
{0,  1,
10, 0,  10, 8,  8,  10, 11, -1, -1, -1, -1, -1, -1, -1},
{3,  1,
10, 11, 3,  10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1,  2,
11, 1,  11, 9,  9,  11, 8,  -1, -1, -1, -1, -1, -1, -1},
{3,  0,  9,
3,  9,  11, 1,  2,  9,  2,  11, 9,  -1, -1, -1, -1},
{0,  2,  11, 8,  0,
11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3,  2,  11, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2,  3,  8,  2,
8,  10, 10, 8,  9,  -1, -1, -1, -1, -1, -1, -1},
{9,  10, 2,  0,  9,
2,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2,  3,  8,  2,  8,
10, 0,  1,  8,  1,  10, 8,  -1, -1, -1, -1},
{1,  10, 2,  -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1,  3,  8,  9,  1,  8,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0,  9,  1,  -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0,  3,  8,  -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{-1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};


class MarchingCubes {
private:
    std::vector<Point3D> pointCloud;
    std::vector<std::vector<std::vector<float>>> scalarField;
    std::vector<Triangle> triangles;
    
    float gridSize;
    int gridResolution;
    Point3D minBounds, maxBounds;
    
public:
    MarchingCubes(float gridSize = 0.1f, int resolution = GRID_RESOLUTION) 
        : gridSize(gridSize), gridResolution(resolution) {}
    
    // Cargar nube de puntos desde archivo .xyz
    bool loadPointCloud(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: No se pudo abrir el archivo " << filename << std::endl;
            return false;
        }
        
        pointCloud.clear();
        float x, y, z;
        
        // Leer puntos del archivo
        while (file >> x >> y >> z) {
            pointCloud.push_back(Point3D(x, y, z));
        }
        
        file.close();
        std::cout << "Cargados " << pointCloud.size() << " puntos" << std::endl;
        
        // Calcular límites de la nube de puntos
        calculateBounds();
        return true;
    }
    
    // Calcular límites de la nube de puntos
    void calculateBounds() {
        if (pointCloud.empty()) return;
        
        minBounds = maxBounds = pointCloud[0];
        
        for (const auto& point : pointCloud) {
            minBounds.x = std::min(minBounds.x, point.x);
            minBounds.y = std::min(minBounds.y, point.y);
            minBounds.z = std::min(minBounds.z, point.z);
            
            maxBounds.x = std::max(maxBounds.x, point.x);
            maxBounds.y = std::max(maxBounds.y, point.y);
            maxBounds.z = std::max(maxBounds.z, point.z);
        }

        // Mostrar información detallada
        // std::cout << "\n=== Límites de la nube de puntos ===" << std::endl;
        // std::cout << "- Límites brutos (sin padding):" << std::endl;
        // std::cout << "  X: [" << minBounds.x << ", " << maxBounds.x << "]" << std::endl;
        // std::cout << "  Y: [" << minBounds.y << ", " << maxBounds.y << "]" << std::endl;
        // std::cout << "  Z: [" << minBounds.z << ", " << maxBounds.z << "]" << std::endl;
        // std::cout << "- Rango bruto:" << std::endl;
        // std::cout << "  ΔX: " << (maxBounds.x - minBounds.x) << " units" << std::endl;
        // std::cout << "  ΔY: " << (maxBounds.y - minBounds.y) << " units" << std::endl;
        // std::cout << "  ΔZ: " << (maxBounds.z - minBounds.z) << " units" << std::endl;
        
        // Aplicar padding (10% del rango o valor fijo si el rango es cero)
        float padding = 0.1f;
        float safePaddingX = (maxBounds.x == minBounds.x) ? padding : (maxBounds.x - minBounds.x) * padding;
        float safePaddingY = (maxBounds.y == minBounds.y) ? padding : (maxBounds.y - minBounds.y) * padding;
        float safePaddingZ = (maxBounds.z == minBounds.z) ? padding : (maxBounds.z - minBounds.z) * padding;

        minBounds.x -= safePaddingX; 
        minBounds.y -= safePaddingY; 
        minBounds.z -= safePaddingZ;
        maxBounds.x += safePaddingX; 
        maxBounds.y += safePaddingY; 
        maxBounds.z += safePaddingZ;

        // Mostrar límites con padding
        // std::cout << "- Límites con padding (" << (padding*100) << "% del rango):" << std::endl;
        // std::cout << "  X: [" << minBounds.x << ", " << maxBounds.x << "]" << std::endl;
        // std::cout << "  Y: [" << minBounds.y << ", " << maxBounds.y << "]" << std::endl;
        // std::cout << "  Z: [" << minBounds.z << ", " << maxBounds.z << "]" << std::endl;
        // std::cout << "- Nuevo rango:" << std::endl;
        // std::cout << "  ΔX: " << (maxBounds.x - minBounds.x) << " units" << std::endl;
        // std::cout << "  ΔY: " << (maxBounds.y - minBounds.y) << " units" << std::endl;
        // std::cout << "  ΔZ: " << (maxBounds.z - minBounds.z) << " units" << std::endl;
        // std::cout << "=================================\n" << std::endl;
    }
    
    // Crear campo escalar usando distancia a la nube de puntos
    void createScalarField() {
        float rangeX = maxBounds.x - minBounds.x;
        float rangeY = maxBounds.y - minBounds.y;
        float rangeZ = maxBounds.z - minBounds.z;
        
        scalarField.resize(gridResolution);
        for (int i = 0; i < gridResolution; i++) {
            //std::cout << "Punto " << i << "/" << gridResolution << std::endl;
            scalarField[i].resize(gridResolution);
            for (int j = 0; j < gridResolution; j++) {
                scalarField[i][j].resize(gridResolution);
            }
        }
        
        int num_procs = omp_get_num_procs();
        if (THREADS_TO_USE > 0) {
            omp_set_num_threads(THREADS_TO_USE);
        } else {
            omp_set_num_threads(num_procs / 2);  // Mitad de los núcleos por defecto
        }
        std::cout << "Usando " << omp_get_max_threads() << " hilos de " << num_procs << " disponibles" << std::endl;
        
        // Calcular distancias (paralelizar el bucle exterior)
        #pragma omp parallel for
        for (int i = 0; i < gridResolution; i++) {
            //std::cout << "Distancia " << i << "/" << gridResolution << " (Hilo " << omp_get_thread_num() << ")" << std::endl;
            
            for (int j = 0; j < gridResolution; j++) {
                for (int k = 0; k < gridResolution; k++) {
                    Point3D gridPoint(
                        minBounds.x + (i * rangeX) / (gridResolution - 1),
                        minBounds.y + (j * rangeY) / (gridResolution - 1),
                        minBounds.z + (k * rangeZ) / (gridResolution - 1)
                    );
                    
                    // Encontrar distancia mínima a la nube de puntos
                    float minDistance = std::numeric_limits<float>::max();
                    for (const auto& point : pointCloud) {
                        float dx = gridPoint.x - point.x;
                        float dy = gridPoint.y - point.y;
                        float dz = gridPoint.z - point.z;
                        float distance = std::sqrt(dx*dx + dy*dy + dz*dz);
                        minDistance = std::min(minDistance, distance);
                    }
                    
                    scalarField[i][j][k] = minDistance;
                }
            }
        }
    }
    // Interpolación linear entre dos puntos
    Point3D interpolate(const Point3D& p1, const Point3D& p2, float val1, float val2, float isoValue) {
        if (std::abs(val1 - val2) < 1e-6) return p1;
        
        float t = (isoValue - val1) / (val2 - val1);
        return Point3D(
            p1.x + t * (p2.x - p1.x),
            p1.y + t * (p2.y - p1.y),
            p1.z + t * (p2.z - p1.z)
        );
    }
    
    // Aplicar algoritmo de Marching Cubes
    void marchingCubes(float isoValue = 0.05f) {
        triangles.clear();
        
        float rangeX = maxBounds.x - minBounds.x;
        float rangeY = maxBounds.y - minBounds.y;
        float rangeZ = maxBounds.z - minBounds.z;
        
        for (int i = 0; i < gridResolution - 1; i++) {
            for (int j = 0; j < gridResolution - 1; j++) {
                for (int k = 0; k < gridResolution - 1; k++) {
                    // Obtener los 8 vértices del cubo
                    Point3D vertices[8];
                    float values[8];
                    
                    vertices[0] = Point3D(minBounds.x + (i * rangeX) / (gridResolution - 1),
                                        minBounds.y + (j * rangeY) / (gridResolution - 1),
                                        minBounds.z + (k * rangeZ) / (gridResolution - 1));
                    vertices[1] = Point3D(vertices[0].x + rangeX / (gridResolution - 1), vertices[0].y, vertices[0].z);
                    vertices[2] = Point3D(vertices[1].x, vertices[1].y + rangeY / (gridResolution - 1), vertices[1].z);
                    vertices[3] = Point3D(vertices[0].x, vertices[0].y + rangeY / (gridResolution - 1), vertices[0].z);
                    vertices[4] = Point3D(vertices[0].x, vertices[0].y, vertices[0].z + rangeZ / (gridResolution - 1));
                    vertices[5] = Point3D(vertices[1].x, vertices[1].y, vertices[1].z + rangeZ / (gridResolution - 1));
                    vertices[6] = Point3D(vertices[2].x, vertices[2].y, vertices[2].z + rangeZ / (gridResolution - 1));
                    vertices[7] = Point3D(vertices[3].x, vertices[3].y, vertices[3].z + rangeZ / (gridResolution - 1));
                    
                    values[0] = scalarField[i][j][k];
                    values[1] = scalarField[i+1][j][k];
                    values[2] = scalarField[i+1][j+1][k];
                    values[3] = scalarField[i][j+1][k];
                    values[4] = scalarField[i][j][k+1];
                    values[5] = scalarField[i+1][j][k+1];
                    values[6] = scalarField[i+1][j+1][k+1];
                    values[7] = scalarField[i][j+1][k+1];
                    
                    // Determinar configuración del cubo
                    int cubeIndex = 0;
                    for (int n = 0; n < 8; n++) {
                        if (values[n] < isoValue) {
                            cubeIndex |= (1 << n);
                        }
                    }
                    
                    // Si el cubo está completamente dentro o fuera, continuar
                    if (edgeTable[cubeIndex] == 0) continue;
                    
                    // Calcular puntos de intersección en las aristas
                    Point3D vertList[12];
                    if (edgeTable[cubeIndex] & 1)
                        vertList[0] = interpolate(vertices[0], vertices[1], values[0], values[1], isoValue);
                    if (edgeTable[cubeIndex] & 2)
                        vertList[1] = interpolate(vertices[1], vertices[2], values[1], values[2], isoValue);
                    if (edgeTable[cubeIndex] & 4)
                        vertList[2] = interpolate(vertices[2], vertices[3], values[2], values[3], isoValue);
                    if (edgeTable[cubeIndex] & 8)
                        vertList[3] = interpolate(vertices[3], vertices[0], values[3], values[0], isoValue);
                    if (edgeTable[cubeIndex] & 16)
                        vertList[4] = interpolate(vertices[4], vertices[5], values[4], values[5], isoValue);
                    if (edgeTable[cubeIndex] & 32)
                        vertList[5] = interpolate(vertices[5], vertices[6], values[5], values[6], isoValue);
                    if (edgeTable[cubeIndex] & 64)
                        vertList[6] = interpolate(vertices[6], vertices[7], values[6], values[7], isoValue);
                    if (edgeTable[cubeIndex] & 128)
                        vertList[7] = interpolate(vertices[7], vertices[4], values[7], values[4], isoValue);
                    if (edgeTable[cubeIndex] & 256)
                        vertList[8] = interpolate(vertices[0], vertices[4], values[0], values[4], isoValue);
                    if (edgeTable[cubeIndex] & 512)
                        vertList[9] = interpolate(vertices[1], vertices[5], values[1], values[5], isoValue);
                    if (edgeTable[cubeIndex] & 1024)
                        vertList[10] = interpolate(vertices[2], vertices[6], values[2], values[6], isoValue);
                    if (edgeTable[cubeIndex] & 2048)
                        vertList[11] = interpolate(vertices[3], vertices[7], values[3], values[7], isoValue);
                    
                    // Crear triángulos
                    for (int n = 0; triTable[cubeIndex][n] != -1; n += 3) {
                        Triangle tri;
                        tri.vertices[0] = vertList[triTable[cubeIndex][n]];
                        tri.vertices[1] = vertList[triTable[cubeIndex][n+1]];
                        tri.vertices[2] = vertList[triTable[cubeIndex][n+2]];
                        triangles.push_back(tri);
                    }
                }
            }
        }
        
        std::cout << "Generados " << triangles.size() << " triángulos" << std::endl;
    }
    
    // Obtener triángulos generados
    const std::vector<Triangle>& getTriangles() const {
        return triangles;
    }
    
    // Procesar todo el pipeline
    void process(const std::string& filename, float isoValue = 0.05f) {
        if (!loadPointCloud(filename)) return;
    
        std::cout << "Creando campo escalar..." << std::endl;
        createScalarField();
        
        std::cout << "Aplicando Marching Cubes..." << std::endl;
        marchingCubes(isoValue);
        
        // Calcular el centroide después de generar los triángulos
        meshCentroid = calculateCentroid();
        std::cout << "Centroide de la malla: ";
        meshCentroid.print();
    }

    Point3D calculateCentroid() const {
        if (triangles.empty()) return Point3D(0, 0, 0);
        
        Point3D sum(0, 0, 0);
        int count = 0;
        
        for (const Triangle& tri : triangles) {
            for (int i = 0; i < 3; i++) {
                sum.x += tri.vertices[i].x;
                sum.y += tri.vertices[i].y;
                sum.z += tri.vertices[i].z;
                count++;
            }
        }
        
        float centroide_x = sum.x/count;
        float centroide_y = sum.y/count;
        float centroide_z = sum.z/count;

        cout << "Centroide:\n\t-x: " << centroide_x << endl;
        cout << "\t-y: " << centroide_y << endl;
        cout << "\t-z: " << centroide_z << endl;

        return Point3D(centroide_x, centroide_y, centroide_z);
    }

    const vector<Point3D>& getPointCloud() const
    {
        return pointCloud;
    }

    Point3D getMinBounds() const { return minBounds; }
    Point3D getMaxBounds() const { return maxBounds; }
};

// Shader simple para visualización
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 Normal;
out vec3 FragPos;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 objectColor;
uniform vec3 lightColor;

void main() {
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
    
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    vec3 result = (ambient + diffuse) * objectColor;
    FragColor = vec4(result, 1.0);
}
)";

#include <GL/glut.h>

// Estructura para almacenar una malla con su color
struct OrganMesh {
    MarchingCubes mc;
    float color[3];
    std::string name;
    bool visible;
    int id;
};
std::vector<OrganMesh> organMeshes;
int totalOrgans = 0;

const float ORGAN_COLORS[17][3] = {
    {1.0f, 0.0f, 0.0f},     // Rojo
    {0.0f, 1.0f, 0.0f},     // Verde
    {0.0f, 0.0f, 1.0f},     // Azul
    {1.0f, 1.0f, 0.0f},     // Amarillo
    {1.0f, 0.0f, 1.0f},     // Magenta
    {0.0f, 1.0f, 1.0f},     // Cian
    {1.0f, 0.5f, 0.0f},     // Naranja
    {0.5f, 0.0f, 0.5f},     // Púrpura
    {0.5f, 0.5f, 0.0f},     // Oliva
    {0.0f, 0.5f, 0.5f},     // Verde azulado
    {0.5f, 0.0f, 0.0f},     // Rojo oscuro
    {0.0f, 0.5f, 0.0f},     // Verde oscuro
    {0.0f, 0.0f, 0.5f},     // Azul oscuro
    {0.8f, 0.6f, 0.2f},     // Oro
    {0.3f, 0.7f, 0.1f},     // Verde claro
    {0.7f, 0.2f, 0.5f},     // Rosa oscuro
    {0.2f, 0.8f, 0.8f}      // Turquesa
};

float rotX = 0, rotY = 0;
int lastX = 0, lastY = 0;
bool mouseLeftDown = false;
float zoom = 30.0f;  // Valor inicial de zoom (distancia de la cámara)
const float ZOOM_SPEED = 0.1f;  // Velocidad del zoom
const float MIN_ZOOM = 0.1f;
const float MAX_ZOOM = 1000.0f;
bool showPoints = false; // Estado inicial: visible

// Función para cargar órganos desde directorio
void loadOrganMeshes(const std::string& directory, float isoValue = 0.05f) {
    DIR *dir;
    struct dirent *ent;
    std::vector<std::string> organFiles;
    
    if ((dir = opendir(directory.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            std::string filename = ent->d_name;
            if (filename.size() > 4 && filename.substr(filename.size() - 4) == ".xyz") {
                organFiles.push_back(filename);
            }
        }
        closedir(dir);
        
        // Ordenar alfabéticamente para consistencia
        std::sort(organFiles.begin(), organFiles.end());
        
        // Verificar si hay archivos
        if (organFiles.empty()) {
            std::cerr << "Error: No se encontraron archivos .xyz en el directorio." << std::endl;
            exit(1);
        }

        // Mensaje informativo
        std::cout << "Se encontraron " << organFiles.size() 
                  << " archivos de órganos (de un máximo de 17)" << std::endl;
        
        // Procesar cada archivo
        for (size_t i = 0; i < organFiles.size() && i < 17; i++) {
            OrganMesh om;
            om.id = i + 1;
            om.name = organFiles[i].substr(0, organFiles[i].size() - 4);
            om.visible = true;
            
            // Asignar color de la paleta (cicla si hay más de 17)
            for (int j = 0; j < 3; j++) {
                om.color[j] = ORGAN_COLORS[i % 17][j];
            }
            
            std::string fullpath = directory + "/" + organFiles[i];
            std::cout << "\n=== ORGANO 1 ===" << std::endl;
            std::cout << "Procesando órgano " << om.id << ": " << om.name << std::endl;
            om.mc.process(fullpath, isoValue);
            std::cout << "=================================\n" << std::endl;

            organMeshes.push_back(om);
        }

        totalOrgans = organMeshes.size();
        
        std::cout << "\n=== RESUMEN ===" << std::endl;
        std::cout << "Órganos cargados: " << totalOrgans << "/17" << std::endl;
        for (const auto& om : organMeshes) {
            std::cout << "Órgano " << om.id << ": " << om.name 
                      << " - Triángulos: " << om.mc.getTriangles().size() << std::endl;
        }
    } else {
        std::cerr << "Error: No se pudo abrir el directorio " << directory << std::endl;
        exit(1);
    }
}

// Función para mostrar información de los órganos
void printOrganInfo() {
    std::cout << "\n=== ÓRGANOS CARGADOS ===" << std::endl;
    for (const auto& om : organMeshes) {
        std::cout << "ID: " << om.id << " | Nombre: " << om.name 
                  << " | Visible: " << (om.visible ? "Sí" : "No")
                  << " | Color: (" << om.color[0] << ", " 
                  << om.color[1] << ", " << om.color[2] << ")" << std::endl;
    }
}

void calculateGlobalBounds(Point3D& globalMin, Point3D& globalMax) {
    if (organMeshes.empty()) return;

    // Inicializar con los límites del primer órgano
    globalMin = organMeshes[0].mc.getMinBounds();
    globalMax = organMeshes[0].mc.getMaxBounds();

    // Expandir los límites con los demás órganos
    for (const auto& om : organMeshes) {
        Point3D currMin = om.mc.getMinBounds();
        Point3D currMax = om.mc.getMaxBounds();

        globalMin.x = std::min(globalMin.x, currMin.x);
        globalMin.y = std::min(globalMin.y, currMin.y);
        globalMin.z = std::min(globalMin.z, currMin.z);

        globalMax.x = std::max(globalMax.x, currMax.x);
        globalMax.y = std::max(globalMax.y, currMax.y);
        globalMax.z = std::max(globalMax.z, currMax.z);
    }
}

// Función para inicializar OpenGL
void initGL() {
    glClearColor(0.7176f, 0.7412f, 0.7255f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    GLfloat lightPos[4] = {0.0f, 1.0f, 1.0f, 0.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    
    GLfloat ambient[4] = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat diffuse[4] = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat specular[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    
    glShadeModel(GL_SMOOTH);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    // Vista centrada en el centroide global
    gluLookAt(meshCentroid.x, meshCentroid.y, meshCentroid.z + zoom,
              meshCentroid.x, meshCentroid.y, meshCentroid.z,
              0, 1, 0);
    
    // Aplicar rotaciones alrededor del centroide
    glTranslatef(meshCentroid.x, meshCentroid.y, meshCentroid.z);
    glRotatef(rotX, 1, 0, 0);
    glRotatef(rotY, 0, 1, 0);
    glTranslatef(-meshCentroid.x, -meshCentroid.y, -meshCentroid.z);
    
    // Dibujar ejes de coordenadas en el centroide
    glBegin(GL_LINES);
    // Eje X (rojo)
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(meshCentroid.x, meshCentroid.y, meshCentroid.z);
    glVertex3f(meshCentroid.x + 1.0f, meshCentroid.y, meshCentroid.z);
    
    // Eje Y (verde)
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(meshCentroid.x, meshCentroid.y, meshCentroid.z);
    glVertex3f(meshCentroid.x, meshCentroid.y + 1.0f, meshCentroid.z);
    
    // Eje Z (azul)
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(meshCentroid.x, meshCentroid.y, meshCentroid.z);
    glVertex3f(meshCentroid.x, meshCentroid.y, meshCentroid.z + 1.0f);
    glEnd();
    
    // Dibujar todos los órganos visibles
    for (const auto& om : organMeshes) {
        if (!om.visible) continue;
        
        glColor3fv(om.color);
        glBegin(GL_TRIANGLES);
        
        const auto& triangles = om.mc.getTriangles();
        for (const auto& tri : triangles) {
            // Calcular normal
            Point3D v1 = tri.vertices[1] - tri.vertices[0];
            Point3D v2 = tri.vertices[2] - tri.vertices[0];
            Point3D normal = v1.cross(v2).normalized();
            
            glNormal3f(normal.x, normal.y, normal.z);
            for (int i = 0; i < 3; i++) {
                glVertex3f(tri.vertices[i].x, tri.vertices[i].y, tri.vertices[i].z);
            }
        }
        
        // Dibujar puntos si están activos (para este órgano)
        if (showPoints) {
            glDisable(GL_LIGHTING);  // Desactiva iluminación para puntos
            glPointSize(1.0f);
            glBegin(GL_POINTS);
            for (const Point3D& p : om.mc.getPointCloud()) {
                glColor3f(om.color[0]*0.6f, om.color[1]*0.6f, om.color[2]*0.6f); // Versión oscura del color
                glVertex3f(p.x, p.y, p.z);
            }
            glEnd();
            glEnable(GL_LIGHTING);   // Reactiva iluminación
        }
        else glEnd();
    }
    
    glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
    switch(key) {
        case '+':
        case '=':
            zoom *= (1.0f - ZOOM_SPEED);
            break;
        case '-':
        case '_':
            zoom *= (1.0f + ZOOM_SPEED);
            break;
        case '0':
            zoom = 3.0f;  // Reset zoom
            rotX = rotY = 0;  // Reset rotation
            break;
        case 'p': // Tecla 'P' para toggle puntos
            showPoints = !showPoints;
            std::cout << "Puntos " << (showPoints ? "VISIBLES" : "OCULTOS") << std::endl;
            break;
        case 'i': printOrganInfo(); break;
        case 'v': 
            for (auto& om : organMeshes) {
                om.visible = !om.visible;
            }
            std::cout << "Todos los órganos " << (organMeshes[0].visible ? "visibles" : "ocultos") << std::endl;
            break;
        case '1'...'9':
        case 'a'...'g': { // Teclas 1-9 y a-g para 17 órganos
            int organId = (key >= 'a') ? (key - 'a' + 10) : (key - '1' + 1);
            if (organId <= totalOrgans) {
                organMeshes[organId-1].visible = !organMeshes[organId-1].visible;
                std::cout << "Órgano " << organId << " (" << organMeshes[organId-1].name 
                      << ") " << (organMeshes[organId-1].visible ? "visible" : "oculto") << std::endl;
            } else {
                std::cout << "No existe el órgano " << organId << std::endl;
            }
                break;
            }
        case 27: exit(0); break;
    }
    
    zoom = std::max(MIN_ZOOM, std::min(zoom, MAX_ZOOM));
    glutPostRedisplay();
}

// Función para manejar el redimensionamiento de la ventana
void reshape(int w, int h) {
    // Calcular límites globales de todos los órganos
    Point3D globalMin, globalMax;
    calculateGlobalBounds(globalMin, globalMax);

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    float aspect = (float)w / (float)h;
    
    // Calcular el tamaño de la escena basado en los límites globales
    float sceneSizeX = globalMax.x - globalMin.x;
    float sceneSizeY = globalMax.y - globalMin.y;
    float sceneSizeZ = globalMax.z - globalMin.z;
    float sceneSize = std::max({sceneSizeX, sceneSizeY, sceneSizeZ});
    
    // Ajustar planos near/far dinámicamente
    float nearPlane = 0.1f * sceneSize;  // 10% del tamaño de la escena
    float farPlane = 10.0f * sceneSize;  // 10 veces el tamaño para seguridad
    
    gluPerspective(45.0, aspect, nearPlane, farPlane);
    
    glMatrixMode(GL_MODELVIEW);
}

// Función para manejar eventos del mouse
void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            mouseLeftDown = true;
            lastX = x;
            lastY = y;
        } else if (state == GLUT_UP) {
            mouseLeftDown = false;
        }
    }
}

// Función para manejar el movimiento del mouse
void mouseMotion(int x, int y) {
    if (mouseLeftDown) {
        rotY += (x - lastX) * 0.5f;
        rotX += (y - lastY) * 0.5f;
        lastX = x;
        lastY = y;
        glutPostRedisplay();
    }
}

// Función principal
int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <directorio_organos> [isoValue] [gridResolution] [threads]\n"
                  << "Controles:\n"
                  << "  Teclas 1-9, a-g: Mostrar/ocultar órganos\n"
                  << "  v: Mostrar/ocultar todos\n"
                  << "  i: Mostrar información de órganos\n"
                  << "  +/-: Zoom\n"
                  << "  p: Visualizar puntos\n"
                  << "  0: Reset vista\n";
        return 1;
    }
    
    // Parámetros con valores por defecto
    float isoValue = (argc > 2) ? atof(argv[2]) : 0.05f;
    if (argc > 3) GRID_RESOLUTION = atoi(argv[3]);
    if (argc > 4) THREADS_TO_USE = atoi(argv[4]);
    
    // Cargar órganos
    loadOrganMeshes(argv[1], isoValue);
    
    // Calcular centroide global
    if (!organMeshes.empty()) {
        meshCentroid = Point3D(0, 0, 0);
        for (const auto& om : organMeshes) {
            Point3D c = om.mc.calculateCentroid();
            meshCentroid.x += c.x;
            meshCentroid.y += c.y;
            meshCentroid.z += c.z;
        }
        meshCentroid.x /= organMeshes.size();
        meshCentroid.y /= organMeshes.size();
        meshCentroid.z /= organMeshes.size();
    }

    Point3D globalMin, globalMax;
    calculateGlobalBounds(globalMin, globalMax);
    float sceneSize = std::max({
        globalMax.x - globalMin.x, 
        globalMax.y - globalMin.y, 
        globalMax.z - globalMin.z
    });
    zoom = sceneSize * 1.5f;

    // Inicializar GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("RENE TE ODIO");
    
    // Registrar callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(mouseMotion);
    //glutMouseWheelFunc(mouseWheel);
    glutKeyboardFunc(keyboard);
    
    initGL();
    glutMainLoop();
    
    return 0;
}