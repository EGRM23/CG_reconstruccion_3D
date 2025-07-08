#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <algorithm>
#include <set>

// OpenGL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// OpenCV
#include <opencv2/opencv.hpp>

struct Point3D {
    float x, y, z;
    Point3D(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
    
    bool operator<(const Point3D& other) const {
        if (x != other.x) return x < other.x;
        if (y != other.y) return y < other.y;
        return z < other.z;
    }

    bool operator==(const Point3D& other) const {
        const float EPSILON = 1e-6f;
        return std::abs(x - other.x) < EPSILON && 
               std::abs(y - other.y) < EPSILON && 
               std::abs(z - other.z) < EPSILON;
    }
};

struct Triangle {
    unsigned int v1, v2, v3;
    Triangle(unsigned int v1, unsigned int v2, unsigned int v3) : v1(v1), v2(v2), v3(v3) {}
};

struct Tetrahedron {
    int vertices[4];
    bool isValid;
    
    Tetrahedron(int a, int b, int c, int d) : isValid(true) {
        vertices[0] = a;
        vertices[1] = b;
        vertices[2] = c;
        vertices[3] = d;
    }
    
    bool contains(int vertex) const {
        for (int i = 0; i < 4; i++) {
            if (vertices[i] == vertex) return true;
        }
        return false;
    }
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
    
    Vertex(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 norm = glm::vec3(0.0f), glm::vec3 col = glm::vec3(1.0f))
        : position(pos), normal(norm), color(col) {}
};

class DelaunayTriangulator {
private:
    std::vector<Point3D> points;
    std::vector<Tetrahedron> tetrahedra;
    
    // Calcular el determinante 4x4 para el test de orientación
    float orient3d(const Point3D& a, const Point3D& b, const Point3D& c, const Point3D& d) {
        float ax = a.x, ay = a.y, az = a.z;
        float bx = b.x, by = b.y, bz = b.z;
        float cx = c.x, cy = c.y, cz = c.z;
        float dx = d.x, dy = d.y, dz = d.z;
        
        return (ax - dx) * ((by - dy) * (cz - dz) - (bz - dz) * (cy - dy))
             - (ay - dy) * ((bx - dx) * (cz - dz) - (bz - dz) * (cx - dx))
             + (az - dz) * ((bx - dx) * (cy - dy) - (by - dy) * (cx - dx));
    }
    
    // Test de in-sphere para verificar si un punto está dentro de la esfera circunscrita
    bool inSphere(const Point3D& a, const Point3D& b, const Point3D& c, const Point3D& d, const Point3D& e) {
        float ax = a.x, ay = a.y, az = a.z;
        float bx = b.x, by = b.y, bz = b.z;
        float cx = c.x, cy = c.y, cz = c.z;
        float dx = d.x, dy = d.y, dz = d.z;
        float ex = e.x, ey = e.y, ez = e.z;
        
        float aex = ax - ex, aey = ay - ey, aez = az - ez;
        float bex = bx - ex, bey = by - ey, bez = bz - ez;
        float cex = cx - ex, cey = cy - ey, cez = cz - ez;
        float dex = dx - ex, dey = dy - ey, dez = dz - ez;
        
        float aexbey = aex * bey, bexaey = bex * aey;
        float bexcey = bex * cey, cexbey = cex * bey;
        float cexdey = cex * dey, dexcey = dex * cey;
        float dexaey = dex * aey, aexdey = aex * dey;
        float aexcey = aex * cey, cexaey = cex * aey;
        float bexdey = bex * dey, dexbey = dex * bey;
        
        float ab = aexbey - bexaey;
        float bc = bexcey - cexbey;
        float cd = cexdey - dexcey;
        float da = dexaey - aexdey;
        float ac = aexcey - cexaey;
        float bd = bexdey - dexbey;
        
        float abc = aez * bc - bez * ac + cez * ab;
        float bcd = bez * cd - cez * bd + dez * bc;
        float cda = cez * da + dez * ac + aez * cd;
        float dab = dez * ab + aez * bd + bez * da;
        
        float alift = aex * aex + aey * aey + aez * aez;
        float blift = bex * bex + bey * bey + bez * bez;
        float clift = cex * cex + cey * cey + cez * cez;
        float dlift = dex * dex + dey * dey + dez * dez;
        
        return (dlift * abc - clift * dab + blift * cda - alift * bcd) > 0;
    }
    
    // Crear un super-tetraedro que contenga todos los puntos
    void createSuperTetrahedron() {
        // Encontrar bounding box
        float minX = points[0].x, maxX = points[0].x;
        float minY = points[0].y, maxY = points[0].y;
        float minZ = points[0].z, maxZ = points[0].z;
        
        for (const auto& p : points) {
            minX = std::min(minX, p.x);
            maxX = std::max(maxX, p.x);
            minY = std::min(minY, p.y);
            maxY = std::max(maxY, p.y);
            minZ = std::min(minZ, p.z);
            maxZ = std::max(maxZ, p.z);
        }
        
        float dx = maxX - minX;
        float dy = maxY - minY;
        float dz = maxZ - minZ;
        float deltaMax = std::max({dx, dy, dz});
        float midX = (minX + maxX) / 2.0f;
        float midY = (minY + maxY) / 2.0f;
        float midZ = (minZ + maxZ) / 2.0f;
        
        // Crear 4 puntos que formen un tetraedro grande
        float size = deltaMax * 20.0f;
        Point3D p1(midX - size, midY - size, midZ - size, -1);
        Point3D p2(midX + size, midY - size, midZ - size, -2);
        Point3D p3(midX, midY + size, midZ - size, -3);
        Point3D p4(midX, midY, midZ + size, -4);
        
        points.insert(points.begin(), {p1, p2, p3, p4});
        
        // Crear el super-tetraedro
        tetrahedra.push_back(Tetrahedron(0, 1, 2, 3));
    }
    
    // Obtener las caras de un tetraedro
    std::vector<std::vector<int>> getTetrahedronFaces(const Tetrahedron& tetra) {
        return {
            {tetra.vertices[0], tetra.vertices[1], tetra.vertices[2]},
            {tetra.vertices[0], tetra.vertices[1], tetra.vertices[3]},
            {tetra.vertices[0], tetra.vertices[2], tetra.vertices[3]},
            {tetra.vertices[1], tetra.vertices[2], tetra.vertices[3]}
        };
    }
    
public:
    void setPoints(const std::vector<Point3D>& inputPoints) {
        points = inputPoints;
        for (int i = 0; i < points.size(); i++) {
            points[i].id = i;
        }
    }
    
    void triangulate() {
        if (points.size() < 4) {
            std::cerr << "Se necesitan al menos 4 puntos para triangulación 3D" << std::endl;
            return;
        }
        
        std::cout << "Iniciando triangulación de Delaunay 3D..." << std::endl;
        
        // Crear super-tetraedro
        createSuperTetrahedron();
        
        // Insertar puntos uno por uno
        for (int i = 4; i < points.size(); i++) {
            std::vector<Tetrahedron> badTetrahedra;
            std::vector<std::vector<int>> boundary;
            
            // Encontrar tetraedros "malos" (que contienen el punto en su esfera circunscrita)
            for (auto& tetra : tetrahedra) {
                if (!tetra.isValid) continue;
                
                if (inSphere(points[tetra.vertices[0]], points[tetra.vertices[1]], 
                           points[tetra.vertices[2]], points[tetra.vertices[3]], points[i])) {
                    badTetrahedra.push_back(tetra);
                    tetra.isValid = false;
                }
            }
            
            // Encontrar el boundary (caras que no son compartidas)
            std::map<std::vector<int>, int> faceCount;
            for (const auto& tetra : badTetrahedra) {
                auto faces = getTetrahedronFaces(tetra);
                for (auto& face : faces) {
                    std::sort(face.begin(), face.end());
                    faceCount[face]++;
                }
            }
            
            for (const auto& pair : faceCount) {
                if (pair.second == 1) {
                    boundary.push_back(pair.first);
                }
            }
            
            // Crear nuevos tetraedros conectando el punto con cada cara del boundary
            for (const auto& face : boundary) {
                tetrahedra.push_back(Tetrahedron(face[0], face[1], face[2], i));
            }
        }
        
        // Remover tetraedros inválidos
        tetrahedra.erase(
            std::remove_if(tetrahedra.begin(), tetrahedra.end(),
                          [](const Tetrahedron& t) { return !t.isValid; }),
            tetrahedra.end()
        );
        
        std::cout << "Triangulación completada. Tetraedros: " << tetrahedra.size() << std::endl;
    }
    
    std::vector<Triangle> extractSurfaceTriangles() {
        std::vector<Triangle> surfaceTriangles;
        std::map<std::vector<int>, int> faceCount;
        
        // Contar cuántas veces aparece cada cara
        for (const auto& tetra : tetrahedra) {
            // Ignorar tetraedros que contienen vértices del super-tetraedro
            bool containsSuperVertex = false;
            for (int i = 0; i < 4; i++) {
                if (tetra.vertices[i] < 4) {
                    containsSuperVertex = true;
                    break;
                }
            }
            if (containsSuperVertex) continue;
            
            auto faces = getTetrahedronFaces(tetra);
            for (auto& face : faces) {
                std::sort(face.begin(), face.end());
                faceCount[face]++;
            }
        }
        
        // Las caras que aparecen solo una vez son parte de la superficie
        for (const auto& pair : faceCount) {
            if (pair.second == 1) {
                const auto& face = pair.first;
                // Ajustar índices (restar 4 porque agregamos 4 vértices del super-tetraedro)
                if (face[0] >= 4 && face[1] >= 4 && face[2] >= 4) {
                    surfaceTriangles.push_back(Triangle(face[0] - 4, face[1] - 4, face[2] - 4));
                }
            }
        }
        
        std::cout << "Triángulos de superficie extraídos: " << surfaceTriangles.size() << std::endl;
        return surfaceTriangles;
    }
    
    const std::vector<Point3D>& getPoints() const {
        return points;
    }
};

class MeshVisualizer {
private:
    // OpenGL variables
    GLFWwindow* window;
    unsigned int shaderProgram;
    unsigned int VAO, VBO, EBO;
    
    // Mesh data
    std::vector<Point3D> points;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Triangle> triangles;
    DelaunayTriangulator triangulator;
    
    // Camera and view variables
    glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 100.0f);
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    float cameraSpeed = 50.0f;
    float mouseSensitivity = 0.1f;
    
    // Mouse and keyboard
    bool firstMouse = true;
     bool leftMousePressed = false;
    float lastX = 400.0f;
    float lastY = 300.0f;
    float yaw = -90.0f;
    float pitch = 0.0f;
    
    // Rendering options
    bool wireframe = false;
    bool showPoints = true;
    bool showMesh = false;
    
    // Mesh bounds
    Point3D minBounds, maxBounds;
    
    // Vertex shader source
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec3 aColor;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        out vec3 FragPos;
        out vec3 Normal;
        out vec3 Color;
        
        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            Color = aColor;
            
            gl_Position = projection * view * vec4(FragPos, 1.0);
        }
    )";
    
    // Fragment shader source
    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        
        in vec3 FragPos;
        in vec3 Normal;
        in vec3 Color;
        
        uniform vec3 lightPos;
        uniform vec3 viewPos;
        uniform vec3 lightColor;
        
        void main() {
            // Ambient
            float ambientStrength = 0.3;
            vec3 ambient = ambientStrength * lightColor;
            
            // Diffuse
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;
            
            // Specular
            float specularStrength = 0.5;
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            vec3 specular = specularStrength * spec * lightColor;
            
            vec3 result = (ambient + diffuse + specular) * Color;
            FragColor = vec4(result, 1.0);
        }
    )";
    
    // Callback functions
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
    }
    
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
        MeshVisualizer* visualizer = static_cast<MeshVisualizer*>(glfwGetWindowUserPointer(window));
        visualizer->processMouse(xpos, ypos);
    }
    
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
        MeshVisualizer* visualizer = static_cast<MeshVisualizer*>(glfwGetWindowUserPointer(window));
        visualizer->processMouseButton(button, action, mods);
    }

    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        MeshVisualizer* visualizer = static_cast<MeshVisualizer*>(glfwGetWindowUserPointer(window));
        visualizer->processKeyboard(key, scancode, action, mods);
    }

    void processMouseButton(int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                leftMousePressed = true;
                firstMouse = true;  // Resetear firstMouse cuando se presiona
            } else if (action == GLFW_RELEASE) {
                leftMousePressed = false;
            }
        }
    }
    
    void processMouse(double xpos, double ypos) {
        if (!leftMousePressed) return;  // Solo procesar si el botón está presionado
        
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
            return;  // No calcular rotación en el primer frame
        }
        
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;
        lastX = xpos;
        lastY = ypos;
        
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;
        
        yaw += xoffset;
        pitch += yoffset;
        
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
        
        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(direction);
    }
    
    void processKeyboard(int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS) {
            switch (key) {
                case GLFW_KEY_W:
                    wireframe = !wireframe;
                    if (wireframe) {
                        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                        std::cout << "Wireframe ON" << std::endl;
                    } else {
                        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                        std::cout << "Wireframe OFF" << std::endl;
                    }
                    break;
                case GLFW_KEY_P:
                    showPoints = !showPoints;
                    std::cout << "Points: " << (showPoints ? "ON" : "OFF") << std::endl;
                    break;
                case GLFW_KEY_M:
                    showMesh = !showMesh;
                    std::cout << "Mesh: " << (showMesh ? "ON" : "OFF") << std::endl;
                    break;
                case GLFW_KEY_R:
                    resetCamera();
                    break;
                case GLFW_KEY_ESCAPE:
                    glfwSetWindowShouldClose(window, true);
                    break;
            }
        }
    }
    
    void resetCamera() {
        // Centrar la cámara en el objeto
        Point3D center = {
            (minBounds.x + maxBounds.x) / 2.0f,
            (minBounds.y + maxBounds.y) / 2.0f,
            (minBounds.z + maxBounds.z) / 2.0f
        };
        
        float maxDim = std::max({
            maxBounds.x - minBounds.x,
            maxBounds.y - minBounds.y,
            maxBounds.z - minBounds.z
        });
        
        cameraPos = glm::vec3(center.x, center.y, center.z + maxDim * 2.0f);
        cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
        yaw = -90.0f;
        pitch = 0.0f;
        
        std::cout << "Camera reset. Center: (" << center.x << ", " << center.y << ", " << center.z << ")" << std::endl;
    }
    
    unsigned int compileShader(unsigned int type, const char* source) {
        unsigned int shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, NULL);
        glCompileShader(shader);
        
        int success;
        char infoLog[512];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            std::cerr << "Shader compilation error: " << infoLog << std::endl;
        }
        
        return shader;
    }
    
    void createShaderProgram() {
        unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
        unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
        
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        
        int success;
        char infoLog[512];
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
            std::cerr << "Shader program linking error: " << infoLog << std::endl;
        }
        
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }
    
    void calculateBounds() {
        if (points.empty()) return;
        
        minBounds = maxBounds = points[0];
        
        for (const auto& point : points) {
            minBounds.x = std::min(minBounds.x, point.x);
            minBounds.y = std::min(minBounds.y, point.y);
            minBounds.z = std::min(minBounds.z, point.z);
            
            maxBounds.x = std::max(maxBounds.x, point.x);
            maxBounds.y = std::max(maxBounds.y, point.y);
            maxBounds.z = std::max(maxBounds.z, point.z);
        }
    }
    
    glm::vec3 calculateNormal(const Point3D& p1, const Point3D& p2, const Point3D& p3) {
        glm::vec3 v1(p2.x - p1.x, p2.y - p1.y, p2.z - p1.z);
        glm::vec3 v2(p3.x - p1.x, p3.y - p1.y, p3.z - p1.z);
        return glm::normalize(glm::cross(v1, v2));
    }
    
    glm::vec3 heightToColor(float z) {
        // Mapear altura a color (azul -> verde -> rojo)
        float normalizedZ = (z - minBounds.z) / (maxBounds.z - minBounds.z);
        
        if (normalizedZ < 0.5f) {
            // Azul a verde
            float t = normalizedZ * 2.0f;
            return glm::vec3(0.0f, t, 1.0f - t);
        } else {
            // Verde a rojo
            float t = (normalizedZ - 0.5f) * 2.0f;
            return glm::vec3(t, 1.0f - t, 0.0f);
        }
    }
    
public:
    bool initializeOpenGL() {
        // Inicializar GLFW
        if (!glfwInit()) {
            std::cerr << "Error inicializando GLFW" << std::endl;
            return false;
        }
        
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
        // Crear ventana
        window = glfwCreateWindow(1200, 800, "Visualizador de Malla 3D", NULL, NULL);
        if (!window) {
            std::cerr << "Error creando ventana GLFW" << std::endl;
            glfwTerminate();
            return false;
        }
        
        glfwMakeContextCurrent(window);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwSetCursorPosCallback(window, mouse_callback);
        glfwSetMouseButtonCallback(window, mouse_button_callback);
        glfwSetKeyCallback(window, key_callback);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        
        // Inicializar GLEW
        if (glewInit() != GLEW_OK) {
            std::cerr << "Error inicializando GLEW" << std::endl;
            return false;
        }
        
        // Configurar OpenGL
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        
        // Crear programa de shaders
        createShaderProgram();
        
        // Generar buffers
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        
        std::cout << "OpenGL inicializado correctamente" << std::endl;
        return true;
    }
    
    bool loadPointsFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error abriendo archivo: " << filename << std::endl;
            return false;
        }
        
        points.clear();
        std::string line;
        
        // Detectar formato del archivo
        bool isPLY = (filename.substr(filename.find_last_of('.') + 1) == "ply");
        bool headerPassed = false;
        
        while (std::getline(file, line)) {
            if (isPLY && !headerPassed) {
                if (line == "end_header") {
                    headerPassed = true;
                }
                continue;
            }
            
            if (line.empty() || line[0] == '#') continue;
            
            std::istringstream iss(line);
            float x, y, z;
            if (iss >> x >> y >> z) {
                points.push_back(Point3D(x, y, z));
            }
        }
        
        file.close();
        
        std::cout << "Puntos cargados: " << points.size() << std::endl;
        calculateBounds();
        
        return !points.empty();
    }
    
    void generateMeshFromPoints() {
        if (points.size() < 3) {
            std::cerr << "Se necesitan al menos 4 puntos para generar malla" << std::endl;
            return;
        }
        
        vertices.clear();
        indices.clear();
        
        // Crear vértices con colores basados en altura
        for (const auto& point : points) {
            glm::vec3 position(point.x, point.y, point.z);
            glm::vec3 normal(0.0f, 0.0f, 1.0f); // Se calculará después
            glm::vec3 color = heightToColor(point.z);
            
            vertices.push_back(Vertex(position, normal, color));
        }
        
        // Generar triangulación simple basada en capas Z
        std::cout << "Generando malla..." << std::endl;
        
        // Agrupar puntos por capa Z
        std::map<int, std::vector<int>> layers;
        for (int i = 0; i < points.size(); i++) {
            int layer = static_cast<int>(std::round(points[i].z));
            layers[layer].push_back(i);
        }
        
        // // Crear triángulos entre capas adyacentes
        // auto layerIt = layers.begin();
        // auto nextLayerIt = std::next(layerIt);
        
        // while (nextLayerIt != layers.end()) {
        //     const auto& currentLayer = layerIt->second;
        //     const auto& nextLayer = nextLayerIt->second;
            
        //     // Triangulación simple entre capas
        //     for (int i = 0; i < currentLayer.size() && i < nextLayer.size(); i++) {
        //         int idx1 = currentLayer[i];
        //         int idx2 = nextLayer[i];
                
        //         // Buscar puntos cercanos para formar triángulos
        //         for (int j = i + 1; j < currentLayer.size() && j < nextLayer.size(); j++) {
        //             int idx3 = currentLayer[j];
        //             int idx4 = nextLayer[j];
                    
        //             // Crear dos triángulos
        //             indices.push_back(idx1);
        //             indices.push_back(idx2);
        //             indices.push_back(idx3);
                    
        //             indices.push_back(idx2);
        //             indices.push_back(idx4);
        //             indices.push_back(idx3);
        //         }
        //     }
            
        //     ++layerIt;
        //     ++nextLayerIt;
        // }
        
        // // Calcular normales
        // std::vector<glm::vec3> vertexNormals(vertices.size(), glm::vec3(0.0f));
        
        // for (int i = 0; i < indices.size(); i += 3) {
        //     int idx1 = indices[i];
        //     int idx2 = indices[i + 1];
        //     int idx3 = indices[i + 2];
            
        //     glm::vec3 normal = calculateNormal(
        //         Point3D(vertices[idx1].position.x, vertices[idx1].position.y, vertices[idx1].position.z),
        //         Point3D(vertices[idx2].position.x, vertices[idx2].position.y, vertices[idx2].position.z),
        //         Point3D(vertices[idx3].position.x, vertices[idx3].position.y, vertices[idx3].position.z)
        //     );
            
        //     vertexNormals[idx1] += normal;
        //     vertexNormals[idx2] += normal;
        //     vertexNormals[idx3] += normal;
        // }
        
        // // Normalizar las normales
        // for (int i = 0; i < vertexNormals.size(); i++) {
        //     vertices[i].normal = glm::normalize(vertexNormals[i]);
        // }
        
        std::cout << "Malla generada: " << vertices.size() << " vértices, " << indices.size() / 3 << " triángulos" << std::endl;
        
        // Subir datos a GPU
        uploadMeshToGPU();
    }
    
    void uploadMeshToGPU() {
        glBindVertexArray(VAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        
        // Posición
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);
        
        // Normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(1);
        
        // Color
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
        glEnableVertexAttribArray(2);
        
        glBindVertexArray(0);
    }
    
    void render() {
        resetCamera();
        
        float deltaTime = 0.0f;
        float lastFrame = 0.0f;
        
        while (!glfwWindowShouldClose(window)) {
            float currentFrame = glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;
            
            // Procesar input de movimiento
            processInput(deltaTime);
            
            // Limpiar pantalla
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            // Usar programa de shaders
            glUseProgram(shaderProgram);
            
            // Matrices de transformación
            glm::mat4 model = glm::mat4(1.0f);
            glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
            glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1200.0f / 800.0f, 0.1f, 10000.0f);
            
            // Uniforms
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            
            // Lighting
            glm::vec3 lightPos = cameraPos + glm::vec3(100.0f, 100.0f, 100.0f);
            glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
            glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(cameraPos));
            glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
            
            // Renderizar malla
            glBindVertexArray(VAO);
            
            if (showMesh && !indices.empty()) {
                glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            }
            
            if (showPoints) {
                glPointSize(2.0f);
                glDrawArrays(GL_POINTS, 0, vertices.size());
            }
            
            // Intercambiar buffers
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }
    
    void processInput(float deltaTime) {
        float velocity = cameraSpeed * deltaTime;
        
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            cameraPos += velocity * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            cameraPos -= velocity * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            cameraPos += velocity * cameraUp;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            cameraPos -= velocity * cameraUp;
    }
    
    void printControls() {
        std::cout << "\n=== CONTROLES ===" << std::endl;
        std::cout << "Mouse: Rotar cámara" << std::endl;
        std::cout << "Flechas: Mover cámara" << std::endl;
        std::cout << "Espacio: Subir" << std::endl;
        std::cout << "Shift: Bajar" << std::endl;
        std::cout << "W: Toggle wireframe" << std::endl;
        std::cout << "P: Toggle puntos" << std::endl;
        std::cout << "M: Toggle malla" << std::endl;
        std::cout << "R: Reset cámara" << std::endl;
        std::cout << "ESC: Salir" << std::endl;
    }
    
    ~MeshVisualizer() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteProgram(shaderProgram);
        glfwTerminate();
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Uso: " << argv[0] << " <archivo_puntos>" << std::endl;
        std::cout << "Formatos soportados: .ply, .xyz, .pcd" << std::endl;
        return -1;
    }
    
    MeshVisualizer visualizer;
    
    // Inicializar OpenGL
    if (!visualizer.initializeOpenGL()) {
        std::cerr << "Error inicializando OpenGL" << std::endl;
        return -1;
    }
    
    // Cargar puntos
    if (!visualizer.loadPointsFromFile(argv[1])) {
        std::cerr << "Error cargando archivo de puntos" << std::endl;
        return -1;
    }
    
    // Generar malla
    visualizer.generateMeshFromPoints();
    
    // Mostrar controles
    visualizer.printControls();
    
    // Renderizar
    visualizer.render();
    
    return 0;
}