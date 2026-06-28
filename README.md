# <img src="frontend/public/favicon.svg" width="35" align="top"> QuadTree para simulación de partículas

Un motor físico de colisiones de partículas 2D de alto rendimiento. Implementa un **QuadTree** desde cero para acelerar la detección de posibles colisiones entre partículas circulares en un espacio 2D. La idea principal es comparar empíricamente el enfoque con QuadTree contra una solución ingenua de fuerza bruta que revisa todos los pares de partículas.

El proyecto corresponde a la **opción A (QuadTree)** del Proyecto 2 del curso de Algoritmos y Estructuras de Datos.

Utiliza una arquitectura híbrida donde el backend en **C++** calcula la física y la estructura del árbol espacial usando Server-Sent Events (SSE) para enviar la simulación en tiempo real a una interfaz visual construida en **Vue.js + Tailwind CSS**.

---

## Características Principales

- **Motor Físico en C++:** Implementación desde cero de un QuadTree para detección de vecinos O(log N) frente a O(N²) de fuerza bruta.
- **Simulación Interactiva en Tiempo Real:** Las partículas se pueden insertar haciendo click directamente en el canvas web.
- **Streaming SSE (Server-Sent Events):** C++ calcula un frame y lo empuja al cliente web instantáneamente, permitiendo simulación infinita y continua (~30 FPS controlados por backend).
- **Métricas en Vivo:** El dashboard compara en tiempo real las operaciones (comparaciones y tiempos) realizadas por el QuadTree vs la Fuerza Bruta.
- **Múltiples Distribuciones Espaciales:** Uniforme, clusters, zona de alta densidad y galaxia para probar escenarios con diferente densidad.

---

## Cómo ejecutar el proyecto

### 1. Compilar y correr el servidor C++
El motor en C++ utiliza la librería `httplib` para montar el servidor web y abrir el stream SSE.

```bash
# Compilar todos los archivos fuente
g++ -std=c++20 Main.cpp Structure.cpp Visualization.cpp Experiment.cpp -o server

# Ejecutar el servidor (escuchará en el puerto 8080)
./server
```
*(Nota: En CLion u otros IDEs también puedes ejecutarlo directamente usando el `CMakeLists.txt` incluido).*

### 2. Iniciar el Frontend (Vue.js)
Asegúrate de tener Node.js instalado. En otra terminal, entra a la carpeta del frontend y levanta el servidor de desarrollo Vite.

```bash
cd frontend
npm install
npm run dev
```

Abre en tu navegador la URL que indique Vite (usualmente `http://localhost:5173`).

---

## 🎮 Uso de la Interfaz

1. **Modo Interactivo (Click):** Con el modo click activado, toca en cualquier parte del canvas para insertar una nueva partícula. Verás el QuadTree subdividirse automáticamente para acomodarla.
2. **Scrubber (Línea de tiempo):** La simulación mantiene un buffer de los últimos 300 frames. Puedes arrastrar el slider inferior para retroceder en el tiempo. Vuelve a hacer click en `▶ LIVE` para regresar al stream en vivo.
3. **Distribuciones masivas:** Usa el panel superior para inyectar cientos de partículas a la vez en distribuciones uniformes, clusters, o alta densidad.
4. **Vista de Organigrama:** Habilita este switch para observar un trazado ortogonal estático que representa la topología en forma de árbol (Raíz → Nodos → Partículas) del QuadTree en el frame actual.

---

## Implementación y Algoritmos

### Modelo de partícula
```cpp
struct Particle {
    int id;
    double x, y;
    double vx, vy;
    double radius;
};
```
Cada partícula tiene posición, velocidad y radio. Dos partículas colisionan si la distancia euclidiana entre sus centros es menor o igual a la suma de sus radios. En cada frame, se actualiza la posición y rebotan contra los bordes del mundo.

### Cómo funciona el QuadTree
El QuadTree divide recursivamente el espacio 2D en cuatro regiones (Noreste, Noroeste, Sureste, Suroeste).
Cada nodo almacena partículas. Si se supera la `capacidad máxima`, el nodo se subdivide y las partículas se redistribuyen en los hijos. Se reconstruye o limpia el árbol por completo en cada frame de la simulación.

La consulta (`queryRange`) evita revisar regiones que no intersectan con el área consultada, lo que filtra drásticamente las partículas candidatas a colisionar frente a una búsqueda lineal.

### Operaciones, Invariantes y Complejidad
Para garantizar el correcto funcionamiento del QuadTree, se mantienen los siguientes **invariantes**:
1. **Conservación espacial:** Toda partícula contenida en un nodo hijo está estrictamente contenida dentro de los límites espaciales del nodo padre.
2. **Capacidad acotada:** Un nodo hoja contiene como máximo un número de partículas igual a su `capacidad máxima`. Si se excede, el nodo se convierte en un nodo interno (sin partículas propias) y da a luz a exactamente 4 hijos (Noroeste, Noreste, Suroeste, Sureste).

| Operación | Descripción | Complejidad Promedio | Peor Caso |
| :--- | :--- | :--- | :--- |
| **Insertar** (`insert`) | Navega recursivamente por el árbol hasta encontrar la hoja correspondiente e inserta la partícula. | $O(\log N)$ | $O(N)$ (todas en el mismo punto) |
| **Subdividir** (`subdivide`) | Divide un nodo saturado en 4 cuadrantes y redistribuye sus partículas. | $O(1)$ | $O(1)$ |
| **Consulta Rectangular** (`queryRange`) | Retorna todas las partículas dentro de un MBR (Minimum Bounding Rectangle), podando las ramas que no intersectan. | $O(\log N + k)$ | $O(N)$ |
| **Consulta Circular** (`queryNearPoint`) | Filtra las partículas devueltas por `queryRange` comprobando que la distancia euclidiana sea menor al radio de búsqueda. | $O(\log N + k)$ | $O(N)$ |
| **Reconstrucción / Actualización** | En lugar de mover partículas entre ramas, el árbol se destruye y reconstruye desde cero cada frame con las nuevas posiciones actualizadas. | $O(N \log N)$ | $O(N^2)$ |
| **Detección de Colisiones** | Búsqueda de vecinos cercanos para todas las partículas. | $O(N \log N)$ | $O(N^2)$ |

*(Nota: $k$ representa el número de partículas encontradas dentro de la región de consulta).*

### Detección de colisiones: QuadTree vs Fuerza Bruta
- **Fuerza bruta:** Compara cada partícula contra todas las demás (`n * (n - 1) / 2` comparaciones). Crece cuadráticamente con $N$.
- **QuadTree:** Para cada partícula se crea una región (bounding box) alrededor de ella equivalente a su radio de alcance. Se consulta al QuadTree para obtener *sólo* las partículas candidatas cercanas. Finalmente, se aplica la fórmula de distancia exacta sólo a esos candidatos.

Ambos métodos encuentran **exactamente las mismas colisiones**, pero el QuadTree lo logra realizando drásticamente menos comparaciones (distancia euclidiana).

---

## Experimentos y Distribuciones

Se implementaron distintas distribuciones para poner a prueba el balanceo del QuadTree:
1. **Uniforme:** Partículas esparcidas aleatoriamente por todo el espacio.
2. **Clusters:** Grupos de partículas aglomeradas en puntos aleatorios siguiendo una distribución normal.
3. **Alta densidad:** El 70% de las partículas apretadas en el centro (20% del espacio), y el 30% esparcido. 

### Interpretación de Resultados
La fuerza bruta siempre revisa todos los pares, por lo que su tiempo colapsa rápidamente. 
El QuadTree brilla en distribuciones uniformes donde la poda espacial es máxima. En escenarios de **clusters o alta densidad**, el QuadTree revisa más candidatos (porque muchas partículas se apilan en la misma región), lo que incrementa la profundidad del árbol y las colisiones reales, pero **incluso en el peor caso**, evita comparar contra el mundo entero, demostrando su enorme utilidad práctica para motores de físicas.

---

## Estructura de archivos

```text
.
├── CMakeLists.txt
├── Main.cpp                # Servidor httplib y endpoints REST/SSE
├── Structure.h / .cpp      # QuadTree core, física matemática y fuerza bruta
├── Visualization.h / .cpp  # Bucle del motor (Loop simulation) y empaquetado a JSON
├── Experiment.h / .cpp     # Benchmarks automatizados y generación estocástica de datos
├── frontend/               # Código fuente del Frontend moderno (Vue 3 + Tailwind)
│   ├── index.html          # Punto de entrada y configuración de Favicon
│   ├── public/favicon.svg  
│   └── src/                
│       ├── App.vue         # Lógica de SSE, estado e UI del dashboard
│       └── components/     # QuadCanvas.vue, MetricsCard.vue, etc.
└── ProyectoFinal-AED.pdf   # Rúbrica del proyecto
```