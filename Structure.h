#ifndef STRUCTURE_H
#define STRUCTURE_H

#include <sstream>
#include <vector>
using namespace std;

struct Particle {
  int id;
  double x, y;
  double vx, vy;
  double radius;
};

struct Rectangle {
  double x, y;
  double w, h;
  bool contains(const Particle &p) const;
  bool intersects(const Rectangle &other) const;
};

struct QueryStats {
  long long nodesVisited;
  long long particlesChecked;
  long long candidatesFound;
};

struct CollisionStats {
  int collisions;         // cantidad ade coliciones encontradas
  long long comparisons;  // cantidad de compraraciones entre particulas para
                          // determinar si tienen coliciones
  long long nodesVisited; // cantidad de nodos visitados en la consulta
  long long
      particlesChecked; // canridad de particulas revisadas dentro de los nodos
  long long candidatesFound; // posibles particulas a colicionar
};

class QuadNode {
public:
  Rectangle boundary;         // region del espacio que controla
  vector<Particle> particles; // particulas entro del espacio del nodo
  bool divided;               // si el nodo ya se dividio o no
  // hijos del nodo
  QuadNode *northeast;
  QuadNode *northwest;
  QuadNode *southeast;
  QuadNode *southwest;

  QuadNode(Rectangle boundary);
  ~QuadNode();

  bool insert(const Particle &p, int capacity);
  void subdivide(int capacity);
  void queryRange(const Rectangle &range, vector<Particle> &found,
                  QueryStats &stats) const; // consultar datos dentro del area
  void collectBoundaries(vector<Rectangle> &boundaries)
      const; // obtener los rectangulos para poder dibujarlos en el front
  void clear();
  void serialize(std::ostringstream &os)
      const; // convertir informacion del nodo en texto y enviarlo al front
};

class QuadTree {
private:
  QuadNode *root;
  int capacity;
  Rectangle boundary; // area mayor del arbol

public:
  QuadTree(Rectangle boundary, int capacity);
  ~QuadTree();

  bool insert(const Particle &p); // inserta una particula en el nodo raiz
  void queryRange(const Rectangle &range, vector<Particle> &found)
      const; // consulta dentro del area sin metricas
  void
  queryRange(const Rectangle &range, vector<Particle> &found,
             QueryStats &stats) const; // consulta dentro del area con metricas

  vector<Particle> queryNearPoint(double x, double y, double radius)
      const; // consulta dentro del area cercanas a un punto sin metricas
  vector<Particle> queryNearPoint(double x, double y, double radius,
                                  QueryStats &stats)
      const; // consulta dentro del area cercanas a un punto con metricas

  void collectBoundaries(vector<Rectangle> &boundaries) const;
  void clear();
  void serialize(std::ostringstream &os) const; // convertir los datos a texto
};
// funciones universales
void updateParticles(
    vector<Particle> &particles, double width,
    double height); // actualiza la posicion de todas las particulas

double distanceBetween(const Particle &a,
                       const Particle &b); // distancia entre particulas
bool areColliding(
    const Particle &a,
    const Particle &b); // verifica si dos particulas estan colicionando

int bruteForceCollisions(
    const vector<Particle> &particles); // version de fuerza bruta para calcular
                                        // la cantidad de coliciones que hay
int quadTreeCollisions(
    QuadTree &qt,
    const vector<Particle>
        &particles); // version de Quaddtree para calcular las coliciones
CollisionStats bruteForceCollisionStats(
    const vector<Particle> &particles); // BF coliciones+stats
CollisionStats quadTreeCollisionStats(
    QuadTree &qt, const vector<Particle> &particles); // QT coliciones+stats

#endif