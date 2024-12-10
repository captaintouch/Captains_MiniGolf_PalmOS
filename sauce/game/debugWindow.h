// Window that is drawn on top containing debug data
#ifndef DEBUGWINDOW_H_
#define DEBUGWINDOW_H_
#include <PalmOS.h>

void debugWindow_setup();
void debugWindow_drawDebugInfo();
void debugWindow_setTopLine(char text[20], int lineValue);
void debugWindow_setBottomLine(char text[20], int lineValue);

#endif