#include <PalmOS.h>
#include "debugWindow.h"
#include <string.h>
#include "colors.h"

#ifndef DEBUG
    void debugWindow_setup() {}
    void debugWindow_drawDebugInfo() {}
    void debugWindow_setTopLine(char text[20], int lineValue) {}
    void debugWindow_setBottomLine(char text[20], int lineValue) {}
#else

typedef struct {
    char text[20];
    int lineValue;
} DebugLine;

DebugLine debugLineA;
DebugLine debugLineB;

void debugWindow_setup() {
    debugLineA.lineValue = 0;
    strcpy(debugLineA.text, "");
    debugLineB.lineValue = 0;
    strcpy(debugLineB.text, "");
}

static void debugWindow_clearBackground() {
    RectangleType rect;
    rect.topLeft.x = 0;
    rect.topLeft.y = 0;
    rect.extent.x = 80;
    rect.extent.y = 30;
    drawhelper_applyForeColor(ALIZARIN);
    WinDrawRectangle(&rect, 0);
}

void debugWindow_drawDebugInfo() {
    int yOffset = 0;
    char displayText[40] = "";
    char valueText[20];

    debugWindow_clearBackground();
    
    StrIToA(valueText, debugLineA.lineValue);
    StrCopy(displayText, debugLineA.text);
    StrCat(displayText, valueText);
    WinDrawChars(displayText, StrLen(displayText), 0, yOffset);
    yOffset += 12;

    StrIToA(valueText, debugLineB.lineValue);
    StrCopy(displayText, debugLineB.text);
    StrCat(displayText, valueText);
    WinDrawChars(displayText, StrLen(displayText), 0, yOffset);
}

void debugWindow_setTopLine(char text[20], int lineValue) {
    strcpy(debugLineA.text, text);
    debugLineA.lineValue = lineValue;
}

void debugWindow_setBottomLine(char text[20], int lineValue) {
    strcpy(debugLineB.text, text);
    debugLineB.lineValue = lineValue;
}

#endif