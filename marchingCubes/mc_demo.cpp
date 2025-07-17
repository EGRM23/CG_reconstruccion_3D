// marching_cubes_demo.cpp
#include <GL/glut.h>
#include <vector>
#include <array>
#include <cmath>
#include <iostream>
#include "marching_cubes_tables.h"

using namespace std;

// ——— Parámetros de la rejilla y umbral ———
const int NX = 10, NY = 10, NZ = 10;
const float ISO_LEVEL = 0.8f;
float angleX = 0.0f, angleY = 0.0f;
int lastMouseX = 0, lastMouseY = 0;
bool rotating = false;

// Almacena los triángulos generados
struct Tri { array<float,3> v0, v1, v2; };
vector<Tri> triangles;

// Función que interpola el punto sobre la arista
array<float,3> vertexInterp(float isolevel, const array<float,3>& p1, const array<float,3>& p2, float valp1, float valp2) {
    if (fabs(isolevel - valp1) < 1e-5) return p1;
    if (fabs(isolevel - valp2) < 1e-5) return p2;
    if (fabs(valp1 - valp2) < 1e-5) return p1;
    float mu = (isolevel - valp1) / (valp2 - valp1);
    return { p1[0] + mu*(p2[0]-p1[0]), p1[1] + mu*(p2[1]-p1[1]), p1[2] + mu*(p2[2]-p1[2]) };
}

// Ejecuta Marching Cubes sobre la rejilla
void marchingCubes(const vector<float>& grid, int nx, int ny, int nz, float iso) {
    triangles.clear();
    auto idx = [=](int x,int y,int z){ return x + nx*(y + ny*z); };

    for(int z=0; z<nz-1; ++z) for(int y=0; y<ny-1; ++y) for(int x=0; x<nx-1; ++x) {
        // Corners
        array<array<float,3>,8> p;
        array<float,8> val;
        for(int i=0;i<8;++i) {
            int dx = i&1, dy=(i>>1)&1, dz=(i>>2)&1;
            p[i] = {float(x+dx), float(y+dy), float(z+dz)};
            val[i] = grid[idx(x+dx,y+dy,z+dz)];
        }
        // Índice del cubo
        int cubeIndex = 0;
        for(int i=0;i<8;++i) if (val[i] < iso) cubeIndex |= (1<<i);
        if (edgeTable[cubeIndex] == 0) continue;

        // Calcula vértices en las aristas
        array<array<float,3>,12> vertList;
        if (edgeTable[cubeIndex] & 1)  vertList[0]  = vertexInterp(iso, p[0], p[1], val[0], val[1]);
        if (edgeTable[cubeIndex] & 2)  vertList[1]  = vertexInterp(iso, p[1], p[2], val[1], val[2]);
        if (edgeTable[cubeIndex] & 4)  vertList[2]  = vertexInterp(iso, p[2], p[3], val[2], val[3]);
        if (edgeTable[cubeIndex] & 8)  vertList[3]  = vertexInterp(iso, p[3], p[0], val[3], val[0]);
        if (edgeTable[cubeIndex] & 16) vertList[4]  = vertexInterp(iso, p[4], p[5], val[4], val[5]);
        if (edgeTable[cubeIndex] & 32) vertList[5]  = vertexInterp(iso, p[5], p[6], val[5], val[6]);
        if (edgeTable[cubeIndex] & 64) vertList[6]  = vertexInterp(iso, p[6], p[7], val[6], val[7]);
        if (edgeTable[cubeIndex] & 128)vertList[7]  = vertexInterp(iso, p[7], p[4], val[7], val[4]);
        if (edgeTable[cubeIndex] & 256)vertList[8]  = vertexInterp(iso, p[0], p[4], val[0], val[4]);
        if (edgeTable[cubeIndex] & 512)vertList[9]  = vertexInterp(iso, p[1], p[5], val[1], val[5]);
        if (edgeTable[cubeIndex] & 1024)vertList[10]= vertexInterp(iso, p[2], p[6], val[2], val[6]);
        if (edgeTable[cubeIndex] & 2048)vertList[11]= vertexInterp(iso, p[3], p[7], val[3], val[7]);

        // Construye los triángulos
        for(int i=0; triTable[cubeIndex][i] != -1; i += 3) {
            triangles.push_back({ vertList[ triTable[cubeIndex][i]   ],
                                  vertList[ triTable[cubeIndex][i+1] ],
                                  vertList[ triTable[cubeIndex][i+2] ] });
        }
    }
}

// Genera la función escalar en la rejilla: esfera centrada
vector<float> generateField(int nx, int ny, int nz) {
    vector<float> f(nx*ny*nz);
    float cx = (nx-1)/2.0f, cy=(ny-1)/2.0f, cz=(nz-1)/2.0f;
    float R = min(min(cx, cy), cz) * 0.8f;
    for(int z=0; z<nz; ++z) for(int y=0; y<ny; ++y) for(int x=0; x<nx; ++x) {
        float dx = x-cx, dy = y-cy, dz = z-cz;
        f[x + nx*(y + ny*z)] = dx*dx + dy*dy + dz*dz - R*R;
    }
    return f;
}

// ——— OpenGL / GLUT ———
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix();
        glTranslatef(0, 0, -10);              // Aleja la vista
        glRotatef(angleX, 1, 0, 0);           // Rotación en X
        glRotatef(angleY, 0, 1, 0);           // Rotación en Y
        glTranslatef(-NX/2.0f, -NY/2.0f, -NZ/2.0f);  // Centrar objeto
        glBegin(GL_TRIANGLES);
            for(auto &t : triangles) {
            // color simple según normal aproximada
            array<float,3> u, v, n;
            for(int i=0;i<3;++i) {
                u[i] = t.v1[i] - t.v0[i];
                v[i] = t.v2[i] - t.v0[i];
            }
            n = { u[1]*v[2] - u[2]*v[1],
                    u[2]*v[0] - u[0]*v[2],
                    u[0]*v[1] - u[1]*v[0] };
            float len = std::sqrt(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]) + 1e-6f;
            glColor3f(fabs(n[0]/len), fabs(n[1]/len), fabs(n[2]/len));
            glVertex3fv(t.v0.data());
            glVertex3fv(t.v1.data());
            glVertex3fv(t.v2.data());
            }
        glEnd();
        glPointSize(4.0f);
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_POINTS);
        for(auto &t : triangles) {
            glVertex3f(t.v0[0], t.v0[1], t.v0[2]);
            glVertex3f(t.v1[0], t.v1[1], t.v1[2]);
            glVertex3f(t.v2[0], t.v2[1], t.v2[2]);
            
        }
        glEnd();
    glPopMatrix();
    // Dibujar puntos de la rejilla en blanco

    glutSwapBuffers();
}

void reshape(int w,int h) {
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, w/(float)h, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(5,5,10, 0,0,0, 0,1,0);
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON)
        rotating = (state == GLUT_DOWN);
    lastMouseX = x;
    lastMouseY = y;
}

void motion(int x, int y) {
    if (rotating) {
        angleY += (x - lastMouseX);
        angleX += (y - lastMouseY);
        lastMouseX = x;
        lastMouseY = y;
        glutPostRedisplay();
    }
}

int main(int argc, char** argv) {
    // Genera campo y triángulos
    auto field = generateField(NX,NY,NZ);
    for(auto&p:field){
        cout << "- " << p << endl;
    }
    marchingCubes(field, NX, NY, NZ, ISO_LEVEL);

    // Inicializa GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800,600);
    glutCreateWindow("Marching Cubes Demo");
    glEnable(GL_DEPTH_TEST);
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    glutMainLoop();
    return 0;
}