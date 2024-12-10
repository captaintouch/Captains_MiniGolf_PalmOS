#include "movement.h"
#include "../MathLib.h"
#include "debugWindow.h"
#include "models.h"
#include <PalmOS.h>
#include <limits.h>

#define M_PI 3.14159265358979323846
#define MAXWALLCOLLISSION 6

typedef struct LineSplitResult {
    Line splittedLine;
    Line remainingLine;
    Boolean didImpact;
} LineSplitResult;

float fmin(float a, float b) {
    if (a < b) {
        return a;
    } else {
        return b;
    }
}

float fmax(float a, float b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

float movement_angleBetweenRad(Coordinate startCoordinate, Coordinate endCoordinate) {
    double deltaY = endCoordinate.y - startCoordinate.y;
    double deltaX = endCoordinate.x - startCoordinate.x;
    double angleInRadians = atan2(deltaY, deltaX);
    return angleInRadians;
}

// Function to calculate target coordinates
void movement_lineToTarget(Coordinate startCoordinate, float angleRad, Int16 distance, Line *targetLine) {
    // Calculate the target coordinates
    targetLine->startpoint.x = startCoordinate.x;
    targetLine->startpoint.y = startCoordinate.y;
    targetLine->endpoint.x = startCoordinate.x + ((float)distance * cos(angleRad));
    targetLine->endpoint.y = startCoordinate.y + ((float)distance * sin(angleRad));
}

Coordinate movement_coordinateAtPercentageOfLine(Line line, float percentage) {
    Coordinate coordinate;
    coordinate.x = line.startpoint.x + percentage * (line.endpoint.x - line.startpoint.x);
    coordinate.y = line.startpoint.y + percentage * (line.endpoint.y - line.startpoint.y);
    return coordinate;
}

Coordinate movement_coordinateAtPercentageOfTrajectory(Trajectory trajectory, float percentage) {
    float easingPercentage = 1.0 - (1.0 - percentage) * (1.0 - percentage); // Use easing to make progress faster at the start and slower at the end
    int targetDistance = trajectory.totalDistance * easingPercentage;
    int i;
    int distance = 0;
    for (i = 0; i < trajectory.lineCount; i++) {
        if (distance <= targetDistance && distance + trajectory.lineDistances[i] >= targetDistance) { // Found the line
            float diffDistance = targetDistance - distance;
            float linePercentage = diffDistance / (float)trajectory.lineDistances[i];
            return movement_coordinateAtPercentageOfLine(trajectory.lines[i], linePercentage);
        }
        distance += trajectory.lineDistances[i];
    }

    return trajectory.lines[trajectory.lineCount - 1].endpoint;
}

float movement_lineDistance(Line *lineDistance) {
    return sqrt(pow(lineDistance->endpoint.x - lineDistance->startpoint.x, 2) + pow(lineDistance->endpoint.y - lineDistance->startpoint.y, 2));
}

static double movement_reflectionAngleRad(double incomingAngleRad, double impactAngleRad) {
    // Convert angles to radians for trigonometric functions
    double reflectionAngleRad, incidenceAngleRad;

    // Calculate the angle of incidence relative to the normal
    incidenceAngleRad = incomingAngleRad - impactAngleRad;

    // Reflection angle is equal to the incidence angle relative to the normal
    reflectionAngleRad = impactAngleRad - incidenceAngleRad;

    return reflectionAngleRad;
}

static void movement_adjustLineAngle(Line *lineToAdjust, Line *wallLine) {
    double currentAngleRad = movement_angleBetweenRad(lineToAdjust->startpoint, lineToAdjust->endpoint);
    double impactAngleRad = movement_angleBetweenRad(wallLine->startpoint, wallLine->endpoint);
    double newAngleRad = movement_reflectionAngleRad(currentAngleRad, impactAngleRad);
    int distance = movement_lineDistance(lineToAdjust);
    lineToAdjust->endpoint.x = (float)lineToAdjust->startpoint.x + (float)distance * cos(newAngleRad);
    lineToAdjust->endpoint.y = (float)lineToAdjust->startpoint.y + (float)distance * sin(newAngleRad);
}
static double movement_sq(double a) {
    return a * a;
}

int movement_distanceToLine(Line line, Coordinate coordinate) {
    Coordinate closest;
    double t;
    double lengthSquared = movement_sq(line.endpoint.x - line.startpoint.x) + movement_sq(line.endpoint.y - line.startpoint.y);
    if (lengthSquared == 0.0) return movement_distanceBetweenCoordinates(coordinate, line.startpoint);
    t = ((coordinate.x - line.startpoint.x) * (line.endpoint.x - line.startpoint.x) + (coordinate.y - line.startpoint.y) * (line.endpoint.y - line.startpoint.y)) / lengthSquared;
    if (t < 0.0) return movement_distanceBetweenCoordinates(coordinate, line.startpoint);
    else if (t > 1.0) return movement_distanceBetweenCoordinates(coordinate, line.endpoint);
    closest.x = line.startpoint.x + t * (line.endpoint.x - line.startpoint.x);
    closest.y = line.startpoint.y + t * (line.endpoint.y - line.startpoint.y);
    return movement_distanceBetweenCoordinates(coordinate, closest);
}

static int movement_sideOfLine(Coordinate p, Coordinate lineStart, Coordinate lineEnd) {
    double crossProduct = (lineEnd.x - lineStart.x) * (p.y - lineStart.y) - (lineEnd.y - lineStart.y) * (p.x - lineStart.x);
    if (crossProduct > 0) {
        return 1; // Point is on the left side
    } else if (crossProduct < 0) {
        return -1; // Point is on the right side
    } else {
        return 0; // Point is on the line
    }
}

static Coordinate movement_findIntersection(Coordinate startingpoint, Coordinate endpoint, Coordinate wallStartingpoint, Coordinate wallEndpoint) {
    double a1, b1, c1; // Coefficients for the first line
    double a2, b2, c2; // Coefficients for the second line
    double determinant;

    // Calculate coefficients for the first line
    a1 = endpoint.y - startingpoint.y;
    b1 = startingpoint.x - endpoint.x;
    c1 = a1 * startingpoint.x + b1 * startingpoint.y;

    // Calculate coefficients for the second line
    a2 = wallEndpoint.y - wallStartingpoint.y;
    b2 = wallStartingpoint.x - wallEndpoint.x;
    c2 = a2 * wallStartingpoint.x + b2 * wallStartingpoint.y;

    // Calculate the determinant
    determinant = a1 * b2 - a2 * b1;

    // Check if lines are parallel
    if (determinant == 0) {
        // Lines are parallel
        return coordinate(-1, -1);
    } else {
        // Lines intersect
        Coordinate impact;
        impact.x = (b2 * c1 - b1 * c2) / determinant;
        impact.y = (a1 * c2 - a2 * c1) / determinant;
        if (impact.x >= fmin(startingpoint.x, endpoint.x) && impact.x <= fmax(startingpoint.x, endpoint.x) &&
            impact.x >= fmin(wallStartingpoint.x, wallEndpoint.x) && impact.x <= fmax(wallStartingpoint.x, wallEndpoint.x) &&
            impact.y >= fmin(startingpoint.y, endpoint.y) && impact.y <= fmax(startingpoint.y, endpoint.y) &&
            impact.y >= fmin(wallStartingpoint.y, wallEndpoint.y) && impact.y <= fmax(wallStartingpoint.y, wallEndpoint.y)) {
            int startSide, endSide;
            Line impactLine;
            float distance, angleRad;
            impactLine.startpoint = startingpoint;
            impactLine.endpoint = impact;
            angleRad = movement_angleBetweenRad(impactLine.startpoint, impact);
            distance = fmax(0, movement_lineDistance(&impactLine) - 5.0);
            movement_lineToTarget(impactLine.startpoint, angleRad, distance, &impactLine);
            impact = impactLine.endpoint;

            startSide = movement_sideOfLine(startingpoint, wallStartingpoint, wallEndpoint);
            endSide = movement_sideOfLine(impact, wallStartingpoint, wallEndpoint);

            if (startSide != endSide) {
                return startingpoint;
            } else {
                return impact;
            }
        } else {
            return coordinate(-1, -1);
        }

        return coordinate(-1, -1);
    }

    return coordinate(-1, -1);
}

int movement_distanceBetweenCoordinates(Coordinate firstCoordinate, Coordinate secondCoordinate) {
    return sqrt(pow(secondCoordinate.x - firstCoordinate.x, 2) + pow(secondCoordinate.y - firstCoordinate.y, 2));
}

Boolean movement_isInsideWalls(Coordinate *coordinates, int n, Coordinate p) {
    int i, j;
    int isInside = 0;

    for (i = 0, j = n - 1; i < n; j = i++) {
        if (((coordinates[i].y > p.y) != (coordinates[j].y > p.y)) &&
            (p.x < (coordinates[j].x - coordinates[i].x) * (p.y - coordinates[i].y) / (coordinates[j].y - coordinates[i].y) + coordinates[i].x)) {
            isInside = !isInside;
        }
    }

    return isInside;
}

Coordinate movement_closestWallStartOrEndpoint(Level *level, int excludeWallIndex, Coordinate position, int maxDistance) {
  int i;
  int shortestDistance = INT_MAX;
  Coordinate shortestCoordinate = coordinate(-1, -1);
    for (i = 0; i < level->wallCount; i++) {
        int distance;
        if (i == excludeWallIndex) continue;
        distance = movement_distanceBetweenCoordinates(position, level->walls[i].startpoint);
        if (shortestDistance > distance && distance <= maxDistance) {
            shortestDistance = distance;
            shortestCoordinate = level->walls[i].startpoint;
        }
        distance = movement_distanceBetweenCoordinates(position, level->walls[i].endpoint);
        if (shortestDistance > distance && distance <= maxDistance) {
            shortestDistance = distance;
            shortestCoordinate = level->walls[i].endpoint;
        }
    }
    return shortestCoordinate;
}

static void movement_keepLineEndpointAwayFromWall(Line *lineToAdjust, Line *wallLines, UInt16 wallCount) {
    int i, shortestWallDistance;
    shortestWallDistance = INT_MAX;
    for (i = 0; i < wallCount; i++) {
        int distance = movement_distanceToLine(wallLines[i], lineToAdjust->endpoint);
        if (shortestWallDistance < distance) {
            shortestWallDistance = distance;
        }
    }
    if (shortestWallDistance <= 5) {
        float distance, angleRad;
        angleRad = movement_angleBetweenRad(lineToAdjust->startpoint, lineToAdjust->endpoint);
        distance = fmax(0, movement_lineDistance(lineToAdjust) - 10.0);
        movement_lineToTarget(lineToAdjust->startpoint, angleRad, distance, lineToAdjust);
    }
}

static LineSplitResult movement_splitLine(Line lineToSplit, Line *wallLines, UInt16 wallCount) {
    int i, lastDistance;
    LineSplitResult result;
    Coordinate intersection;
    int resultDistance = INT_MAX;
    result.didImpact = false;
    for (i = 0; i < wallCount; i++) {
        intersection = movement_findIntersection(lineToSplit.startpoint, lineToSplit.endpoint, wallLines[i].startpoint, wallLines[i].endpoint);
        if (intersection.x != -1 && intersection.y != -1) { // line intersects with a wall
            Line splittedLine;
            splittedLine.startpoint = lineToSplit.startpoint;
            splittedLine.endpoint = intersection;
            lastDistance = movement_lineDistance(&splittedLine);
            if (lastDistance < resultDistance) { // always ensure the closest wall is taken into account, not any others
                resultDistance = lastDistance;
                result.remainingLine.startpoint = splittedLine.endpoint;
                result.remainingLine.endpoint = lineToSplit.endpoint;
                result.splittedLine = splittedLine;
                movement_adjustLineAngle(&result.remainingLine, &wallLines[i]);
                result.didImpact = true;
            }
        }
    }
    return result;
}

// Split initialLine up into multiple lines, based on impact with wallLines
Trajectory movement_trajectory(Line *initialLine, Line *wallLines, UInt16 wallCount) {
    Trajectory trajectory;
    Line *lineToSplit = initialLine;
    int i;
    int collisionCount = 0;
    LineSplitResult lineSplitResult;
    Line remainingLine;
    lineSplitResult.didImpact = true;
    trajectory.lines = (Line *)MemPtrNew(sizeof(Line) * (MAXWALLCOLLISSION + 1));

    lineSplitResult = movement_splitLine(*lineToSplit, wallLines, wallCount);
    while (lineSplitResult.didImpact && collisionCount < MAXWALLCOLLISSION) {
        collisionCount++;
        trajectory.lines[collisionCount - 1] = lineSplitResult.splittedLine;
        lineSplitResult.remainingLine.startpoint = lineSplitResult.splittedLine.endpoint;
        remainingLine = lineSplitResult.remainingLine;
        lineSplitResult = movement_splitLine(lineSplitResult.remainingLine, wallLines, wallCount);
    }
    if (collisionCount == 0) {
        collisionCount = 1;
        movement_keepLineEndpointAwayFromWall(initialLine, wallLines, wallCount);
        trajectory.lines[0] = *initialLine;
    } else if (collisionCount < MAXWALLCOLLISSION) {
        collisionCount++;
        movement_keepLineEndpointAwayFromWall(&remainingLine, wallLines, wallCount);
        trajectory.lines[collisionCount - 1] = remainingLine;
    } else {
        Line lastLine = trajectory.lines[collisionCount - 1];
        movement_keepLineEndpointAwayFromWall(&lastLine, wallLines, wallCount);
        trajectory.lines[collisionCount - 1] = lastLine;
    }

    MemPtrResize(trajectory.lines, sizeof(Line) * collisionCount);

    trajectory.lineDistances = (int *)MemPtrNew(sizeof(int) * collisionCount);
    trajectory.totalDistance = 0;
    trajectory.lineCount = collisionCount;
    for (i = 0; i < collisionCount; i++) {
        trajectory.lineDistances[i] = movement_lineDistance(&trajectory.lines[i]);
        trajectory.totalDistance += trajectory.lineDistances[i];
    }
    return trajectory;
}

void movement_cleanupTrajectory(Trajectory *trajectory) {
    if (trajectory->lineCount > 0) {
        MemPtrFree(trajectory->lines);
        trajectory->lines = NULL;
        MemPtrFree(trajectory->lineDistances);
        trajectory->lineDistances = NULL;
        trajectory->lineCount = 0;
    }
}
