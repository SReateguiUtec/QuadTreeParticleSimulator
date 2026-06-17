#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#include <string>

void generateVisualization(
    double width,
    double height,
    double radius,
    int capacity,
    const std::string& fileName,
    int particleCount
);

void generateVisualizationFrames(
    double width,
    double height,
    double radius,
    int capacity,
    const std::string& fileName,
    int particleCount,
    int totalFrames
);

#endif
