#include "Structure.h"

#include <cmath>
#include <iostream>
#include <sstream>

using namespace std;

static double maxParticleRadius(const vector<Particle> &particles) {
  double maxRadius = 0;

  for (const Particle &p : particles) {
    if (p.radius > maxRadius) {
      maxRadius = p.radius;
    }
  }

  return maxRadius;
}

bool Rectangle::contains(const Particle &p) const {
  return p.x >= x - w / 2 && p.x < x + w / 2 && p.y >= y - h / 2 &&
         p.y < y + h / 2;
}

bool Rectangle::intersects(const Rectangle &other) const {
  return !(
      other.x - other.w / 2 > x + w / 2 || other.x + other.w / 2 < x - w / 2 ||
      other.y - other.h / 2 > y + h / 2 || other.y + other.h / 2 < y - h / 2);
}

static bool pointInsideCircle(double pointX, double pointY, double centerX,
                              double centerY, double radius) {
  double dx = pointX - centerX;
  double dy = pointY - centerY;
  return dx * dx + dy * dy <= radius * radius;
}

QuadNode::QuadNode(Rectangle boundary) {
  this->boundary = boundary;
  this->divided = false;

  this->northeast = nullptr;
  this->northwest = nullptr;
  this->southeast = nullptr;
  this->southwest = nullptr;
}

QuadNode::~QuadNode() {
  delete northeast;
  delete northwest;
  delete southeast;
  delete southwest;
}

//////////////////////////////
// Funcion minima: insertar objetos en el QuadTree.
//////////////////////////////
bool QuadNode::insert(const Particle &p, int capacity) {
  if (!boundary.contains(p)) {
    return false;
  }

  if (particles.size() < static_cast<size_t>(capacity) && !divided) {
    particles.push_back(p);
    return true;
  }

  if (!divided) {
    subdivide(capacity);
  }

  if (northeast->insert(p, capacity))
    return true;
  if (northwest->insert(p, capacity))
    return true;
  if (southeast->insert(p, capacity))
    return true;
  if (southwest->insert(p, capacity))
    return true;

  return false;
}

//////////////////////////////
// Funcion minima: subdividir regiones cuando se supera la capacidad maxima.
//////////////////////////////
void QuadNode::subdivide(int capacity) {
  double x = boundary.x;
  double y = boundary.y;
  double w = boundary.w / 2;
  double h = boundary.h / 2;

  Rectangle ne{x + w / 2, y - h / 2, w, h};
  Rectangle nw{x - w / 2, y - h / 2, w, h};
  Rectangle se{x + w / 2, y + h / 2, w, h};
  Rectangle sw{x - w / 2, y + h / 2, w, h};

  northeast = new QuadNode(ne);
  northwest = new QuadNode(nw);
  southeast = new QuadNode(se);
  southwest = new QuadNode(sw);

  divided = true;

  vector<Particle> oldParticles = particles;
  particles.clear();

  for (const Particle &p : oldParticles) {
    insert(p, capacity);
  }
}

//////////////////////////////
// Funcion minima: consultar objetos dentro de una region rectangular.
//////////////////////////////
void QuadNode::queryRange(const Rectangle &range, vector<Particle> &found,
                          QueryStats &stats) const {
  stats.nodesVisited++;

  if (!boundary.intersects(range)) {
    return;
  }

  for (const Particle &p : particles) {
    stats.particlesChecked++;

    if (range.contains(p)) {
      found.push_back(p);
      stats.candidatesFound++;
    }
  }

  if (divided) {
    northeast->queryRange(range, found, stats);
    northwest->queryRange(range, found, stats);
    southeast->queryRange(range, found, stats);
    southwest->queryRange(range, found, stats);
  }
}

void QuadNode::collectBoundaries(vector<Rectangle> &boundaries) const {
  boundaries.push_back(boundary);

  if (divided) {
    northeast->collectBoundaries(boundaries);
    northwest->collectBoundaries(boundaries);
    southeast->collectBoundaries(boundaries);
    southwest->collectBoundaries(boundaries);
  }
}

void QuadNode::clear() {
  particles.clear();
  divided = false;
  if (northeast != nullptr) {
    northeast->clear();
    northwest->clear();
    southeast->clear();
    southwest->clear();
  }
}

void QuadNode::serialize(std::ostringstream &os) const {
  os << "{\"divided\":" << (divided ? "true" : "false")
     << ",\"particles\":" << particles.size();
  if (divided) {
    os << ",\"children\":[";
    northeast->serialize(os);
    os << ",";
    northwest->serialize(os);
    os << ",";
    southeast->serialize(os);
    os << ",";
    southwest->serialize(os);
    os << "]";
  }
  os << "}";
}

QuadTree::QuadTree(Rectangle boundary, int capacity) {
  this->boundary = boundary;
  this->capacity = capacity < 1 ? 1 : capacity;
  this->root = new QuadNode(boundary);
}

QuadTree::~QuadTree() {
  delete root;
}

bool QuadTree::insert(const Particle &p) {
  return root->insert(p, capacity);
}

void QuadTree::queryRange(const Rectangle &range,
                          vector<Particle> &found) const {
  QueryStats stats{0, 0, 0};
  queryRange(range, found, stats);
}

void QuadTree::queryRange(const Rectangle &range, vector<Particle> &found,
                          QueryStats &stats) const {
  root->queryRange(range, found, stats);
}

//////////////////////////////
// Funcion minima: consultar objetos cercanos a un punto.
//////////////////////////////
vector<Particle> QuadTree::queryNearPoint(double x, double y,
                                          double radius) const {
  QueryStats stats{0, 0, 0};
  return queryNearPoint(x, y, radius, stats);
}

vector<Particle> QuadTree::queryNearPoint(double x, double y, double radius,
                                          QueryStats &stats) const {
  Rectangle searchArea{x, y, radius * 2, radius * 2};
  vector<Particle> candidates;
  vector<Particle> nearby;

  queryRange(searchArea, candidates, stats);

  for (const Particle &candidate : candidates) {
    if (pointInsideCircle(candidate.x, candidate.y, x, y, radius)) {
      nearby.push_back(candidate);
    }
  }

  return nearby;
}

void QuadTree::collectBoundaries(vector<Rectangle> &boundaries) const {
  root->collectBoundaries(boundaries);
}

void QuadTree::clear() {
  root->clear();
}

void QuadTree::serialize(std::ostringstream &os) const {
  root->serialize(os);
}

//////////////////////////////
// Funcion minima: actualizar posiciones de objetos frame por frame.
//////////////////////////////
void updateParticles(vector<Particle> &particles, double width, double height) {
  for (Particle &p : particles) {
    p.x += p.vx;
    p.y += p.vy;

    if (p.x - p.radius < 0 || p.x + p.radius > width) {
      p.vx *= -1;
    }

    if (p.y - p.radius < 0 || p.y + p.radius > height) {
      p.vy *= -1;
    }
  }
}

double distanceBetween(const Particle &a, const Particle &b) {
  double dx = a.x - b.x;
  double dy = a.y - b.y;
  return sqrt(dx * dx + dy * dy);
}

bool areColliding(const Particle &a, const Particle &b) {
  return distanceBetween(a, b) <= a.radius + b.radius;
}

//////////////////////////////
// Funcion minima: solucion ingenua de fuerza bruta para comparar resultados.
//////////////////////////////
CollisionStats bruteForceCollisionStats(const vector<Particle> &particles) {
  CollisionStats stats{0, 0, 0, 0, 0};

  for (size_t i = 0; i < particles.size(); i++) {
    for (size_t j = i + 1; j < particles.size(); j++) {
      stats.comparisons++;

      if (areColliding(particles[i], particles[j])) {
        stats.collisions++;
      }
    }
  }

  return stats;
}

//////////////////////////////
// Funcion minima: detectar posibles colisiones usando el QuadTree.
//////////////////////////////
CollisionStats quadTreeCollisionStats(QuadTree &qt,
                                      const vector<Particle> &particles) {
  CollisionStats stats{0, 0, 0, 0, 0};
  double maxRadius = maxParticleRadius(particles);

  for (const Particle &p : particles) {
    double searchSize = 2 * (p.radius + maxRadius);
    Rectangle searchArea{p.x, p.y, searchSize, searchSize};
    vector<Particle> candidates;
    QueryStats queryStats{0, 0, 0};

    qt.queryRange(searchArea, candidates, queryStats);
    stats.nodesVisited += queryStats.nodesVisited;
    stats.particlesChecked += queryStats.particlesChecked;
    stats.candidatesFound += queryStats.candidatesFound;

    for (const Particle &candidate : candidates) {
      if (candidate.id <= p.id) {
        continue;
      }

      stats.comparisons++;

      if (areColliding(p, candidate)) {
        stats.collisions++;
      }
    }
  }

  return stats;
}

int bruteForceCollisions(const vector<Particle> &particles) {
  return bruteForceCollisionStats(particles).collisions;
}

int quadTreeCollisions(QuadTree &qt, const vector<Particle> &particles) {
  return quadTreeCollisionStats(qt, particles).collisions;
}
