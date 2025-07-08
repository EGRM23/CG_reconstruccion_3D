El modo de compilación es:

g++ -std=c++11 -o tiff_extractor main.cpp `pkg-config --cflags --libs opencv4`

g++ -std=c++11 -o visualizador visualizador.cpp \
    -lglfw -lGL -lGLEW `pkg-config --cflags --libs opencv4`

Ejemplos de ejecucion para generar los puntos:
./tiff_extractor imagenT/eyeMasks.tiff manual
./tiff_extractor imagenT/eyeMasks.tiff morphological

Ejemplos de ejecucion para generar la visualizacion:
./visualizador output/imagenT/eyeMasks_3D_edges_manual.xyz
./visualizador output/imagenT/eyeMasks_3D_edges_morphological.xyz
