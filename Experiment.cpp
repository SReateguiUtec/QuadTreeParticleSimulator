#include "Experiment.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>

using namespace std;
using namespace chrono;

static double randomVelocity(mt19937& generator) {
    uniform_real_distribution<double> velocityDistribution(-2.0, 2.0);
    return velocityDistribution(generator);
}

static double clampPosition(double value, double minValue, double maxValue) {
    return max(minValue, min(value, maxValue));
}

static Particle makeParticle(
    int id,
    double x,
    double y,
    double radius,
    mt19937& generator
) {
    return {
        id,
        x,
        y,
        randomVelocity(generator),
        randomVelocity(generator),
        radius
    };
}

//////////////////////////////
// Funcion minima: generar distribucion uniforme.
//////////////////////////////
static vector<Particle> generateUniformParticles(
    int n,
    double width,
    double height,
    double radius,
    mt19937& generator
) {
    vector<Particle> particles;
    uniform_real_distribution<double> xDistribution(radius, width - radius);
    uniform_real_distribution<double> yDistribution(radius, height - radius);

    particles.reserve(n);

    for (int i = 0; i < n; i++) {
        particles.push_back(makeParticle(
            i + 1,
            xDistribution(generator),
            yDistribution(generator),
            radius,
            generator
        ));
    }

    return particles;
}

//////////////////////////////
// Funcion minima: generar distribucion con clusters.
//////////////////////////////
static vector<Particle> generateClusterParticles(
    int n,
    double width,
    double height,
    double radius,
    mt19937& generator
) {
    vector<Particle> particles;
    vector<pair<double, double>> centers;
    int clusterCount = 5;
    double deviation = min(width, height) * 0.06;

    uniform_real_distribution<double> xCenterDistribution(width * 0.15, width * 0.85);
    uniform_real_distribution<double> yCenterDistribution(height * 0.15, height * 0.85);
    uniform_int_distribution<int> clusterDistribution(0, clusterCount - 1);
    normal_distribution<double> offsetDistribution(0.0, deviation);

    particles.reserve(n);
    centers.reserve(clusterCount);

    for (int i = 0; i < clusterCount; i++) {
        centers.push_back({
            xCenterDistribution(generator),
            yCenterDistribution(generator)
        });
    }

    for (int i = 0; i < n; i++) {
        const auto& center = centers[clusterDistribution(generator)];
        double x = clampPosition(
            center.first + offsetDistribution(generator),
            radius,
            width - radius
        );
        double y = clampPosition(
            center.second + offsetDistribution(generator),
            radius,
            height - radius
        );

        particles.push_back(makeParticle(i + 1, x, y, radius, generator));
    }

    return particles;
}

//////////////////////////////
// Funcion minima: generar distribucion con una zona de alta densidad.
//////////////////////////////
static vector<Particle> generateHighDensityParticles(
    int n,
    double width,
    double height,
    double radius,
    mt19937& generator
) {
    vector<Particle> particles;
    int denseCount = static_cast<int>(n * 0.70);
    double denseWidth = width * 0.20;
    double denseHeight = height * 0.20;
    double denseMinX = width / 2 - denseWidth / 2;
    double denseMaxX = width / 2 + denseWidth / 2;
    double denseMinY = height / 2 - denseHeight / 2;
    double denseMaxY = height / 2 + denseHeight / 2;

    uniform_real_distribution<double> denseXDistribution(denseMinX, denseMaxX);
    uniform_real_distribution<double> denseYDistribution(denseMinY, denseMaxY);
    uniform_real_distribution<double> worldXDistribution(radius, width - radius);
    uniform_real_distribution<double> worldYDistribution(radius, height - radius);

    particles.reserve(n);

    for (int i = 0; i < denseCount; i++) {
        particles.push_back(makeParticle(
            i + 1,
            denseXDistribution(generator),
            denseYDistribution(generator),
            radius,
            generator
        ));
    }

    for (int i = denseCount; i < n; i++) {
        particles.push_back(makeParticle(
            i + 1,
            worldXDistribution(generator),
            worldYDistribution(generator),
            radius,
            generator
        ));
    }

    return particles;
}

string distributionName(DistributionType distribution) {
    switch (distribution) {
        case DistributionType::Uniform:
            return "uniforme";
        case DistributionType::Clusters:
            return "clusters";
        case DistributionType::HighDensity:
            return "alta densidad";
    }

    return "desconocida";
}

vector<Particle> generateParticles(
    DistributionType distribution,
    int n,
    double width,
    double height,
    double radius,
    int seed
) {
    mt19937 generator(seed);

    switch (distribution) {
        case DistributionType::Uniform:
            return generateUniformParticles(n, width, height, radius, generator);
        case DistributionType::Clusters:
            return generateClusterParticles(n, width, height, radius, generator);
        case DistributionType::HighDensity:
            return generateHighDensityParticles(n, width, height, radius, generator);
    }

    return {};
}

//////////////////////////////
// Funcion minima: reconstruir el QuadTree por frame y comparar contra fuerza bruta.
//////////////////////////////
ExperimentResult runExperiment(
    DistributionType distribution,
    int particleCount,
    int frames,
    double width,
    double height,
    double radius,
    int capacity
) {
    Rectangle world {width / 2, height / 2, width, height};
    vector<Particle> particles = generateParticles(
        distribution,
        particleCount,
        width,
        height,
        radius,
        42 + particleCount
    );

    long long totalBuildTime = 0;
    long long totalQuadTreeTime = 0;
    long long totalBruteForceTime = 0;
    long long totalQuadTreeComparisons = 0;
    long long totalBruteForceComparisons = 0;
    long long totalQuadTreeNodesVisited = 0;
    long long totalQuadTreeParticlesChecked = 0;
    long long totalQuadTreeCandidatesFound = 0;
    long long totalCollisions = 0;

    for (int frame = 0; frame < frames; frame++) {
        updateParticles(particles, width, height);

        auto buildStart = high_resolution_clock::now();
        QuadTree qt(world, capacity);

        for (const Particle& p : particles) {
            qt.insert(p);
        }

        auto buildEnd = high_resolution_clock::now();

        auto quadStart = high_resolution_clock::now();
        CollisionStats quadStats = quadTreeCollisionStats(qt, particles);
        auto quadEnd = high_resolution_clock::now();

        auto bruteStart = high_resolution_clock::now();
        CollisionStats bruteStats = bruteForceCollisionStats(particles);
        auto bruteEnd = high_resolution_clock::now();

        if (quadStats.collisions != bruteStats.collisions) {
            cout << "Advertencia: resultados distintos en "
                 << distributionName(distribution)
                 << " n=" << particleCount
                 << " frame=" << frame
                 << ". QuadTree=" << quadStats.collisions
                 << ", fuerza bruta=" << bruteStats.collisions
                 << endl;
        }

        totalBuildTime += duration_cast<microseconds>(buildEnd - buildStart).count();
        totalQuadTreeTime += duration_cast<microseconds>(quadEnd - quadStart).count();
        totalBruteForceTime += duration_cast<microseconds>(bruteEnd - bruteStart).count();
        totalQuadTreeComparisons += quadStats.comparisons;
        totalBruteForceComparisons += bruteStats.comparisons;
        totalQuadTreeNodesVisited += quadStats.nodesVisited;
        totalQuadTreeParticlesChecked += quadStats.particlesChecked;
        totalQuadTreeCandidatesFound += quadStats.candidatesFound;
        totalCollisions += quadStats.collisions;
    }

    return {
        distributionName(distribution),
        particleCount,
        frames,
        static_cast<double>(totalBuildTime) / frames,
        static_cast<double>(totalQuadTreeTime) / frames,
        static_cast<double>(totalBruteForceTime) / frames,
        static_cast<double>(totalQuadTreeComparisons) / frames,
        static_cast<double>(totalBruteForceComparisons) / frames,
        static_cast<double>(totalQuadTreeNodesVisited) / frames,
        static_cast<double>(totalQuadTreeParticlesChecked) / frames,
        static_cast<double>(totalQuadTreeCandidatesFound) / frames,
        static_cast<double>(totalCollisions) / frames,
        static_cast<double>(totalQuadTreeComparisons) / frames / particleCount
    };
}

void printExperimentHeader(
    double width,
    double height,
    double radius,
    int capacity,
    int frames
) {
    cout << "Experimentos QuadTree vs fuerza bruta" << endl;
    cout << "Mundo: " << width << " x " << height
         << ", radio: " << radius
         << ", capacidad: " << capacity
         << ", frames por caso: " << frames << endl;
    cout << "Los tiempos estan expresados en microsegundos." << endl;
    cout << "Candidatos por particula = comparaciones promedio del QuadTree / numero de particulas." << endl;
    cout << "Nodos visitados, particulas revisadas y candidatos corresponden a las consultas internas del QuadTree." << endl;
    cout << endl;

    cout << left
         << setw(18) << "Distribucion"
         << right
         << setw(16) << "Particulas"
         << setw(10) << "Frames"
         << setw(30) << "Construccion QuadTree"
         << setw(30) << "Deteccion con QuadTree"
         << setw(32) << "Deteccion con fuerza bruta"
         << setw(32) << "Comparaciones QuadTree"
         << setw(34) << "Comparaciones fuerza bruta"
         << setw(30) << "Nodos visitados QuadTree"
         << setw(34) << "Particulas revisadas QuadTree"
         << setw(28) << "Candidatos QuadTree"
         << setw(22) << "Colisiones"
         << setw(34) << "Candidatos por particula"
         << endl;
}

void printResult(const ExperimentResult& result) {
    cout << left
         << setw(18) << result.distributionName
         << right
         << setw(16) << result.particleCount
         << setw(10) << result.frames
         << setw(30) << fixed << setprecision(0) << result.averageBuildTimeUs
         << setw(30) << fixed << setprecision(0) << result.averageQuadTreeTimeUs
         << setw(32) << fixed << setprecision(0) << result.averageBruteForceTimeUs
         << setw(32) << fixed << setprecision(0) << result.averageQuadTreeComparisons
         << setw(34) << fixed << setprecision(0) << result.averageBruteForceComparisons
         << setw(30) << fixed << setprecision(0) << result.averageQuadTreeNodesVisited
         << setw(34) << fixed << setprecision(0) << result.averageQuadTreeParticlesChecked
         << setw(28) << fixed << setprecision(0) << result.averageQuadTreeCandidatesFound
         << setw(22) << fixed << setprecision(0) << result.averageCollisions
         << setw(34) << fixed << setprecision(2) << result.averageCandidatesPerParticle
         << endl;
}

void writeResultsCsv(
    const string& fileName,
    const vector<ExperimentResult>& results
) {
    ofstream file(fileName);

    file << "distribucion,particulas,frames,construccion_quadtree_us,"
         << "deteccion_quadtree_us,deteccion_fuerza_bruta_us,"
         << "comparaciones_quadtree,comparaciones_fuerza_bruta,"
         << "nodos_visitados_quadtree,particulas_revisadas_quadtree,"
         << "candidatos_quadtree,colisiones,candidatos_por_particula\n";

    for (const ExperimentResult& result : results) {
        file << result.distributionName << ','
             << result.particleCount << ','
             << result.frames << ','
             << fixed << setprecision(2)
             << result.averageBuildTimeUs << ','
             << result.averageQuadTreeTimeUs << ','
             << result.averageBruteForceTimeUs << ','
             << result.averageQuadTreeComparisons << ','
             << result.averageBruteForceComparisons << ','
             << result.averageQuadTreeNodesVisited << ','
             << result.averageQuadTreeParticlesChecked << ','
             << result.averageQuadTreeCandidatesFound << ','
             << result.averageCollisions << ','
             << result.averageCandidatesPerParticle
             << '\n';
    }
}
