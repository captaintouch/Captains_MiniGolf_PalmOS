#ifndef GAME_H_
#define GAME_H_
#include <PalmOS.h>
#include "gamesession.h"

typedef void (*openMainMenuCallback_t)();

Boolean game_mainLoop(EventPtr eventptr, openMainMenuCallback_t callback);
void game_setup(LocalID dbId, GameMode gameMode);
Boolean game_restore();
void game_end(Boolean saveGameState);
int game_eventDelayTime();

#endif