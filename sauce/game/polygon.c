#include "polygon.h"
#include "movement.h"
#include <PalmOS.h>

static Boolean polygon_wallExistsWithCoordinate(Line *walls, int wallCount, int excludingIndex, Coordinate coordinate) {
    int i;
    for (i = 0; i < wallCount; i++) {
        if (i == excludingIndex)
            continue;
        if (walls[i].startpoint.x == coordinate.x && walls[i].startpoint.y == coordinate.y) {
            return true;
        }
        if (walls[i].endpoint.x == coordinate.x && walls[i].endpoint.y == coordinate.y) {
            return true;
        }
    }
    return false;
}

static Boolean polygon_coordinateIsUnique(Coordinate *arr, int n, Coordinate coord) {
    int i;
    for (i = 0; i < n; i++) {
        if (arr[i].x == coord.x && arr[i].y == coord.y) {
            return false;
        }
    }
    return true;
}

static int polygon_removeCoordinateDuplicates(Coordinate *arr, int n) {
    int i, uniqueCount = 0;
    if (n == 0)
        return 0;

    for (i = 0; i < n; i++) {
        if (polygon_coordinateIsUnique(arr, uniqueCount, arr[i])) {
            arr[uniqueCount++] = arr[i];
        }
    }
    return uniqueCount;
}

static void polygon_removeFirstUnconnectedLine(Line *remainingWalls, int *remainingCount) {
    int i, j;

    for (i = 0; i < *remainingCount; i++) {
        if (!polygon_wallExistsWithCoordinate(remainingWalls, *remainingCount, i, remainingWalls[i].startpoint) || !polygon_wallExistsWithCoordinate(remainingWalls, *remainingCount, i, remainingWalls[i].endpoint)) {
            for (j = i; j < *remainingCount - 1; j++) {
                remainingWalls[j] = remainingWalls[j + 1];
            }
            (*remainingCount)--;
            return;
        }
    }
}

static void reverse_line(Line *line) {
    Coordinate temp = line->startpoint;
    line->startpoint = line->endpoint;
    line->endpoint = temp;
}

static int connect_lines(Line *a, Line *b) {
    if (a->endpoint.x == b->startpoint.x && a->endpoint.y == b->startpoint.y) {
        return 1; // Already connected as is
    } else if (a->endpoint.x == b->endpoint.x && a->endpoint.y == b->endpoint.y) {
        reverse_line(b); // Reverse b to connect
        return 1;
    } else if (a->startpoint.x == b->startpoint.x && a->startpoint.y == b->startpoint.y) {
        reverse_line(a); // Reverse a to connect
        return 1;
    } else if (a->startpoint.x == b->endpoint.x && a->startpoint.y == b->endpoint.y) {
        reverse_line(a); // Reverse a to connect
        reverse_line(b); // Reverse b to connect
        return 1;
    }
    return 0;
}

static void polygon_sortLines(Line lines[], int n) {
    int i, j;
    for (i = 0; i < n - 1; i++) {
        for (j = i + 1; j < n; j++) {
            if (connect_lines(&lines[i], &lines[j])) {
                // If they are connected, swap them
                Line temp = lines[i + 1];
                lines[i + 1] = lines[j];
                lines[j] = temp;
                break;
            }
        }
    }
}

static int polygon_pointsFromLines(Line *walls, int wallCount, Coordinate *polygonPoints) {
    int i, count = 0;

    for (i = 0; i < wallCount; i++) {
        count += 2;
        polygonPoints[count - 2] = walls[i].startpoint;
        polygonPoints[count - 1] = walls[i].endpoint;
    }
    return count;
}

void polygon_outerPolygon(Line *walls, int wallCount, Coordinate *polygonPoints, int *polyCount) {
    int i, count, lastWallCount = -1;
    Line *remainingWalls;
    int remainingWallCount = wallCount;

    if (wallCount <= 2) {
        *polyCount = 0;
        return;
    }
    remainingWalls = (Line *)MemPtrNew(sizeof(Line) * wallCount);
    for (i = 0; i < wallCount; i++) {
        remainingWalls[i] = walls[i];
    }
    while (remainingWallCount != lastWallCount) {
        lastWallCount = remainingWallCount;
        polygon_removeFirstUnconnectedLine(remainingWalls, &remainingWallCount);
    }
    polygon_sortLines(remainingWalls, remainingWallCount);
    count = polygon_pointsFromLines(remainingWalls, remainingWallCount, polygonPoints);
    MemPtrFree(remainingWalls);
    count = polygon_removeCoordinateDuplicates(polygonPoints, count);
    *polyCount = count;
}