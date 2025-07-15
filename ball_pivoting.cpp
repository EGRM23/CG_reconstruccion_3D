#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <set>
#include <map>
#include <queue>
#include <GL/glut.h>

using namespace std;

struct Point3D {
    double x, y, z;
    Point3D(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
    
    Point3D operator+(const Point3D& other) const {
        return Point3D(x + other.x, y + other.y, z + other.z);
    }
    
    Point3D operator-(const Point3D& other) const {
        return Point3D(x - other.x, y - other.y, z - other.z);
    }
    
    Point3D operator*(double scalar) const {
        return Point3D(x * scalar, y * scalar, z * scalar);
    }
    
    double dot(const Point3D& other) const {
        return x * other.x + y * other.y + z * other.z;
    }
    
    Point3D cross(const Point3D& other) const {
        return Point3D(y * other.z - z * other.y,
                       z * other.x - x * other.z,
                       x * other.y - y * other.x);
    }
    
    double length() const {
        return sqrt(x * x + y * y + z * z);
    }
    
    Point3D normalize() const {
        double len = length();
        if (len > 0) return Point3D(x / len, y / len, z / len);
        return Point3D(0, 0, 0);
    }
};

struct Triangle {
    int v1, v2, v3;
    Triangle(int a, int b, int c) : v1(a), v2(b), v3(c) {}
};

struct Edge {
    int v1, v2;
    Edge(int a, int b) : v1(std::min(a, b)), v2(std::max(a, b)) {}
    
    bool operator<(const Edge& other) const {
        if (v1 != other.v1) return v1 < other.v1;
        return v2 < other.v2;
    }
};

class BallPivoting {
private:
    std::vector<Point3D> points;
    std::vector<Triangle> triangles;
    std::set<Edge> activeEdges;
    std::set<Edge> processedEdges;
    double ballRadius;
    
    double distance(const Point3D& a, const Point3D& b) {
        return (a - b).length();
    }
    
    // Encuentra el centro de la esfera que pasa por tres puntos
    Point3D findCircumcenter(const Point3D& a, const Point3D& b, const Point3D& c) {
        Point3D ab = b - a;
        Point3D ac = c - a;
        Point3D normal = ab.cross(ac);
        
        if (normal.length() < 1e-10) return Point3D(0, 0, 0); // Puntos colineales
        
        double d = 2 * (ab.x * ac.y - ab.y * ac.x);
        if (std::abs(d) < 1e-10) return Point3D(0, 0, 0);
        
        double ux = (ac.y * (ab.x * ab.x + ab.y * ab.y) - ab.y * (ac.x * ac.x + ac.y * ac.y)) / d;
        double uy = (ab.x * (ac.x * ac.x + ac.y * ac.y) - ac.x * (ab.x * ab.x + ab.y * ab.y)) / d;
        
        return Point3D(a.x + ux, a.y + uy, a.z);
    }
    
    // Verifica si un punto está dentro de la esfera
    bool isInsideBall(const Point3D& center, const Point3D& point) {
        return distance(center, point) <= ballRadius + 1e-10;
    }
    
    // Encuentra el punto candidato para formar un triángulo con una arista
    int findBestCandidate(const Edge& edge) {
        Point3D p1 = points[edge.v1];
        Point3D p2 = points[edge.v2];
        
        int bestCandidate = -1;
        double minAngle = M_PI;
        
        for (size_t i = 0; i < points.size(); i++) {
            if (i == edge.v1 || i == edge.v2) continue;
            
            Point3D p3 = points[i];
            
            // Verifica si los tres puntos pueden formar un triángulo válido
            Point3D center = findCircumcenter(p1, p2, p3);
            if (center.x == 0 && center.y == 0 && center.z == 0) continue;
            
            double radius = distance(center, p1);
            if (radius > ballRadius + 1e-10) continue;
            
            // Verifica que no haya otros puntos dentro de la esfera
            bool valid = true;
            for (size_t j = 0; j < points.size(); j++) {
                if (j == edge.v1 || j == edge.v2 || j == i) continue;
                if (isInsideBall(center, points[j])) {
                    valid = false;
                    break;
                }
            }
            
            if (valid) {
                // Calcula el ángulo para encontrar el mejor candidato
                Point3D v1 = (p1 - p3).normalize();
                Point3D v2 = (p2 - p3).normalize();
                double angle = acos(std::max(-1.0, std::min(1.0, v1.dot(v2))));
                
                if (angle < minAngle) {
                    minAngle = angle;
                    bestCandidate = i;
                }
            }
        }
        
        return bestCandidate;
    }
    
    // Añade las aristas de un triángulo a la lista de aristas activas
    void addEdgesToActive(const Triangle& triangle) {
        Edge e1(triangle.v1, triangle.v2);
        Edge e2(triangle.v2, triangle.v3);
        Edge e3(triangle.v3, triangle.v1);
        
        if (processedEdges.find(e1) == processedEdges.end()) {
            activeEdges.insert(e1);
        }
        if (processedEdges.find(e2) == processedEdges.end()) {
            activeEdges.insert(e2);
        }
        if (processedEdges.find(e3) == processedEdges.end()) {
            activeEdges.insert(e3);
        }
    }
    
    // Encuentra el triángulo inicial (seed triangle)
    bool findSeedTriangle() {
        size_t n_point = points.size();
        double avg_radius = 0.0;
        for (size_t i = 0; i < points.size(); i++) {
            cout << "Progress "<< i+1 << "/" << n_point << endl;
            for (size_t j = i + 1; j < points.size(); j++) {
                for (size_t k = j + 1; k < points.size(); k++) {
                    Point3D p1 = points[i];
                    Point3D p2 = points[j];
                    Point3D p3 = points[k];
                    
                    Point3D center = findCircumcenter(p1, p2, p3);
                    if (center.x == 0 && center.y == 0 && center.z == 0) continue;
                    
                    double radius = distance(center, p1);
                    avg_radius += radius;
                    if (radius > ballRadius + 1e-10) continue;
                    
                    // Verifica que no haya otros puntos dentro de la esfera
                    bool valid = true;
                    for (size_t l = 0; l < points.size(); l++) {
                        if (l == i || l == j || l == k) continue;
                        if (isInsideBall(center, points[l])) {
                            valid = false;
                            break;
                        }
                    }
                    
                    if (valid) {
                        Triangle seedTriangle(i, j, k);
                        triangles.push_back(seedTriangle);
                        addEdgesToActive(seedTriangle);
                        return true;
                    }
                }
            }
            cout << "\t AVG Radio: " << avg_radius/n_point << endl;
        }
        return false;
    }
    
public:
    BallPivoting(double radius) : ballRadius(radius) {}
    
    void setPoints(const std::vector<Point3D>& pts) {
        points = pts;
    }
    
    void generateMesh() {
        if (points.size() < 3) {
            std::cerr << "No hay suficientes puntos para generar una malla" << std::endl;
            return;
        }
        cerr << "!Si hay puntos... READY!" << endl;
        
        // Encuentra el triángulo inicial
        if (!findSeedTriangle()) {
            std::cerr << "No se pudo encontrar un triángulo inicial" << std::endl;
            return;
        }
        
        cerr << "!Triangulo Inicial READY!." << endl;


        // Algoritmo principal de Ball Pivoting
        while (!activeEdges.empty()) {
            Edge currentEdge = *activeEdges.begin();
            activeEdges.erase(activeEdges.begin());
            processedEdges.insert(currentEdge);
            
            int candidate = findBestCandidate(currentEdge);
            if (candidate != -1) {
                Triangle newTriangle(currentEdge.v1, currentEdge.v2, candidate);
                triangles.push_back(newTriangle);
                addEdgesToActive(newTriangle);
            }
        }
        
        std::cout << "Malla generada con " << triangles.size() << " triángulos" << std::endl;
    }
    
    const std::vector<Triangle>& getTriangles() const {
        return triangles;
    }
    
    const std::vector<Point3D>& getPoints() const {
        return points;
    }
};

// Variables globales para OpenGL
BallPivoting* bpa;
float rotationX = 0.0f;
float rotationY = 0.0f;
float scale = 1.0f;
bool wireframe = true;

// Función para cargar puntos desde archivo XYZ
bool loadPointsFromXYZ(const std::string& filename, std::vector<Point3D>& points) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo " << filename << std::endl;
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        double x, y, z;
        if (iss >> x >> y >> z) {
            points.push_back(Point3D(x, y, z));
        }
    }
    
    file.close();
    cout << "Cargados " << points.size() << " puntos desde " << filename << std::endl;
    return true;
}

Point3D computeCentroid(const std::vector<Point3D>& points) {
    Point3D centroid(0.0f, 0.0f, 0.0f);
    for (const Point3D& p : points) {
        centroid.x += p.x;
        centroid.y += p.y;
        centroid.z += p.z;
    }
    if (!points.empty()) {
        centroid.x /= points.size();
        centroid.y /= points.size();
        centroid.z /= points.size();
    }
    return centroid;
}


// Funciones de OpenGL
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    glTranslatef(0.0f, 0.0f, -5.0f);
    glScalef(scale, scale, scale);
    glRotatef(rotationX, 1.0f, 0.0f, 0.0f);
    glRotatef(rotationY, 0.0f, 1.0f, 0.0f);
    
    
    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    // Dibuja la malla
    const std::vector<Triangle>& triangles = bpa->getTriangles();
    const std::vector<Point3D>& points = bpa->getPoints();
    
    Point3D centroid = computeCentroid(points);

    glBegin(GL_TRIANGLES);
    glColor3f(0.7f, 0.7f, 0.9f);
        
    for (const Triangle& tri : triangles) {
        Point3D p1 = points[tri.v1] - centroid;
        Point3D p2 = points[tri.v2] - centroid;
        Point3D p3 = points[tri.v3] - centroid;

        Point3D normal = ((p2 - p1).cross(p3 - p1)).normalize();
        glNormal3f(normal.x, normal.y, normal.z);

        glVertex3f(p1.x, p1.y, p1.z);
        glVertex3f(p2.x, p2.y, p2.z);
        glVertex3f(p3.x, p3.y, p3.z);
    }
    glEnd();
    
    // Dibuja los puntos originales
    glPointSize(3.0f);
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_POINTS);
    for (const Point3D& point : points) {
        Point3D centered = point - centroid;
        glVertex3f(centered.x, centered.y, centered.z);
    }
    glEnd();

    
    glutSwapBuffers();
}

void reshape(int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)width / height, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 'w':
        case 'W':
            wireframe = !wireframe;
            break;
        case '+':
            scale *= 1.1f;
            break;
        case '-':
            scale /= 1.1f;
            break;
        case 'q':
        case 'Q':
        case 27: // ESC
            exit(0);
            break;
    }
    glutPostRedisplay();
}

void specialKeys(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_UP:
            rotationX -= 5.0f;
            break;
        case GLUT_KEY_DOWN:
            rotationX += 5.0f;
            break;
        case GLUT_KEY_LEFT:
            rotationY -= 5.0f;
            break;
        case GLUT_KEY_RIGHT:
            rotationY += 5.0f;
            break;
    }
    glutPostRedisplay();
}

void initGL() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    GLfloat lightPos[] = {1.0f, 1.0f, 1.0f, 0.0f};
    GLfloat lightAmbient[] = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat lightDiffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
    
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " <archivo.xyz> <radio_bola>" << std::endl;
        return 1;
    }
    
    std::string filename = argv[1];
    double ballRadius = std::stod(argv[2]);
    
    // Carga los puntos desde el archivo XYZ
    std::vector<Point3D> points;
    if (!loadPointsFromXYZ(filename, points)) {
        return 1;
    }
    cout<<"Puntos Cargados Generando la visualización: "<<endl; 
    // Crea el objeto BallPivoting y genera la malla
    bpa = new BallPivoting(ballRadius);
    bpa->setPoints(points);
    bpa->generateMesh();
    
    // Inicializa OpenGL
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Ball Pivoting Algorithm - Visualización");
    
    initGL();
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    
    std::cout << "Controles:" << std::endl;
    std::cout << "  Flechas: Rotar modelo" << std::endl;
    std::cout << "  +/-: Zoom in/out" << std::endl;
    std::cout << "  W: Alternar wireframe" << std::endl;
    std::cout << "  Q/ESC: Salir" << std::endl;
    
    glutMainLoop();
    
    delete bpa;
    return 0;
}