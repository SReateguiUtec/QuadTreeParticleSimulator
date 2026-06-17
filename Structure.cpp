#include "Structure.h"
#include <cmath>
#include <iostream>
using namespace std;

static double maxParticleRadius(const vector<Particle>& particles) {
    double maxRadius = 0;

    for (const Particle& p : particles) {
        if (p.radius > maxRadius) {
            maxRadius = p.radius;
        }
    }

    return maxRadius;
}


QuadTree::QuadTree(Rectangle boundary, int capacity) {
    this->boundary = boundary;
    this->capacity = capacity < 1 ? 1 : capacity;
    this->divided = false;

    this->northeast = nullptr;
    this->northwest = nullptr;
    this->southeast = nullptr;
    this->southwest = nullptr;
}

bool Rectangle::contains(const Particle& p) const {
    return p.x >= x - w / 2 &&
           p.x <= x + w / 2 &&
           p.y >= y - h / 2 &&
           p.y <= y + h / 2;
}

bool Rectangle::intersects(const Rectangle& other) const {
    return !(other.x - other.w / 2 > x + w / 2 ||
             other.x + other.w / 2 < x - w / 2 ||
             other.y - other.h / 2 > y + h / 2 ||
             other.y + other.h / 2 < y - h / 2);
}

static bool pointInsideCircle(
    double pointX,
    double pointY,
    double centerX,
    double centerY,
    double radius
) {
    double dx = pointX - centerX;
    double dy = pointY - centerY;
    return dx * dx + dy * dy <= radius * radius;
}

bool QuadTree::insert(const Particle& p) {
    if (!boundary.contains(p)) {
        return false;
    }

    if (particles.size() < static_cast<size_t>(capacity) && !divided) {
        particles.push_back(p);
        return true;
    }

    if (!divided) {
        subdivide();
    }

    if (northeast->insert(p)) return true;
    if (northwest->insert(p)) return true;
    if (southeast->insert(p)) return true;
    if (southwest->insert(p)) return true;

    return false;
}

void QuadTree::subdivide() {
    double x = boundary.x;
    double y = boundary.y;
    double w = boundary.w / 2;
    double h = boundary.h / 2;

    Rectangle ne {x + w / 2, y - h / 2, w, h};
    Rectangle nw {x - w / 2, y - h / 2, w, h};
    Rectangle se {x + w / 2, y + h / 2, w, h};
    Rectangle sw {x - w / 2, y + h / 2, w, h};

    northeast = new QuadTree(ne, capacity);
    northwest = new QuadTree(nw, capacity);
    southeast = new QuadTree(se, capacity);
    southwest = new QuadTree(sw, capacity);

    divided = true;

    vector<Particle> oldParticles = particles;
    particles.clear();

    for (const Particle& p : oldParticles) {
        insert(p);
    }
}
QuadTree::~QuadTree() {
    delete northeast;
    delete northwest;
    delete southeast;
    delete southwest;
}
void QuadTree::queryRange(const Rectangle& range, vector<Particle>& found) const {
    QueryStats stats {0, 0, 0};
    queryRange(range, found, stats);
}

void QuadTree::queryRange(const Rectangle& range, vector<Particle>& found, QueryStats& stats) const {
    stats.nodesVisited++;

    if (!boundary.intersects(range)) {
        return;
    }

    for (const Particle& p : particles) {
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

vector<Particle> QuadTree::queryNearPoint(double x, double y, double radius) const {
    QueryStats stats {0, 0, 0};
    return queryNearPoint(x, y, radius, stats);
}

vector<Particle> QuadTree::queryNearPoint(double x, double y, double radius, QueryStats& stats) const {
    Rectangle searchArea {x, y, radius * 2, radius * 2};
    vector<Particle> candidates;
    vector<Particle> nearby;

    queryRange(searchArea, candidates, stats);

    for (const Particle& candidate : candidates) {
        if (pointInsideCircle(candidate.x, candidate.y, x, y, radius)) {
            nearby.push_back(candidate);
        }
    }

    return nearby;
}

void QuadTree::collectBoundaries(vector<Rectangle>& boundaries) const {
    boundaries.push_back(boundary);

    if (divided) {
        northeast->collectBoundaries(boundaries);
        northwest->collectBoundaries(boundaries);
        southeast->collectBoundaries(boundaries);
        southwest->collectBoundaries(boundaries);
    }
}

void updateParticles(vector<Particle>& particles, double width, double height) {
    for (Particle& p : particles) {
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


double distanceBetween(const Particle& a, const Particle& b) {
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return sqrt(dx * dx + dy * dy);
}

bool areColliding(const Particle& a, const Particle& b) {
    return distanceBetween(a, b) <= a.radius + b.radius;
}

CollisionStats bruteForceCollisionStats(const vector<Particle>& particles) {
    CollisionStats stats {0, 0, 0, 0, 0};

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

CollisionStats quadTreeCollisionStats(QuadTree& qt, const vector<Particle>& particles) {
    CollisionStats stats {0, 0, 0, 0, 0};
    double maxRadius = maxParticleRadius(particles);

    for (const Particle& p : particles) {
        double searchSize = 2 * (p.radius + maxRadius);
        Rectangle searchArea {p.x, p.y, searchSize, searchSize};
        vector<Particle> candidates;
        QueryStats queryStats {0, 0, 0};

        qt.queryRange(searchArea, candidates, queryStats);
        stats.nodesVisited += queryStats.nodesVisited;
        stats.particlesChecked += queryStats.particlesChecked;
        stats.candidatesFound += queryStats.candidatesFound;

        for (const Particle& candidate : candidates) {
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

int bruteForceCollisions(const vector<Particle>& particles) {
    return bruteForceCollisionStats(particles).collisions;
}

int quadTreeCollisions(QuadTree& qt, const vector<Particle>& particles) {
    return quadTreeCollisionStats(qt, particles).collisions;
}
