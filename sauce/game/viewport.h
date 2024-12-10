#ifndef VIEWPORT_H_
#define VIEWPORT_H_
#include <PalmOS.h>
#include "models.h"

Coordinate viewport_convertedCoordinate(Coordinate coordinate);
Coordinate viewport_convertedCoordinateInverted(Coordinate coordinate);
Line viewport_convertedLine(Line line);
Line viewport_convertedLineInverted(Line line);

#endif
