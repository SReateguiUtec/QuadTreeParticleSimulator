# Proyecto 2 AED - QuadTree para simulacion de particulas

Este proyecto implementa un QuadTree desde cero para acelerar la deteccion de posibles colisiones entre particulas circulares en un espacio 2D. La idea principal es comparar el enfoque con QuadTree contra una solucion ingenua de fuerza bruta que revisa todos los pares de particulas.

El proyecto corresponde a la opcion de QuadTree del Proyecto 2 del curso de Algoritmos y Estructuras de Datos.

## Estado actual

Actualmente el proyecto tiene:

- Estructura `Particle` con id, posicion, velocidad y radio.
- Estructura `Rectangle` para representar regiones rectangulares.
- Implementacion basica de `QuadTree` desde cero.
- Insercion de particulas en el QuadTree.
- Subdivision de nodos cuando se supera la capacidad maxima.
- Redistribucion de particulas antiguas al subdividir un nodo.
- Consulta rectangular con `queryRange`.
- Actualizacion de posiciones frame por frame.
- Rebote simple de particulas contra los bordes del mundo.
- Deteccion de colisiones por fuerza bruta.
- Deteccion de colisiones usando QuadTree.
- Consulta de particulas cercanas a un punto con `queryNearPoint`.
- Conteo de nodos visitados durante las consultas del QuadTree.
- Conteo de particulas revisadas y candidatos encontrados por el QuadTree.
- Generacion sintetica de datos con tres distribuciones:
  - uniforme;
  - clusters;
  - zona de alta densidad.
- Experimentos automaticos con 1000, 5000 y 10000 particulas.
- Tabla comparativa con tiempos, comparaciones y colisiones promedio.
- Exportacion de resultados experimentales en `results.csv`.
- Generacion de una visualizacion en `visualization.svg`.
- Generacion de datos frame por frame en `visualization_data.js`.
- Front dinamico en `frontend/index.html`.

## Estructura de archivos

```txt
.
├── CMakeLists.txt
├── Experiment.cpp
├── Experiment.h
├── Main.cpp
├── Structure.cpp
├── Structure.h
├── frontend
│   ├── app.js
│   ├── index.html
│   └── styles.css
├── Visualization.cpp
└── Visualization.h
```

### `Structure.h`

Declara las estructuras y funciones principales:

- `Particle`: representa una particula circular.
- `Rectangle`: representa una region rectangular del espacio.
- `CollisionStats`: almacena cantidad de colisiones y comparaciones.
- `QueryStats`: almacena nodos visitados, particulas revisadas y candidatos encontrados durante una consulta.
- `QuadTree`: clase principal del QuadTree.
- Funciones auxiliares para movimiento, distancia, colision y comparacion experimental.

### `Structure.cpp`

Implementa:

- `Rectangle::contains`
- `Rectangle::intersects`
- `QuadTree::insert`
- `QuadTree::subdivide`
- `QuadTree::queryRange`
- `QuadTree::queryNearPoint`
- `updateParticles`
- `distanceBetween`
- `areColliding`
- `bruteForceCollisionStats`
- `quadTreeCollisionStats`

### `Main.cpp`

Contiene el flujo principal del programa:

- define parametros globales;
- ejecuta los experimentos;
- imprime la tabla de resultados;
- exporta `results.csv`;
- genera `visualization.svg`.

### `Experiment.h` y `Experiment.cpp`

Contienen la generacion de datos y el corredor de experimentos:

- genera particulas sinteticas;
- ejecuta varios frames de simulacion;
- construye el QuadTree en cada frame;
- compara QuadTree contra fuerza bruta;
- imprime una tabla resumen con los resultados;
- exporta los resultados a CSV.

### `Visualization.h` y `Visualization.cpp`

Contienen la generacion de salidas visuales:

- construye una simulacion pequena;
- extrae las subdivisiones del QuadTree;
- consulta particulas cercanas a un punto;
- genera `visualization.svg`;
- genera `visualization_data.js` para el front dinamico.

### `frontend/`

Contiene una visualizacion dinamica en HTML, CSS y JavaScript:

- reproduce la simulacion frame por frame;
- dibuja particulas, candidatos, colisiones y regiones del QuadTree en Canvas;
- permite pausar, avanzar, retroceder y mover el frame con un slider;
- muestra metricas del frame actual.

## Modelo de particula

```cpp
struct Particle {
    int id;
    double x, y;
    double vx, vy;
    double radius;
};
```

Cada particula tiene una posicion `(x, y)`, una velocidad `(vx, vy)` y un radio. Dos particulas se consideran en colision si la distancia entre sus centros es menor o igual que la suma de sus radios.

## Como funciona el QuadTree

El QuadTree divide recursivamente el espacio 2D en cuatro regiones:

- noreste;
- noroeste;
- sureste;
- suroeste.

Cada nodo almacena particulas mientras no supere una capacidad maxima. Si se supera esa capacidad, el nodo se subdivide en cuatro hijos y las particulas se redistribuyen hacia el cuadrante correspondiente.

La consulta rectangular (`queryRange`) evita revisar regiones que no intersectan con el area consultada. Esto permite reducir la cantidad de particulas candidatas frente a una busqueda lineal.

Ademas, el QuadTree expone `queryNearPoint`, que permite consultar particulas cercanas a un punto. Internamente se consulta primero una region cuadrada alrededor del punto y luego se filtran las particulas que realmente estan dentro del radio circular.

## Deteccion de colisiones

El proyecto implementa dos enfoques.

### Fuerza bruta

Compara cada particula contra todas las demas sin repetir pares:

```txt
particula i contra particula j, con j > i
```

Esto hace aproximadamente:

```txt
n * (n - 1) / 2
```

comparaciones por frame.

### QuadTree

Para cada particula se crea una region de busqueda alrededor de ella. Luego se consulta el QuadTree para obtener solo particulas candidatas cercanas. Finalmente, se aplica la formula exacta de colision solo contra esos candidatos.

Este enfoque debe encontrar las mismas colisiones que fuerza bruta, pero con muchas menos comparaciones cuando la distribucion espacial permite filtrar regiones.

## Distribuciones implementadas

### Uniforme

Las particulas se ubican aleatoriamente en todo el espacio 2D.

### Clusters

Se generan varios centros y las particulas se ubican alrededor de esos centros usando una distribucion normal. Sirve para observar como cambia el rendimiento cuando los datos no estan uniformemente repartidos.

### Alta densidad

El 70% de las particulas se ubica dentro de una region central pequena. El resto se distribuye en todo el mundo. Este escenario genera mas colisiones y mas candidatos por particula.

## Metricas que imprime el programa

La tabla de salida incluye:

- `Distribucion`: tipo de distribucion usada.
- `Particulas`: cantidad total de particulas.
- `Frames`: cantidad de frames usados para calcular promedios.
- `Construccion QuadTree`: tiempo promedio de construccion del QuadTree por frame.
- `Deteccion con QuadTree`: tiempo promedio para detectar colisiones usando QuadTree.
- `Deteccion con fuerza bruta`: tiempo promedio para detectar colisiones revisando todos los pares.
- `Comparaciones QuadTree`: cantidad promedio de comparaciones exactas realizadas por el metodo con QuadTree.
- `Comparaciones fuerza bruta`: cantidad promedio de comparaciones realizadas por fuerza bruta.
- `Nodos visitados QuadTree`: cantidad promedio de nodos del QuadTree visitados durante las consultas.
- `Particulas revisadas QuadTree`: cantidad promedio de particulas revisadas dentro de nodos que intersectan la consulta.
- `Candidatos QuadTree`: cantidad promedio de particulas candidatas devueltas por las consultas rectangulares del QuadTree.
- `Colisiones`: cantidad promedio de colisiones encontradas.
- `Candidatos por particula`: comparaciones promedio del QuadTree divididas entre el numero de particulas.

Los tiempos estan expresados en microsegundos.

## CSV de resultados

Al ejecutar el programa tambien se genera:

```txt
results.csv
```

Este archivo contiene las mismas metricas principales de la tabla de consola, pero en formato CSV para abrirlo con Excel, Google Sheets, LibreOffice Calc o usarlo para graficos del reporte.

## Visualizacion estatica

Al final de la ejecucion, el programa genera el archivo:

```txt
visualization.svg
```

Esta visualizacion muestra:

- particulas en el plano;
- subdivisiones actuales del QuadTree;
- radio de una consulta cercana a un punto;
- candidatos encontrados por la consulta;
- colisiones detectadas;
- cantidad de nodos del QuadTree usados en la visualizacion.

La leyenda de colores es:

- azul: particulas normales;
- naranja: particulas candidatas encontradas por la consulta;
- rojo: particulas en colision;
- morado: radio de consulta;
- gris: regiones o nodos del QuadTree.

La visualizacion se genera con una simulacion pequena de 250 particulas para que el resultado sea legible.

## Visualizacion dinamica

El programa tambien genera:

```txt
visualization_data.js
```

Este archivo contiene 120 frames de simulacion generados desde C++. El front esta en:

```txt
frontend/index.html
```

El front permite reproducir la simulacion frame por frame. Muestra:

- particulas en movimiento;
- subdivisiones del QuadTree por frame;
- consulta circular movil;
- candidatos retornados por el QuadTree;
- colisiones del frame;
- metricas internas del QuadTree.

Se puede abrir `frontend/index.html` directamente en el navegador despues de ejecutar el programa.

### Cambiar cantidad de particulas del front

La cantidad de particulas de la visualizacion dinamica se configura en `Main.cpp`:

```cpp
int visualParticleCount = 420;
int visualFrames = 160;
double visualParticleRadius = 3;
```

Despues de cambiar esos valores, se debe recompilar y ejecutar el programa para regenerar `visualization_data.js`.

La visualizacion dinamica usa un escenario tipo colision de galaxias: dos grupos de particulas se generan como discos espirales y avanzan uno contra el otro. El boton `Modo plano` / `Modo espacio` del front cambia el estilo visual sin modificar los datos.

## Compilacion y ejecucion

### Opcion 1: usando g++

```bash
g++ -std=c++20 Main.cpp Structure.cpp Experiment.cpp Visualization.cpp -o proyecto2
./proyecto2
```

Para compilar con advertencias:

```bash
g++ -std=c++20 -Wall -Wextra -pedantic Main.cpp Structure.cpp Experiment.cpp Visualization.cpp -o proyecto2
./proyecto2
```

Despues de ejecutar el programa, se crean `results.csv`, `visualization.svg` y `visualization_data.js` en la carpeta del proyecto. El front dinamico queda disponible en `frontend/index.html`.

### Opcion 2: usando CMake

Si `cmake` esta instalado:

```bash
cmake -S . -B build
cmake --build build
./build/proyecto2
```

En CLion, el proyecto puede ejecutarse directamente usando el archivo `CMakeLists.txt`.

## Ejemplo de salida

La salida tiene esta forma:

```txt
Experimentos QuadTree vs fuerza bruta
Mundo: 1000 x 1000, radio: 5, capacidad: 4, frames por caso: 3
Los tiempos estan expresados en microsegundos.
Candidatos por particula = comparaciones promedio del QuadTree / numero de particulas.
Nodos visitados, particulas revisadas y candidatos corresponden a las consultas internas del QuadTree.

Distribucion            Particulas    Frames         Construccion QuadTree        Deteccion con QuadTree      Deteccion con fuerza bruta          Comparaciones QuadTree        Comparaciones fuerza bruta      Nodos visitados QuadTree     Particulas revisadas QuadTree         Candidatos QuadTree            Colisiones          Candidatos por particula
uniforme                      1000         3                           ...                           ...                             ...                             ...                            499500                           ...                               ...                         ...                   ...                              ...
uniforme                      5000         3                           ...                           ...                             ...                             ...                          12497500                           ...                               ...                         ...                   ...                              ...
uniforme                     10000         3                           ...                           ...                             ...                             ...                          49995000                           ...                               ...                         ...                   ...                              ...
```

Los valores exactos pueden cambiar segun la maquina, el compilador y el modo de compilacion.

## Partes del enunciado ya cubiertas

- Implementacion del QuadTree desde cero.
- Insercion de objetos en el QuadTree.
- Subdivision cuando se supera la capacidad maxima.
- Actualizacion de posiciones frame por frame.
- Reconstruccion del QuadTree despues del movimiento.
- Consulta rectangular.
- Consulta de particulas cercanas a un punto.
- Deteccion de colisiones.
- Comparacion contra fuerza bruta.
- Experimentos con tres tamanos de entrada: 1000, 5000 y 10000.
- Pruebas con tres distribuciones espaciales.
- Medicion de tiempo promedio por frame.
- Medicion de comparaciones promedio.
- Medicion de candidatos revisados por particula.
- Medicion de nodos visitados por el QuadTree.
- Medicion de particulas revisadas por las consultas del QuadTree.

## Pendientes importantes

Todavia falta completar:

- Reporte final con interpretacion de resultados.

## Interpretacion preliminar

La fuerza bruta siempre revisa todos los pares de particulas, por lo que su numero de comparaciones crece cuadraticamente con `n`.

El QuadTree reduce las comparaciones al consultar solo regiones cercanas a cada particula. En distribuciones uniformes suele revisar pocos candidatos por particula. En distribuciones con clusters o alta densidad revisa mas candidatos, porque muchas particulas caen en regiones pequenas, pero aun asi evita comparar contra todo el conjunto.

Esto permite demostrar la utilidad practica del QuadTree para simulaciones 2D con muchas particulas.
