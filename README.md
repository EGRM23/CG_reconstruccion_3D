## Modo de compilación

```bash
g++ -std=c++11 -o tiff_extractor main.cpp `pkg-config --cflags --libs opencv4`

g++ -std=c++11 -o visualizador visualizador.cpp \
    -lglfw -lGL -lGLEW `pkg-config --cflags --libs opencv4`
```

## Ejemplos de ejecución para generar los puntos

```bash
./tiff_extractor imagenT/eyeMasks.tiff manual
./tiff_extractor imagenT/eyeMasks.tiff morphological
```

## Ejemplos de ejecución para generar la visualización

```bash
./visualizador output/imagenT/eyeMasks_3D_edges_manual.xyz
./visualizador output/imagenT/eyeMasks_3D_edges_morphological.xyz
```
