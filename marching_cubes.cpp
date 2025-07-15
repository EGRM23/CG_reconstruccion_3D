#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tiffio.h>
#include <iostream>
#include <vector>
#include <array>
#include <cmath>

// Estructura para almacenar vértices
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

// Estructura para almacenar triángulos
struct Triangle {
    std::array<Vertex, 3> vertices;
};

// Tabla de configuraciones para Marching Cubes
// Cada entrada indica qué aristas del cubo están intersectadas
static const int edgeTable[256] = {
    0x0, 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
    0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
    0x190, 0x99, 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
    0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
    0x230, 0x339, 0x33, 0x13a, 0x636, 0x73f, 0x435, 0x53c,
    0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
    0x3a0, 0x2a9, 0x1a3, 0xaa, 0x7a6, 0x6af, 0x5a5, 0x4ac,
    0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
    0x460, 0x569, 0x663, 0x76a, 0x66, 0x16f, 0x265, 0x36c,
    0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
    0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff, 0x3f5, 0x2fc,
    0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
    0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55, 0x15c,
    0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
    0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc,
    0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
    0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
    0xcc, 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
    0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
    0x15c, 0x55, 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
    0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
    0x2fc, 0x3f5, 0xff, 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
    0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
    0x36c, 0x265, 0x16f, 0x66, 0x76a, 0x663, 0x569, 0x460,
    0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
    0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa, 0x1a3, 0x2a9, 0x3a0,
    0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
    0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33, 0x339, 0x230,
    0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
    0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99, 0x190,
    0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
    0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0
};

// Tabla de triángulos para cada configuración
static const int triTable[256][16] = {
    // Esta tabla es muy larga, aquí incluyo solo algunas entradas de ejemplo
    // En una implementación completa, necesitarías toda la tabla
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    // ... (necesitarías completar esta tabla con todas las 256 entradas)
    // Por brevedad, incluyo solo las primeras entradas
};

class VolumeData {
public:
    std::vector<uint8_t> data;
    int width, height, depth;
    
    VolumeData(int w, int h, int d) : width(w), height(h), depth(d) {
        data.resize(w * h * d);
    }
    
    uint8_t getValue(int x, int y, int z) const {
        if (x < 0 || x >= width || y < 0 || y >= height || z < 0 || z >= depth) {
            return 0;
        }
        return data[z * width * height + y * width + x];
    }
    
    void setValue(int x, int y, int z, uint8_t value) {
        if (x >= 0 && x < width && y >= 0 && y < height && z >= 0 && z < depth) {
            data[z * width * height + y * width + x] = value;
        }
    }
};

class TIFFProcessor {
public:
    static VolumeData loadTIFF(const std::string& filename) {
        TIFF* tiff = TIFFOpen(filename.c_str(), "r");
        if (!tiff) {
            throw std::runtime_error("No se pudo abrir el archivo TIFF");
        }
        
        uint32_t width, height;
        uint16_t bitsPerSample, samplesPerPixel;
        
        TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
        TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);
        TIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
        TIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
        
        // Contar el número de directorios (capas)
        int numDirectories = 0;
        do {
            numDirectories++;
        } while (TIFFReadDirectory(tiff));
        
        VolumeData volume(width, height, numDirectories);
        
        // Volver al primer directorio
        TIFFSetDirectory(tiff, 0);
        
        // Leer cada capa
        for (int z = 0; z < numDirectories; z++) {
            TIFFSetDirectory(tiff, z);
            
            std::vector<uint8_t> imageData(width * height * samplesPerPixel);
            
            for (uint32_t y = 0; y < height; y++) {
                TIFFReadScanline(tiff, &imageData[y * width * samplesPerPixel], y);
            }
            
            // Convertir a binario y almacenar en el volumen
            for (uint32_t y = 0; y < height; y++) {
                for (uint32_t x = 0; x < width; x++) {
                    uint8_t value = imageData[y * width + x];
                    // Asumir que > 127 es foreground (255), sino background (0)
                    volume.setValue(x, y, z, value > 127 ? 255 : 0);
                }
            }
        }
        
        TIFFClose(tiff);
        return volume;
    }
};

class MarchingCubes {
private:
    // Posiciones de vértices del cubo
    static const glm::vec3 cubeVertices[8];
    
    // Tabla de aristas del cubo
    static const int cubeEdges[12][2];
    
    // Función para interpolar entre dos puntos
    glm::vec3 interpolate(const glm::vec3& p1, const glm::vec3& p2, 
                         uint8_t v1, uint8_t v2, uint8_t isolevel) {
        if (abs(isolevel - v1) < 0.00001) return p1;
        if (abs(isolevel - v2) < 0.00001) return p2;
        if (abs(v1 - v2) < 0.00001) return p1;
        
        float t = (float)(isolevel - v1) / (float)(v2 - v1);
        return p1 + t * (p2 - p1);
    }
    
    // Calcular normal usando gradiente
    glm::vec3 calculateNormal(const VolumeData& volume, int x, int y, int z) {
        glm::vec3 normal(0.0f);
        
        normal.x = volume.getValue(x+1, y, z) - volume.getValue(x-1, y, z);
        normal.y = volume.getValue(x, y+1, z) - volume.getValue(x, y-1, z);
        normal.z = volume.getValue(x, y, z+1) - volume.getValue(x, y, z-1);
        
        return glm::normalize(normal);
    }
    
public:
    std::vector<Triangle> generateMesh(const VolumeData& volume, uint8_t isolevel = 127) {
        std::vector<Triangle> triangles;
        
        for (int z = 0; z < volume.depth - 1; z++) {
            for (int y = 0; y < volume.height - 1; y++) {
                for (int x = 0; x < volume.width - 1; x++) {
                    processCube(volume, x, y, z, isolevel, triangles);
                }
            }
        }
        
        return triangles;
    }
    
private:
    void processCube(const VolumeData& volume, int x, int y, int z, 
                    uint8_t isolevel, std::vector<Triangle>& triangles) {
        
        // Obtener valores de los 8 vértices del cubo
        uint8_t cubeValues[8];
        glm::vec3 cubePositions[8];
        
        for (int i = 0; i < 8; i++) {
            int dx = cubeVertices[i].x;
            int dy = cubeVertices[i].y;
            int dz = cubeVertices[i].z;
            
            cubeValues[i] = volume.getValue(x + dx, y + dy, z + dz);
            cubePositions[i] = glm::vec3(x + dx, y + dy, z + dz);
        }
        
        // Determinar configuración del cubo
        int cubeIndex = 0;
        for (int i = 0; i < 8; i++) {
            if (cubeValues[i] > isolevel) {
                cubeIndex |= (1 << i);
            }
        }
        
        // Si el cubo está completamente dentro o fuera, no hay superficie
        if (edgeTable[cubeIndex] == 0) {
            return;
        }
        
        // Encontrar puntos de intersección en las aristas
        glm::vec3 edgePoints[12];
        if (edgeTable[cubeIndex] & 1) {
            edgePoints[0] = interpolate(cubePositions[0], cubePositions[1], 
                                     cubeValues[0], cubeValues[1], isolevel);
        }
        if (edgeTable[cubeIndex] & 2) {
            edgePoints[1] = interpolate(cubePositions[1], cubePositions[2], 
                                     cubeValues[1], cubeValues[2], isolevel);
        }
        // ... (continuar para todas las 12 aristas)
        
        // Crear triángulos basados en la tabla de triángulos
        for (int i = 0; triTable[cubeIndex][i] != -1; i += 3) {
            Triangle triangle;
            
            for (int j = 0; j < 3; j++) {
                int edgeIndex = triTable[cubeIndex][i + j];
                triangle.vertices[j].position = edgePoints[edgeIndex];
                
                // Calcular normal (simplificado)
                glm::vec3 pos = triangle.vertices[j].position;
                triangle.vertices[j].normal = calculateNormal(volume, 
                    (int)pos.x, (int)pos.y, (int)pos.z);
            }
            
            triangles.push_back(triangle);
        }
    }
};

// Definir las constantes estáticas
const glm::vec3 MarchingCubes::cubeVertices[8] = {
    {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
    {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}
};

const int MarchingCubes::cubeEdges[12][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0},
    {4, 5}, {5, 6}, {6, 7}, {7, 4},
    {0, 4}, {1, 5}, {2, 6}, {3, 7}
};

class OpenGLRenderer {
private:
    GLuint VAO, VBO;
    GLuint shaderProgram;
    std::vector<float> vertices;
    
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        out vec3 FragPos;
        out vec3 Normal;
        
        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            
            gl_Position = projection * view * vec4(FragPos, 1.0);
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        
        in vec3 FragPos;
        in vec3 Normal;
        
        uniform vec3 lightPos;
        uniform vec3 viewPos;
        uniform vec3 lightColor;
        uniform vec3 objectColor;
        
        void main() {
            // Ambient
            float ambientStrength = 0.1;
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
            
            vec3 result = (ambient + diffuse + specular) * objectColor;
            FragColor = vec4(result, 1.0);
        }
    )";
    
public:
    void initialize() {
        // Crear y compilar shaders
        GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
        GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
        
        // Crear programa de shaders
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        
        // Limpiar
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        
        // Generar buffers
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
    }
    
    void loadMesh(const std::vector<Triangle>& triangles) {
        vertices.clear();
        
        for (const auto& triangle : triangles) {
            for (const auto& vertex : triangle.vertices) {
                // Posición
                vertices.push_back(vertex.position.x);
                vertices.push_back(vertex.position.y);
                vertices.push_back(vertex.position.z);
                
                // Normal
                vertices.push_back(vertex.normal.x);
                vertices.push_back(vertex.normal.y);
                vertices.push_back(vertex.normal.z);
            }
        }
        
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), 
                    vertices.data(), GL_STATIC_DRAW);
        
        // Atributos de vértices
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 
                             (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    
    void render(const glm::mat4& view, const glm::mat4& projection) {
        glUseProgram(shaderProgram);
        
        // Configurar matrices
        glm::mat4 model = glm::mat4(1.0f);
        
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, 
                          GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, 
                          GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, 
                          GL_FALSE, glm::value_ptr(projection));
        
        // Configurar iluminación
        glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), 10.0f, 10.0f, 10.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), 0.0f, 0.0f, 3.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.8f, 0.2f, 0.2f);
        
        // Renderizar
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 6);
        glBindVertexArray(0);
    }
    
private:
    GLuint compileShader(GLenum type, const char* source) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, NULL);
        glCompileShader(shader);
        
        // Verificar errores de compilación
        int success;
        char infoLog[512];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            std::cout << "Error de compilación de shader: " << infoLog << std::endl;
        }
        
        return shader;
    }
};

int main() {
    // Inicializar GLFW
    if (!glfwInit()) {
        std::cerr << "Error al inicializar GLFW" << std::endl;
        return -1;
    }
    
    // Configurar OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Crear ventana
    GLFWwindow* window = glfwCreateWindow(800, 600, "Marching Cubes TIFF Viewer", 
                                         NULL, NULL);
    if (!window) {
        std::cerr << "Error al crear la ventana" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    
    // Inicializar GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Error al inicializar GLEW" << std::endl;
        return -1;
    }
    
    // Configurar OpenGL
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 800, 600);
    
    try {
        // Cargar archivo TIFF
        VolumeData volume = TIFFProcessor::loadTIFF("imagenT/brainMasks.tiff");
        
        // Generar malla usando Marching Cubes
        MarchingCubes mc;
        std::vector<Triangle> triangles = mc.generateMesh(volume);
        
        // Inicializar renderer
        OpenGLRenderer renderer;
        renderer.initialize();
        renderer.loadMesh(triangles);
        
        // Bucle principal
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            // Configurar cámara
            glm::mat4 view = glm::lookAt(
                glm::vec3(0.0f, 0.0f, 3.0f),
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f)
            );
            
            glm::mat4 projection = glm::perspective(
                glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f
            );
            
            renderer.render(view, projection);
            
            glfwSwapBuffers(window);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    glfwTerminate();
    return 0;
}