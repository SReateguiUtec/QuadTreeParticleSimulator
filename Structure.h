#ifndef STRUCTURE_H
#define STRUCTURE_H

#include <vector>
using namespace std;

struct Particle {
    int id;
    double x, y; // posición
    double vx, vy; // velocidad
    double radius; // radio angulo de giro
};

struct Rectangle {
    double x, y;
    double w, h;

    bool contains(const Particle& p) const;
    bool intersects(const Rectangle& other) const;
};

struct QueryStats {
    long long nodesVisited;
    long long particlesChecked;
    long long candidatesFound;
};

struct CollisionStats {
    int collisions;
    long long comparisons;
    long long nodesVisited;
    long long particlesChecked;
    long long candidatesFound;
};
/*
    Idea
1. Tengo un rectángulo grande: el mundo completo.
2. Inserto puntos o partículas.
3. Si la cantidad de partículas supera la capacidad del nodo, divido el rectángulo en 4.
4. Reviso la posición x, y de cada partícula.
5. La mando al cuadrante correspondiente.
6. Si un cuadrante también se llena, ese cuadrante se vuelve a dividir en 4.
 */



class QuadTree {
private:
    Rectangle boundary;
    int capacity;
    vector<Particle> particles;
    bool divided;

    QuadTree* northeast;
    QuadTree* northwest;
    QuadTree* southeast;
    QuadTree* southwest;

public:
    QuadTree(Rectangle boundary, int capacity);
    ~QuadTree();
    bool insert(const Particle& p);
    void subdivide();
    void queryRange(const Rectangle& range, vector<Particle>& found) const;
    void queryRange(const Rectangle& range, vector<Particle>& found, QueryStats& stats) const;
    vector<Particle> queryNearPoint(double x, double y, double radius) const;
    vector<Particle> queryNearPoint(double x, double y, double radius, QueryStats& stats) const;
    void collectBoundaries(vector<Rectangle>& boundaries) const;
};

void updateParticles(vector<Particle>& particles, double width, double height);

double distanceBetween(const Particle& a, const Particle& b);
bool areColliding(const Particle& a, const Particle& b);

int bruteForceCollisions(const vector<Particle>& particles);
int quadTreeCollisions(QuadTree& qt, const vector<Particle>& particles);
CollisionStats bruteForceCollisionStats(const vector<Particle>& particles);
CollisionStats quadTreeCollisionStats(QuadTree& qt, const vector<Particle>& particles);

#endif
