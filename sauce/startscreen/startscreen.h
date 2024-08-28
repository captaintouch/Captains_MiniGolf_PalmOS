#ifndef STARTSCREEN_H_
#define STARTSCREEN_H_
#include <PalmOS.h>
#include "../game/gamesession.h"

typedef void (*startGameCallback_t)(LocalID, GameMode);

void startscreen_setup();
void startscreen_cleanup();
Boolean startscreen_eventHandler(EventPtr eventptr, startGameCallback_t callback);

#endif