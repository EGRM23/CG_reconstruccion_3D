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
};

struct Triangle {
    unsigned int v1, v2, v3;
    Triangle(unsigned int v1, unsigned int v2, unsigned int v3) : v1(v1), v2(v2), v3(v3) {}
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
    
    Vertex(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 norm = glm::vec3(0.0f), glm::vec3 col = glm::vec3(1.0f))
        : position(pos), normal(norm), color(col) {}
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
            std::cerr << "Se necesitan al menos 3 puntos para generar malla" << std::endl;
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