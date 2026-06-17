#ifndef EXPERIMENT_H
#define EXPERIMENT_H

#include <string>
#include <vector>
#include "Structure.h"

enum class DistributionType {
    Uniform,
    Clusters,
    HighDensity
};

struct ExperimentResult {
    std::string distributionName;
    int particleCount;
    int frames;
    double averageBuildTimeUs;
    double averageQuadTreeTimeUs;
    double averageBruteForceTimeUs;
    double averageQuadTreeComparisons;
    double averageBruteForceComparisons;
    double averageQuadTreeNodesVisited;
    double averageQuadTreeParticlesChecked;
    double averageQuadTreeCandidatesFound;
    double averageCollisions;
    double averageCandidatesPerParticle;
};

std::string distributionName(DistributionType distribution);

std::vector<Particle> generateParticles(
    DistributionType distribution,
    int n,
    double width,
    double height,
    double radius,
    int seed
);

ExperimentResult runExperiment(
    DistributionType distribution,
    int particleCount,
    int frames,
    double width,
    double height,
    double radius,
    int capacity
);

void printExperimentHeader(double width, double height, double radius, int capacity, int frames);
void printResult(const ExperimentResult& result);
void writeResultsCsv(const std::string& fileName, const std::vector<ExperimentResult>& results);

#endif
