#include "Visualization.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <set>
#include <vector>
#include "Experiment.h"
#include "Structure.h"

using namespace std;

struct CollisionPair {
    Particle first;
    Particle second;
};

static vector<CollisionPair> findCollisionPairs(const vector<Particle>& particles) {
    vector<CollisionPair> collisions;

    for (size_t i = 0; i < particles.size(); i++) {
        for (size_t j = i + 1; j < particles.size(); j++) {
            if (areColliding(particles[i], particles[j])) {
                collisions.push_back({particles[i], particles[j]});
            }
        }
    }

    return collisions;
}

static set<int> collectCollidingIds(const vector<CollisionPair>& collisions) {
    set<int> ids;

    for (const CollisionPair& collision : collisions) {
        ids.insert(collision.first.id);
        ids.insert(collision.second.id);
    }

    return ids;
}

static double clampCoordinate(double value, double minValue, double maxValue) {
    return max(minValue, min(value, maxValue));
}

static vector<Particle> generateGalaxyParticles(
    int particleCount,
    double width,
    double height,
    double particleRadius
) {
    vector<Particle> particles;
    mt19937 generator(9090);
    uniform_real_distribution<double> unitDistribution(0.0, 1.0);
    normal_distribution<double> noiseDistribution(0.0, 10.0);
    int firstGalaxyCount = particleCount / 2;
    double galaxyRadius = min(width, height) * 0.18;
    double leftCenterX = width * 0.34;
    double rightCenterX = width * 0.66;
    double centerY = height * 0.50;

    particles.reserve(particleCount);

    for (int i = 0; i < particleCount; i++) {
        bool leftGalaxy = i < firstGalaxyCount;
        int localIndex = leftGalaxy ? i : i - firstGalaxyCount;
        double centerX = leftGalaxy ? leftCenterX : rightCenterX;
        double spinDirection = leftGalaxy ? 1.0 : -1.0;
        double driftDirection = leftGalaxy ? 0.55 : -0.55;
        double distance = galaxyRadius * sqrt(unitDistribution(generator));
        double angle = localIndex * 0.42 + distance * 0.035;
        double armWave = sin(angle * 2.0) * 9.0;
        double x = centerX + cos(angle) * (distance + armWave) + noiseDistribution(generator);
        double y = centerY + sin(angle) * (distance * 0.72 + armWave) + noiseDistribution(generator);
        double tangentSpeed = 0.55 + distance / galaxyRadius * 1.15;
        double vx = driftDirection + spinDirection * (-sin(angle)) * tangentSpeed;
        double vy = spinDirection * cos(angle) * tangentSpeed * 0.72;

        particles.push_back({
            i + 1,
            clampCoordinate(x, particleRadius, width - particleRadius),
            clampCoordinate(y, particleRadius, height - particleRadius),
            vx,
            vy,
            particleRadius
        });
    }

    return particles;
}

static void writeSvgVisualization(
    const string& fileName,
    const vector<Particle>& particles,
    const vector<Rectangle>& boundaries,
    const vector<Particle>& queryCandidates,
    const vector<CollisionPair>& collisions,
    double width,
    double height,
    double queryX,
    double queryY,
    double queryRadius
) {
    ofstream file(fileName);
    set<int> candidateIds;
    set<int> collidingIds;

    for (const Particle& particle : queryCandidates) {
        candidateIds.insert(particle.id);
    }

    for (const CollisionPair& collision : collisions) {
        collidingIds.insert(collision.first.id);
        collidingIds.insert(collision.second.id);
    }

    file << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1200\" height=\"1100\" viewBox=\"0 0 1200 1100\">\n";
    file << "<rect width=\"1200\" height=\"1100\" fill=\"#f8fafc\"/>\n";
    file << "<text x=\"30\" y=\"35\" font-family=\"Arial\" font-size=\"24\" fill=\"#111827\">Visualizacion QuadTree - particulas, subdivisiones, consulta y colisiones</text>\n";
    file << "<text x=\"30\" y=\"65\" font-family=\"Arial\" font-size=\"15\" fill=\"#374151\">Azul: particulas | Naranja: candidatos de la consulta | Rojo: particulas en colision | Morado: radio de consulta | Lineas grises: nodos del QuadTree</text>\n";
    file << "<g transform=\"translate(50, 80)\">\n";
    file << "<rect x=\"0\" y=\"0\" width=\"" << width << "\" height=\"" << height << "\" fill=\"#ffffff\" stroke=\"#111827\" stroke-width=\"2\"/>\n";

    for (const Rectangle& boundary : boundaries) {
        double x = boundary.x - boundary.w / 2;
        double y = boundary.y - boundary.h / 2;

        file << "<rect x=\"" << x << "\" y=\"" << y
             << "\" width=\"" << boundary.w
             << "\" height=\"" << boundary.h
             << "\" fill=\"none\" stroke=\"#9ca3af\" stroke-width=\"0.8\"/>\n";
    }

    file << "<circle cx=\"" << queryX
         << "\" cy=\"" << queryY
         << "\" r=\"" << queryRadius
         << "\" fill=\"#8b5cf6\" fill-opacity=\"0.08\" stroke=\"#7c3aed\" stroke-width=\"3\"/>\n";

    for (const CollisionPair& collision : collisions) {
        file << "<line x1=\"" << collision.first.x
             << "\" y1=\"" << collision.first.y
             << "\" x2=\"" << collision.second.x
             << "\" y2=\"" << collision.second.y
             << "\" stroke=\"#dc2626\" stroke-width=\"1.2\" stroke-opacity=\"0.55\"/>\n";
    }

    for (const Particle& particle : particles) {
        string fill = "#2563eb";
        string stroke = "#1e3a8a";
        double drawRadius = max(2.5, particle.radius * 0.75);

        if (candidateIds.count(particle.id) > 0) {
            fill = "#f59e0b";
            stroke = "#92400e";
            drawRadius = max(drawRadius, 5.0);
        }

        if (collidingIds.count(particle.id) > 0) {
            fill = "#ef4444";
            stroke = "#7f1d1d";
            drawRadius = max(drawRadius, 6.0);
        }

        file << "<circle cx=\"" << particle.x
             << "\" cy=\"" << particle.y
             << "\" r=\"" << drawRadius
             << "\" fill=\"" << fill
             << "\" fill-opacity=\"0.85\" stroke=\"" << stroke
             << "\" stroke-width=\"0.8\"/>\n";
    }

    file << "</g>\n";
    file << "<g transform=\"translate(50, 1045)\" font-family=\"Arial\" font-size=\"15\" fill=\"#111827\">\n";
    file << "<text x=\"0\" y=\"0\">Particulas: " << particles.size()
         << " | Nodos QuadTree: " << boundaries.size()
         << " | Candidatos en consulta: " << queryCandidates.size()
         << " | Colisiones visibles: " << collisions.size()
         << "</text>\n";
    file << "</g>\n";
    file << "</svg>\n";
}

void generateVisualization(
    double width,
    double height,
    double radius,
    int capacity,
    const string& fileName,
    int particleCount
) {
    double queryX = width / 2;
    double queryY = height / 2;
    double queryRadius = 170;
    Rectangle world {width / 2, height / 2, width, height};
    vector<Particle> particles = generateParticles(
        DistributionType::HighDensity,
        particleCount,
        width,
        height,
        radius,
        2026
    );

    QuadTree qt(world, capacity);

    for (const Particle& particle : particles) {
        qt.insert(particle);
    }

    QueryStats queryStats {0, 0, 0};
    vector<Particle> queryCandidates = qt.queryNearPoint(queryX, queryY, queryRadius, queryStats);
    vector<Rectangle> boundaries;
    vector<CollisionPair> collisions = findCollisionPairs(particles);

    qt.collectBoundaries(boundaries);

    writeSvgVisualization(
        fileName,
        particles,
        boundaries,
        queryCandidates,
        collisions,
        width,
        height,
        queryX,
        queryY,
        queryRadius
    );

    cout << endl;
    cout << "Visualizacion generada en: " << fileName << endl;
    cout << "Consulta visual -> nodos visitados: " << queryStats.nodesVisited
         << ", particulas revisadas: " << queryStats.particlesChecked
         << ", candidatos dentro del radio: " << queryCandidates.size()
         << endl;
}

void generateVisualizationFrames(
    double width,
    double height,
    double radius,
    int capacity,
    const string& fileName,
    int particleCount,
    int totalFrames
) {
    double queryRadius = 170;
    Rectangle world {width / 2, height / 2, width, height};
    vector<Particle> particles = generateGalaxyParticles(
        particleCount,
        width,
        height,
        radius
    );
    ofstream file(fileName);

    file << fixed << setprecision(3);
    file << "window.visualizationData = {\n";
    file << "  width: " << width << ",\n";
    file << "  height: " << height << ",\n";
    file << "  particleRadius: " << radius << ",\n";
    file << "  particleCount: " << particleCount << ",\n";
    file << "  capacity: " << capacity << ",\n";
    file << "  queryRadius: " << queryRadius << ",\n";
    file << "  scenario: \"galaxy_collision\",\n";
    file << "  frames: [\n";

    for (int frame = 0; frame < totalFrames; frame++) {
        updateParticles(particles, width, height);

        double queryX = width / 2 + 145.0 * sin(frame * 0.08);
        double queryY = height / 2 + 110.0 * cos(frame * 0.06);
        QuadTree qt(world, capacity);

        for (const Particle& particle : particles) {
            qt.insert(particle);
        }

        QueryStats queryStats {0, 0, 0};
        vector<Particle> queryCandidates = qt.queryNearPoint(queryX, queryY, queryRadius, queryStats);
        vector<Rectangle> boundaries;
        vector<CollisionPair> collisions = findCollisionPairs(particles);
        set<int> candidateIds;
        set<int> collidingIds = collectCollidingIds(collisions);

        for (const Particle& particle : queryCandidates) {
            candidateIds.insert(particle.id);
        }

        qt.collectBoundaries(boundaries);

        file << "    {\n";
        file << "      index: " << frame << ",\n";
        file << "      query: { x: " << queryX << ", y: " << queryY << ", radius: " << queryRadius << " },\n";
        file << "      metrics: { nodesVisited: " << queryStats.nodesVisited
             << ", particlesChecked: " << queryStats.particlesChecked
             << ", candidates: " << queryCandidates.size()
             << ", collisions: " << collisions.size()
             << ", quadtreeNodes: " << boundaries.size()
             << " },\n";

        file << "      particles: [";
        for (size_t i = 0; i < particles.size(); i++) {
            const Particle& particle = particles[i];

            if (i > 0) {
                file << ",";
            }

            file << "{id:" << particle.id
                 << ",x:" << particle.x
                 << ",y:" << particle.y
                 << ",r:" << particle.radius
                 << ",candidate:" << (candidateIds.count(particle.id) > 0 ? "true" : "false")
                 << ",colliding:" << (collidingIds.count(particle.id) > 0 ? "true" : "false")
                 << "}";
        }
        file << "],\n";

        file << "      boundaries: [";
        for (size_t i = 0; i < boundaries.size(); i++) {
            const Rectangle& boundary = boundaries[i];
            double x = boundary.x - boundary.w / 2;
            double y = boundary.y - boundary.h / 2;

            if (i > 0) {
                file << ",";
            }

            file << "{x:" << x
                 << ",y:" << y
                 << ",w:" << boundary.w
                 << ",h:" << boundary.h
                 << "}";
        }
        file << "],\n";

        file << "      collisions: [";
        for (size_t i = 0; i < collisions.size(); i++) {
            const CollisionPair& collision = collisions[i];

            if (i > 0) {
                file << ",";
            }

            file << "{a:" << collision.first.id
                 << ",b:" << collision.second.id
                 << "}";
        }
        file << "]\n";
        file << "    }";

        if (frame + 1 < totalFrames) {
            file << ",";
        }

        file << "\n";
    }

    file << "  ]\n";
    file << "};\n";

    cout << "Datos para front generados en: " << fileName << endl;
}
