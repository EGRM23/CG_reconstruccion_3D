#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <limits>

struct Point3D {
    double x, y, z;
    Point3D(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
    
    double distanceTo(const Point3D& other) const {
        double dx = x - other.x;
        double dy = y - other.y;
        double dz = z - other.z;
        return sqrt(dx*dx + dy*dy + dz*dz);
    }
};

class BallRadiusAnalyzer {
private:
    std::vector<Point3D> points;
    
public:
    bool loadPoints(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: No se pudo abrir " << filename << std::endl;
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
        
        std::cout << "Cargados " << points.size() << " puntos" << std::endl;
        return true;
    }
    
    void analyzePointDistribution() {
        if (points.size() < 2) {
            std::cerr << "Necesitas al menos 2 puntos para el análisis" << std::endl;
            return;
        }
        
        std::vector<double> nearestDistances;
        std::vector<double> allDistances;
        
        // Calcular distancias
        for (size_t i = 0; i < points.size(); i++) {
            double minDist = std::numeric_limits<double>::max();
            
            for (size_t j = 0; j < points.size(); j++) {
                if (i == j) continue;
                
                double dist = points[i].distanceTo(points[j]);
                allDistances.push_back(dist);
                
                if (dist < minDist) {
                    minDist = dist;
                }
            }
            
            nearestDistances.push_back(minDist);
        }
        
        // Estadísticas de distancias al vecino más cercano
        std::sort(nearestDistances.begin(), nearestDistances.end());
        
        double sumNearest = 0;
        for (double d : nearestDistances) {
            sumNearest += d;
        }
        
        double avgNearest = sumNearest / nearestDistances.size();
        double medianNearest = nearestDistances[nearestDistances.size() / 2];
        double minNearest = nearestDistances[0];
        double maxNearest = nearestDistances[nearestDistances.size() - 1];
        
        // Estadísticas de todas las distancias
        std::sort(allDistances.begin(), allDistances.end());
        double sumAll = 0;
        for (double d : allDistances) {
            sumAll += d;
        }
        double avgAll = sumAll / allDistances.size();
        
        // Análisis de bounding box
        Point3D minPoint = points[0];
        Point3D maxPoint = points[0];
        
        for (const Point3D& p : points) {
            if (p.x < minPoint.x) minPoint.x = p.x;
            if (p.y < minPoint.y) minPoint.y = p.y;
            if (p.z < minPoint.z) minPoint.z = p.z;
            if (p.x > maxPoint.x) maxPoint.x = p.x;
            if (p.y > maxPoint.y) maxPoint.y = p.y;
            if (p.z > maxPoint.z) maxPoint.z = p.z;
        }
        
        double rangeX = maxPoint.x - minPoint.x;
        double rangeY = maxPoint.y - minPoint.y;
        double rangeZ = maxPoint.z - minPoint.z;
        
        // Mostrar resultados
        std::cout << "\n=== ANÁLISIS DE DISTRIBUCIÓN DE PUNTOS ===" << std::endl;
        std::cout << "Número total de puntos: " << points.size() << std::endl;
        std::cout << "\nBounding Box:" << std::endl;
        std::cout << "  X: [" << minPoint.x << ", " << maxPoint.x << "] (rango: " << rangeX << ")" << std::endl;
        std::cout << "  Y: [" << minPoint.y << ", " << maxPoint.y << "] (rango: " << rangeY << ")" << std::endl;
        std::cout << "  Z: [" << minPoint.z << ", " << maxPoint.z << "] (rango: " << rangeZ << ")" << std::endl;
        
        std::cout << "\nDistancias al vecino más cercano:" << std::endl;
        std::cout << "  Mínima: " << minNearest << std::endl;
        std::cout << "  Máxima: " << maxNearest << std::endl;
        std::cout << "  Promedio: " << avgNearest << std::endl;
        std::cout << "  Mediana: " << medianNearest << std::endl;
        
        std::cout << "\nDistancia promedio entre todos los puntos: " << avgAll << std::endl;
        
        // Recomendaciones de radio
        std::cout << "\n=== RECOMENDACIONES DE RADIO DE BOLA ===" << std::endl;
        
        double radio1 = avgNearest * 0.5;
        double radio2 = avgNearest * 1.0;
        double radio3 = avgNearest * 1.5;
        double radio4 = avgNearest * 2.0;
        
        std::cout << "Radio conservador (detalle fino): " << radio1 << std::endl;
        std::cout << "Radio recomendado (equilibrado): " << radio2 << std::endl;
        std::cout << "Radio moderado (menos detalle): " << radio3 << std::endl;
        std::cout << "Radio amplio (superficie suave): " << radio4 << std::endl;
        
        std::cout << "\n=== SUGERENCIAS ===" << std::endl;
        std::cout << "• Comienza con el radio recomendado: " << radio2 << std::endl;
        std::cout << "• Si no se genera malla, prueba con: " << radio3 << " o " << radio4 << std::endl;
        std::cout << "• Si la malla tiene muchos huecos, usa: " << radio1 << std::endl;
        std::cout << "• Para superficies rugosas, usa radios menores" << std::endl;
        std::cout << "• Para superficies suaves, usa radios mayores" << std::endl;
        
        // Análisis de densidad por regiones
        analyzeLocalDensity();
    }
    
    void analyzeLocalDensity() {
        if (points.size() < 10) return;
        
        std::cout << "\n=== ANÁLISIS DE DENSIDAD LOCAL ===" << std::endl;
        
        std::vector<int> neighborCounts;
        double searchRadius = 0;
        
        // Calcular radio de búsqueda como 2x la distancia promedio al vecino más cercano
        double sumNearest = 0;
        for (size_t i = 0; i < std::min(points.size(), size_t(100)); i++) {
            double minDist = std::numeric_limits<double>::max();
            for (size_t j = 0; j < points.size(); j++) {
                if (i == j) continue;
                double dist = points[i].distanceTo(points[j]);
                if (dist < minDist) minDist = dist;
            }
            sumNearest += minDist;
        }
        searchRadius = 2.0 * (sumNearest / std::min(points.size(), size_t(100)));
        
        // Contar vecinos en el radio de búsqueda
        for (size_t i = 0; i < points.size(); i++) {
            int count = 0;
            for (size_t j = 0; j < points.size(); j++) {
                if (i == j) continue;
                if (points[i].distanceTo(points[j]) <= searchRadius) {
                    count++;
                }
            }
            neighborCounts.push_back(count);
        }
        
        std::sort(neighborCounts.begin(), neighborCounts.end());
        
        int minNeighbors = neighborCounts[0];
        int maxNeighbors = neighborCounts[neighborCounts.size() - 1];
        double avgNeighbors = 0;
        for (int n : neighborCounts) avgNeighbors += n;
        avgNeighbors /= neighborCounts.size();
        
        std::cout << "Radio de búsqueda local: " << searchRadius << std::endl;
        std::cout << "Vecinos por punto (promedio): " << avgNeighbors << std::endl;
        std::cout << "Vecinos por punto (rango): [" << minNeighbors << ", " << maxNeighbors << "]" << std::endl;
        
        if (avgNeighbors < 4) {
            std::cout << "⚠️  ADVERTENCIA: Puntos muy dispersos, usa radio grande" << std::endl;
        } else if (avgNeighbors > 20) {
            std::cout << "ℹ️  NOTA: Puntos muy densos, puedes usar radio pequeño" << std::endl;
        }
    }
    
    void testRadiusRange(double minRadius, double maxRadius, int steps) {
        std::cout << "\n=== PRUEBA DE RANGO DE RADIOS ===" << std::endl;
        
        double step = (maxRadius - minRadius) / steps;
        
        for (int i = 0; i <= steps; i++) {
            double radius = minRadius + i * step;
            int validTriangles = countValidTriangles(radius);
            
            std::cout << "Radio " << radius << ": " << validTriangles << " triángulos potenciales" << std::endl;
        }
    }
    
private:
    int countValidTriangles(double ballRadius) {
        int count = 0;
        int maxTests = std::min(1000, (int)(points.size() * (points.size() - 1) * (points.size() - 2) / 6));
        
        for (int test = 0; test < maxTests; test++) {
            int i = rand() % points.size();
            int j = rand() % points.size();
            int k = rand() % points.size();
            
            if (i == j || j == k || i == k) continue;
            
            // Verificar si puede formar un triángulo válido con este radio
            if (isValidTriangle(i, j, k, ballRadius)) {
                count++;
            }
        }
        
        return count;
    }
    
    bool isValidTriangle(int i, int j, int k, double ballRadius) {
        Point3D p1 = points[i];
        Point3D p2 = points[j];
        Point3D p3 = points[k];
        
        // Calcular circunradio aproximado
        double a = p1.distanceTo(p2);
        double b = p2.distanceTo(p3);
        double c = p3.distanceTo(p1);
        
        double s = (a + b + c) / 2.0;
        double area = sqrt(s * (s - a) * (s - b) * (s - c));
        
        if (area < 1e-10) return false;
        
        double circumradius = (a * b * c) / (4.0 * area);
        
        return circumradius <= ballRadius;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Uso: " << argv[0] << " <archivo.xyz>" << std::endl;
        return 1;
    }
    
    BallRadiusAnalyzer analyzer;
    
    if (!analyzer.loadPoints(argv[1])) {
        return 1;
    }
    
    analyzer.analyzePointDistribution();
    
    // Prueba de diferentes radios
    std::cout << "\n¿Quieres probar un rango de radios? (y/n): ";
    char response;
    std::cin >> response;
    
    if (response == 'y' || response == 'Y') {
        double minR, maxR;
        int steps;
        std::cout << "Radio mínimo: ";
        std::cin >> minR;
        std::cout << "Radio máximo: ";
        std::cin >> maxR;
        std::cout << "Número de pasos: ";
        std::cin >> steps;
        
        analyzer.testRadiusRange(minR, maxR, steps);
    }
    
    return 0;
}