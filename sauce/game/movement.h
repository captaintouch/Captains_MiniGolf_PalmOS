#ifndef MOVEMENT_H_
#define MOVEMENT_H_
#include <PalmOS.h>
#include "models.h"
#include "level.h"
float fmin(float a, float b);
float fmax(float a, float b);
float movement_angleBetweenRad(Coordinate startCoordinate, Coordinate endCoordinate);
void movement_lineToTarget(Coordinate startCoordinate, float angle, Int16 distance, Line *targetLine);
float movement_lineDistance(Line *lineDistance);
Coordinate movement_coordinateAtPercentageOfLine(Line line, float percentage);
Coordinate movement_coordinateAtPercentageOfTrajectory(Trajectory trajectory, float percentage);
Trajectory movement_trajectory(Line* initialLine, Line* wallLines, UInt16 wallCount);
void movement_cleanupTrajectory(Trajectory *trajectory);
int movement_distanceBetweenCoordinates(Coordinate firstCoordinate, Coordinate secondCoordinate);
int movement_distanceToLine(Line line, Coordinate coordinate);
Boolean movement_isInsideWalls(Coordinate *coordinates, int n, Coordinate p);
Coordinate movement_closestWallStartOrEndpoint(Level *level, int excludeWallIndex, Coordinate position, int maxDistance);

#endif