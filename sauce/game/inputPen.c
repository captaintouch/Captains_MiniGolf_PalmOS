#include <PalmOS.h>
#include "inputPen.h"
#include "models.h"

InputPen inputPen_updateEventDetails(InputPen lastInput, EventPtr eventPtr, int xOffset, int yOffset) {
    if (eventPtr->eType != penDownEvent && eventPtr->eType != penUpEvent && eventPtr->eType != penMoveEvent) {
        return lastInput;
    }
    if (eventPtr->eType == penDownEvent) {
        lastInput.touchStartCoordinate.x = eventPtr->screenX + xOffset;
        lastInput.touchStartCoordinate.y = eventPtr->screenY + yOffset;
        lastInput.touchEndCoordinate.x = eventPtr->screenX + xOffset;
        lastInput.touchEndCoordinate.y = eventPtr->screenY + yOffset;
    }
    if (eventPtr->eType == penMoveEvent) {
        lastInput.touchEndCoordinate.x = eventPtr->screenX + xOffset;
        lastInput.touchEndCoordinate.y = eventPtr->screenY + yOffset;
    }
    lastInput.didMove = lastInput.touchStartCoordinate.x != lastInput.touchEndCoordinate.x || lastInput.touchStartCoordinate.y != lastInput.touchEndCoordinate.y;
    lastInput.touching = eventPtr->penDown;
    lastInput.wasUpdatedFlag = true;

    return lastInput;
} 