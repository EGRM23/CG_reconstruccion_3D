#!/bin/bash
# Instalar OpenCV (Ubuntu/Debian)
#sudo apt-get install libopencv-dev

# Compilar
g++ -std=c++11 -o tiff_extractor main.cpp `pkg-config --cflags --libs opencv4`