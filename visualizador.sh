#!/bin/bash
# Instalar dependencias (Ubuntu/Debian)
#sudo apt-get install libglfw3-dev libglew-dev libglm-dev libopencv-dev

# Compilar
g++ -std=c++11 -o visualizador visualizador.cpp \
    -lglfw -lGL -lGLEW `pkg-config --cflags --libs opencv4`