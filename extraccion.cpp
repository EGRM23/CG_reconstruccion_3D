#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

class Point3D {
public:
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
};

class MultiTiffEdgeExtractor {
private:
    std::vector<cv::Mat> images;
    std::vector<Point3D> pointCloud;
    int totalImages;
    
    // Función para detectar si un píxel es borde
    bool isEdgePixel(const cv::Mat& img, int row, int col) {
        // Verificar si el píxel actual es blanco
        if (img.at<uchar>(row, col) == 0) return false;
        
        // Verificar los 8 vecinos
        for (int dr = -1; dr <= 1; dr++) {
            for (int dc = -1; dc <= 1; dc++) {
                if (dr == 0 && dc == 0) continue;
                
                int newRow = row + dr;
                int newCol = col + dc;
                
                // Verificar límites
                if (newRow < 0 || newRow >= img.rows || 
                    newCol < 0 || newCol >= img.cols) {
                    return true; // Borde de la imagen
                }
                
                // Si hay un píxel negro adyacente, es borde
                if (img.at<uchar>(newRow, newCol) == 0) {
                    return true;
                }
            }
        }
        return false;
    }
    
    // Función alternativa usando operadores morfológicos
    cv::Mat detectEdgesMorphological(const cv::Mat& binary) {
        cv::Mat edges;
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
        
        // Erosión de la imagen binaria
        cv::Mat eroded;
        cv::erode(binary, eroded, kernel);
        
        // Restar erosión de la imagen original para obtener bordes
        cv::subtract(binary, eroded, edges);
        
        return edges;
    }

public:
    // Cargar archivo TIFF multi-imagen
    bool loadMultiTiffImage(const std::string& filename) {
        images.clear();
        
        std::cout << "Cargando archivo TIFF multi-imagen: " << filename << std::endl;
        
        // Verificar si el archivo existe
        std::ifstream file(filename);
        if (!file.good()) {
            std::cerr << "Error: No se pudo encontrar el archivo " << filename << std::endl;
            return false;
        }
        file.close();
        
        // Cargar todas las imágenes del TIFF
        std::vector<cv::Mat> tempImages;
        bool success = cv::imreadmulti(filename, tempImages, cv::IMREAD_GRAYSCALE);
        
        if (!success || tempImages.empty()) {
            std::cerr << "Error: No se pudo cargar el archivo multi-TIFF " << filename << std::endl;
            return false;
        }
        
        totalImages = tempImages.size();
        std::cout << "Número total de imágenes encontradas: " << totalImages << std::endl;
        
        // Procesar cada imagen para asegurar que sea binaria
        for (int i = 0; i < totalImages; i++) {
            cv::Mat binaryImage;
            cv::threshold(tempImages[i], binaryImage, 127, 255, cv::THRESH_BINARY);
            images.push_back(binaryImage);
            
            if (i == 0) {
                std::cout << "Dimensiones de imagen: " << binaryImage.cols << "x" << binaryImage.rows << " píxeles" << std::endl;
            }
        }
        
        return true;
    }
    
    // Extraer puntos de borde de todas las imágenes usando método manual
    void extractEdgePointsManual() {
        pointCloud.clear();
        
        std::cout << "Extrayendo puntos de borde de " << totalImages << " imágenes..." << std::endl;
        
        for (int imgIndex = 0; imgIndex < totalImages; imgIndex++) {
            const cv::Mat& currentImage = images[imgIndex];
            
            // Mostrar progreso cada 10 imágenes
            if (imgIndex % 10 == 0) {
                std::cout << "Procesando imagen " << (imgIndex + 1) << "/" << totalImages << std::endl;
            }
            
            for (int row = 0; row < currentImage.rows; row++) {
                for (int col = 0; col < currentImage.cols; col++) {
                    if (isEdgePixel(currentImage, row, col)) {
                        // Convertir coordenadas de imagen a coordenadas del mundo
                        double x = col;
                        double y = currentImage.rows - row; // Invertir Y para coordenadas estándar
                        double z = imgIndex; // Número de imagen (0-based)
                        
                        pointCloud.push_back(Point3D(x, y, z));
                    }
                }
            }
        }
        
        std::cout << "Puntos de borde extraídos total: " << pointCloud.size() << std::endl;
    }
    
    // Extraer puntos de borde usando operadores morfológicos
    void extractEdgePointsMorphological() {
        pointCloud.clear();
        
        std::cout << "Extrayendo puntos de borde con operadores morfológicos de " << totalImages << " imágenes..." << std::endl;
        
        for (int imgIndex = 0; imgIndex < totalImages; imgIndex++) {
            const cv::Mat& currentImage = images[imgIndex];
            cv::Mat edges = detectEdgesMorphological(currentImage);
            
            // Mostrar progreso cada 10 imágenes
            if (imgIndex % 10 == 0) {
                std::cout << "Procesando imagen " << (imgIndex + 1) << "/" << totalImages << std::endl;
            }
            
            for (int row = 0; row < edges.rows; row++) {
                for (int col = 0; col < edges.cols; col++) {
                    if (edges.at<uchar>(row, col) > 0) {
                        double x = col;
                        double y = edges.rows - row;
                        double z = imgIndex; // Número de imagen (0-based)
                        
                        pointCloud.push_back(Point3D(x, y, z));
                    }
                }
            }
        }
        
        std::cout << "Puntos de borde extraídos total: " << pointCloud.size() << std::endl;
    }
    
    // Extraer puntos de borde de un rango específico de imágenes
    void extractEdgePointsRange(int startImg, int endImg, bool useMorphological = false) {
        pointCloud.clear();
        
        // Validar rango
        startImg = std::max(0, startImg);
        endImg = std::min(totalImages - 1, endImg);
        
        std::cout << "Extrayendo puntos de borde de imágenes " << startImg << " a " << endImg << std::endl;
        
        for (int imgIndex = startImg; imgIndex <= endImg; imgIndex++) {
            const cv::Mat& currentImage = images[imgIndex];
            
            std::cout << "Procesando imagen " << (imgIndex + 1) << "/" << totalImages << std::endl;
            
            if (useMorphological) {
                cv::Mat edges = detectEdgesMorphological(currentImage);
                
                for (int row = 0; row < edges.rows; row++) {
                    for (int col = 0; col < edges.cols; col++) {
                        if (edges.at<uchar>(row, col) > 0) {
                            double x = col;
                            double y = edges.rows - row;
                            double z = imgIndex;
                            
                            pointCloud.push_back(Point3D(x, y, z));
                        }
                    }
                }
            } else {
                for (int row = 0; row < currentImage.rows; row++) {
                    for (int col = 0; col < currentImage.cols; col++) {
                        if (isEdgePixel(currentImage, row, col)) {
                            double x = col;
                            double y = currentImage.rows - row;
                            double z = imgIndex;
                            
                            pointCloud.push_back(Point3D(x, y, z));
                        }
                    }
                }
            }
        }
        
        std::cout << "Puntos de borde extraídos: " << pointCloud.size() << std::endl;
    }
    
    // Guardar nube de puntos en formato PLY
    bool savePointCloudPLY(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: No se pudo crear el archivo " << filename << std::endl;
            return false;
        }
        
        // Escribir cabecera PLY
        file << "ply\n";
        file << "format ascii 1.0\n";
        file << "element vertex " << pointCloud.size() << "\n";
        file << "property float x\n";
        file << "property float y\n";
        file << "property float z\n";
        file << "end_header\n";
        
        // Escribir puntos
        for (const auto& point : pointCloud) {
            file << point.x << " " << point.y << " " << point.z << "\n";
        }
        
        file.close();
        std::cout << "Nube de puntos guardada en: " << filename << std::endl;
        return true;
    }
    
    // Guardar nube de puntos en formato XYZ
    bool savePointCloudXYZ(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: No se pudo crear el archivo " << filename << std::endl;
            return false;
        }
        
        for (const auto& point : pointCloud) {
            file << point.x << " " << point.y << " " << point.z << "\n";
        }
        
        file.close();
        std::cout << "Nube de puntos guardada en: " << filename << std::endl;
        return true;
    }
    
    // Guardar nube de puntos en formato PCD (Point Cloud Data)
    bool savePointCloudPCD(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: No se pudo crear el archivo " << filename << std::endl;
            return false;
        }
        
        // Escribir cabecera PCD
        file << "# .PCD v0.7 - Point Cloud Data file format\n";
        file << "VERSION 0.7\n";
        file << "FIELDS x y z\n";
        file << "SIZE 4 4 4\n";
        file << "TYPE F F F\n";
        file << "COUNT 1 1 1\n";
        file << "WIDTH " << pointCloud.size() << "\n";
        file << "HEIGHT 1\n";
        file << "VIEWPOINT 0 0 0 1 0 0 0\n";
        file << "POINTS " << pointCloud.size() << "\n";
        file << "DATA ascii\n";
        
        // Escribir puntos
        for (const auto& point : pointCloud) {
            file << point.x << " " << point.y << " " << point.z << "\n";
        }
        
        file.close();
        std::cout << "Nube de puntos guardada en: " << filename << std::endl;
        return true;
    }
    
    // Guardar imágenes de bordes individuales para verificación
    bool saveEdgeImages(const std::string& baseName, int maxImages = 10) {
        std::cout << "Guardando imágenes de bordes (máximo " << maxImages << ")..." << std::endl;
        
        int imagesToSave = std::min(maxImages, totalImages);
        
        for (int i = 0; i < imagesToSave; i++) {
            cv::Mat edges = detectEdgesMorphological(images[i]);
            std::string filename = baseName + "_edges_" + std::to_string(i) + ".png";
            
            if (!cv::imwrite(filename, edges)) {
                std::cerr << "Error guardando imagen: " << filename << std::endl;
                return false;
            }
        }
        
        std::cout << "Imágenes de bordes guardadas." << std::endl;
        return true;
    }
    
    // Obtener estadísticas de la nube de puntos
    void printStatistics() {
        if (pointCloud.empty()) {
            std::cout << "No hay puntos en la nube" << std::endl;
            return;
        }
        
        double minX = pointCloud[0].x, maxX = pointCloud[0].x;
        double minY = pointCloud[0].y, maxY = pointCloud[0].y;
        double minZ = pointCloud[0].z, maxZ = pointCloud[0].z;
        
        for (const auto& point : pointCloud) {
            minX = std::min(minX, point.x);
            maxX = std::max(maxX, point.x);
            minY = std::min(minY, point.y);
            maxY = std::max(maxY, point.y);
            minZ = std::min(minZ, point.z);
            maxZ = std::max(maxZ, point.z);
        }
        
        std::cout << "\n=== Estadísticas de la nube de puntos ===" << std::endl;
        std::cout << "Número total de puntos: " << pointCloud.size() << std::endl;
        std::cout << "Número de imágenes procesadas: " << totalImages << std::endl;
        std::cout << "Promedio de puntos por imagen: " << (double)pointCloud.size() / totalImages << std::endl;
        std::cout << "Rango X: [" << minX << ", " << maxX << "]" << std::endl;
        std::cout << "Rango Y: [" << minY << ", " << maxY << "]" << std::endl;
        std::cout << "Rango Z: [" << minZ << ", " << maxZ << "]" << std::endl;
    }
    
    // Obtener información del archivo TIFF
    void printTiffInfo() {
        if (images.empty()) {
            std::cout << "No hay imágenes cargadas" << std::endl;
            return;
        }
        
        std::cout << "\n=== Información del archivo TIFF ===" << std::endl;
        std::cout << "Número total de imágenes: " << totalImages << std::endl;
        std::cout << "Dimensiones: " << images[0].cols << "x" << images[0].rows << " píxeles" << std::endl;
        std::cout << "Tipo de datos: " << images[0].type() << std::endl;
        std::cout << "Canales: " << images[0].channels() << std::endl;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Uso: " << argv[0] << " <archivo_tiff> [método] [imagen_inicio] [imagen_fin]" << std::endl;
        std::cout << "Métodos disponibles:" << std::endl;
        std::cout << "  manual (por defecto) - Detección manual de bordes" << std::endl;
        std::cout << "  morphological - Detección con operadores morfológicos" << std::endl;
        std::cout << "Ejemplos:" << std::endl;
        std::cout << "  " << argv[0] << " stack.tiff" << std::endl;
        std::cout << "  " << argv[0] << " stack.tiff morphological" << std::endl;
        std::cout << "  " << argv[0] << " stack.tiff manual 0 50" << std::endl;
        return -1;
    }
    
    std::string inputFile = argv[1];
    std::string method = (argc >= 3) ? argv[2] : "manual";
    
    MultiTiffEdgeExtractor extractor;
    
    // Cargar archivo TIFF multi-imagen
    if (!extractor.loadMultiTiffImage(inputFile)) {
        return -1;
    }
    
    // Mostrar información del archivo
    extractor.printTiffInfo();
    
    // Extraer puntos de borde según los parámetros
    if (argc >= 5) {
        // Rango específico de imágenes
        int startImg = std::atoi(argv[3]);
        int endImg = std::atoi(argv[4]);
        bool useMorphological = (method == "morphological");
        extractor.extractEdgePointsRange(startImg, endImg, useMorphological);
    } else {
        // Todas las imágenes
        if (method == "morphological") {
            extractor.extractEdgePointsMorphological();
        } else {
            extractor.extractEdgePointsManual();
        }
    }
    
    // Mostrar estadísticas
    extractor.printStatistics();
    
    // Generar nombres de archivo de salida
    std::string baseName = inputFile.substr(0, inputFile.find_last_of('.'));
    //std::string plyFile = "output/" + baseName + "_3D_edges.ply";
    std::string xyzFile = "output/" + baseName + "_3D_edges_" + method + ".xyz";
    //std::string pcdFile = "output/" + baseName + "_3D_edges.pcd";
    
    // Guardar resultados
    //extractor.savePointCloudPLY(plyFile);
    extractor.savePointCloudXYZ(xyzFile);
    //extractor.savePointCloudPCD(pcdFile);
    
    // Guardar algunas imágenes de bordes para verificación
    extractor.saveEdgeImages(baseName, 0);
    
    std::cout << "\nProcesamiento completado exitosamente!" << std::endl;
    std::cout << "Archivos generados:" << std::endl;
    //std::cout << "  - " << plyFile << " (formato PLY)" << std::endl;
    std::cout << "  - " << xyzFile << " (formato XYZ)" << std::endl;
    //std::cout << "  - " << pcdFile << " (formato PCD)" << std::endl;
    
    return 0;
}