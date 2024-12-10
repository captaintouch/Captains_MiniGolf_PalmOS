#ifndef MODELS_H_
#define MODELS_H_

#include <PalmOS.h>

typedef struct Coordinate {
    int x;
    int y;
} Coordinate;

typedef struct Line {
    Coordinate startpoint;
    Coordinate endpoint;
} Line;

typedef struct Trajectory {
    Line* lines;
    int* lineDistances;
    int totalDistance;
    int lineCount;
} Trajectory;

Coordinate coordinate(int x, int y);

#endif
