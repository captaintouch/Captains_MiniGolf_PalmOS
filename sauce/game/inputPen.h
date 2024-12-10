#ifndef INPUTPEN_H_
#define INPUTPEN_H_

#include <PalmOS.h>
#include "models.h"

typedef struct InputPen {
    Coordinate touchStartCoordinate;
    Coordinate touchEndCoordinate;
    Boolean touching;
    Boolean didMove;
    Boolean wasUpdatedFlag;
} InputPen;

InputPen inputPen_updateEventDetails(InputPen lastInput, EventPtr eventPtr, int xOffset, int yOffset);

#endif