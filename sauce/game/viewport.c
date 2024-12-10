#include "viewport.h"
#include <PalmOS.h>
#include "gamesession.h"

Coordinate viewport_convertedCoordinate(Coordinate coordinate) {
    coordinate.x = -gameSession.viewportOffset.x + coordinate.x;
    coordinate.y = -gameSession.viewportOffset.y + coordinate.y;
    return coordinate;
}

Coordinate viewport_convertedCoordinateInverted(Coordinate coordinate) {
    coordinate.x = gameSession.viewportOffset.x + coordinate.x;
    coordinate.y = gameSession.viewportOffset.y + coordinate.y;
    return coordinate;
}

Line viewport_convertedLine(Line line) {
    line.startpoint = viewport_convertedCoordinate(line.startpoint);
    line.endpoint = viewport_convertedCoordinate(line.endpoint);
    return line;
}

Line viewport_convertedLineInverted(Line line) {
    line.startpoint = viewport_convertedCoordinateInverted(line.startpoint);
    line.endpoint = viewport_convertedCoordinateInverted(line.endpoint);
    return line;
}