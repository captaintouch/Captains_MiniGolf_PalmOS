#ifndef GAMESESSION_H_
#define GAMESESSION_H_
#include <PalmOS.h>
#include "inputPen.h"
#include "models.h"
#include "level.h"

typedef struct SpriteLibrary {
    Boolean initialized;
    ImageSprite ballSprite;
    ImageSprite holeSprite;
    ImageSprite grassSprite;
    ImageSprite ufoSprite;
    AnimationSprite windmillSprite;
    AnimationSprite chickenSprite;
} SpriteLibrary;

typedef enum GameMode {
    NORMALGAME,
    PRACTICEGAME,
    LEVELEDIT
} GameMode;

typedef struct GameRestorableSessionData {
    LocalID dbId;
    int level;
    int remainingShots;
    Coordinate ballPosition;
    GameMode gameMode;
} GameRestorableSessionData;

typedef struct GameSession {
    // Basic game loop timing mechanism
    Int32 nextGameLogicProgressionTime;
    UInt32 timeBetweenLogicProgressions;

    // All input related variables
    InputPen lastPenInput;
    Boolean isAiming;
    Boolean rejectPenInput;

    // Ball related variables
    Coordinate ballPosition;
    Int32 ballLaunchTimestamp;
    Int32 ballInertia;
    Trajectory ballTrajectory;

    // General
    LocalID dbId;
    Level currentLevel;
    int level;
    int levelCount;
    int remainingShots;
    Coordinate viewportOffset;
    
    // States
    Boolean needsBackgroundRefresh;
    Boolean needsHeaderRefresh;
    Boolean userScroll;
    Boolean paused;
    Boolean levelPackFinished;
    Boolean isLoading;
    GameMode gameMode;
    GameMode originalGameMode;

    // Level Edit
    int levelEditSelectedWallIndex;
    int levelEditSelectedItemIndex;
    Boolean levelEditDraggingStart;
    Boolean levelEditDraggingEnd;

    // Graphics
    UInt32 depth;
} GameSession;

GameSession gameSession;
SpriteLibrary spriteLibrary;

void gameSession_end(Boolean clearMemory);
void gameSession_levelRestart(int newLevel, GameMode gameMode);
void gameSession_openBlankLevel();
void gameSession_clearPenInput();
void gameSession_saveGameState();
void gameSession_clearSavedGameState();
GameRestorableSessionData *gameSession_loadGameState();

void gameSession_scheduleNextGameLogicProgression();
void gameSession_progressGameLogic();
Int32 gameSession_timeUntilNextEvent();
void gameSession_registerPenInput(EventPtr eventptr);
void gameSession_aimingLine(Line *aimingLine, Line *leftArrowLine, Line *rightArrowLine, int *power);
void gameSession_updateViewPortOffset();

Coordinate gameSession_buttonUpPosition();
Coordinate gameSession_buttonDownPosition();
Coordinate gameSession_buttonLeftPosition();
Coordinate gameSession_buttonRightPosition();

#endif