#include <iostream>
#include <vector>
#include "Experiment.h"
#include "Visualization.h"

using namespace std;

int main() {
    double width = 1000;
    double height = 1000;
    double particleRadius = 5;
    double visualParticleRadius = 3;
    int capacity = 4;
    int frames = 3;
    int visualParticleCount = 420;
    int visualFrames = 160;

    vector<int> sizes = {1000, 5000, 10000};
    vector<DistributionType> distributions = {
        DistributionType::Uniform,
        DistributionType::Clusters,
        DistributionType::HighDensity
    };
    vector<ExperimentResult> results;

    printExperimentHeader(width, height, particleRadius, capacity, frames);

    for (DistributionType distribution : distributions) {
        for (int size : sizes) {
            ExperimentResult result = runExperiment(
                distribution,
                size,
                frames,
                width,
                height,
                particleRadius,
                capacity
            );

            results.push_back(result);
            printResult(result);
        }
    }

    writeResultsCsv("results.csv", results);
    cout << endl;
    cout << "Resultados exportados en: results.csv" << endl;

    generateVisualization(width, height, particleRadius, capacity, "visualization.svg", 250);
    generateVisualizationFrames(
        width,
        height,
        visualParticleRadius,
        capacity,
        "visualization_data.js",
        visualParticleCount,
        visualFrames
    );
    cout << "Front dinamico disponible en: frontend/index.html" << endl;

    return 0;
}
