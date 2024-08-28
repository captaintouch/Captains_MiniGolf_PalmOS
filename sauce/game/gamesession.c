#include "gamesession.h"
#include "../constants.h"
#include "../database.h"
#include "../deviceinfo.h"
#include "../resources.h"
#include "debugWindow.h"
#include "level.h"
#include "models.h"
#include "movement.h"
#include "viewport.h"
#include <PalmOS.h>
#include <limits.h>

#define GAME_LOGIC_TICK 100
#define DEGREESRAD180 3.142
#define DEGREESRAD45 0.7854

// static functions
static void gameSession_progressGameLogicPenInput();
static void gameSession_launchBall();
static void gameSession_progressUpdateBall();

void gameSession_saveGameState() {
    void *dataPtr;
    GameRestorableSessionData sessionData;
    gameSession_clearSavedGameState();
    sessionData.dbId = gameSession.dbId;
    sessionData.level = gameSession.level;
    sessionData.remainingShots = gameSession.remainingShots;
    sessionData.ballPosition = gameSession.ballPosition;
    FtrPtrNew(APP_CREATOR_ID, FEATUREMEM_SAVESTATE, sizeof(GameRestorableSessionData), &dataPtr);
    DmWrite(dataPtr, 0, &sessionData, sizeof(GameRestorableSessionData));
}

GameRestorableSessionData *gameSession_loadGameState() {
    GameRestorableSessionData *sessionData;
    UInt32 dataPtr;
    if (FtrGet(APP_CREATOR_ID, FEATUREMEM_SAVESTATE, &dataPtr) != errNone)
        return NULL;
    sessionData = (GameRestorableSessionData *)dataPtr;
    if (!database_levelPackExists(sessionData->dbId) || (database_levelCount(sessionData->dbId) <= sessionData->level))
        return NULL;
    return sessionData;
}

void gameSession_clearSavedGameState() {
    FtrUnregister(APP_CREATOR_ID, FEATUREMEM_SAVESTATE);
}

static void gameSession_initializeSpriteLibrary() {
    spriteLibrary.ballSprite = imageSprite(
        RESOURCE_GFX_BALL,
        coordinate(RESOURCE_GFX_BALLDIMENSION, RESOURCE_GFX_BALLDIMENSION));
    spriteLibrary.holeSprite = imageSprite(
        RESOURCE_GFX_HOLE,
        coordinate(RESOURCE_GFX_HOLEDIMENSION, RESOURCE_GFX_HOLEDIMENSION));
    spriteLibrary.grassSprite = imageSprite(
        RESOURCE_GFX_GRASS,
        coordinate(RESOURCE_GFX_GRASSDIMENSION, RESOURCE_GFX_GRASSDIMENSION));
    spriteLibrary.ufoSprite = imageSprite(
        RESOURCE_GFX_UFO,
        coordinate(RESOURCE_GFX_UFODIMENSION, RESOURCE_GFX_UFODIMENSION));
    spriteLibrary.windmillSprite =
        animationSprite(RESOURCE_GFX_WINDMILL, 8,
                        coordinate(RESOURCE_GFX_WINDMILLDIMENSION,
                                   RESOURCE_GFX_WINDMILLDIMENSION));
    spriteLibrary.chickenSprite =
        animationSprite(RESOURCE_GFX_CHICKEN, 4,
                        coordinate(RESOURCE_GFX_CHICKENDIMENSION,
                                   RESOURCE_GFX_CHICKENDIMENSION));

    spriteLibrary.ballSprite.imageData =
        drawhelper_loadImage(spriteLibrary.ballSprite.resourceId);
    spriteLibrary.holeSprite.imageData =
        drawhelper_loadImage(spriteLibrary.holeSprite.resourceId);
    spriteLibrary.grassSprite.imageData =
        drawhelper_loadImage(spriteLibrary.grassSprite.resourceId);
    spriteLibrary.ufoSprite.imageData =
        drawhelper_loadImage(spriteLibrary.ufoSprite.resourceId);
    spriteLibrary.windmillSprite.imageData = drawhelper_loadAnimation(
        spriteLibrary.windmillSprite.resourceId, spriteLibrary.windmillSprite.frameCount);
    spriteLibrary.chickenSprite.imageData = drawhelper_loadAnimation(
        spriteLibrary.chickenSprite.resourceId, spriteLibrary.chickenSprite.frameCount);

    spriteLibrary.initialized = true;
}

static void gameSession_unloadSpriteLibrary() {
    if (spriteLibrary.ballSprite.imageData) {
        drawhelper_releaseImage(spriteLibrary.ballSprite.imageData);
    }
    if (spriteLibrary.holeSprite.imageData) {
        drawhelper_releaseImage(spriteLibrary.holeSprite.imageData);
    }
    if (spriteLibrary.grassSprite.imageData) {
        drawhelper_releaseImage(spriteLibrary.grassSprite.imageData);
    }
    if (spriteLibrary.ufoSprite.imageData) {
        drawhelper_releaseImage(spriteLibrary.ufoSprite.imageData);
    }
    if (spriteLibrary.windmillSprite.imageData) {
        drawhelper_releaseAnimation(spriteLibrary.windmillSprite.imageData,
                                    spriteLibrary.windmillSprite.frameCount);
    }
    if (spriteLibrary.chickenSprite.imageData) {
        drawhelper_releaseAnimation(spriteLibrary.chickenSprite.imageData,
                                    spriteLibrary.chickenSprite.frameCount);
    }

    spriteLibrary.initialized = false;
}

Coordinate gameSession_buttonUpPosition() {
    return coordinate(
        GAMEWINDOW_WIDTH / 2 - RESOURCE_GFX_BUTTON_DIMENSION / 2,
        0);
}

Coordinate gameSession_buttonDownPosition() {
    return coordinate(
        GAMEWINDOW_WIDTH / 2 - RESOURCE_GFX_BUTTON_DIMENSION / 2,
        GAMEWINDOW_HEIGHT - RESOURCE_GFX_BUTTON_DIMENSION);
}

Coordinate gameSession_buttonLeftPosition() {
    return coordinate(
        0,
        GAMEWINDOW_HEIGHT / 2 - RESOURCE_GFX_BUTTON_DIMENSION / 2);
}

Coordinate gameSession_buttonRightPosition() {
    return coordinate(
        GAMEWINDOW_WIDTH - RESOURCE_GFX_BUTTON_DIMENSION,
        GAMEWINDOW_HEIGHT / 2 - RESOURCE_GFX_BUTTON_DIMENSION / 2);
}

Int32 game_timeUntilNextEvent() {
    Int32 timeleft = evtWaitForever;

    if (gameSession.nextGameLogicProgressionTime != evtWaitForever) {
        timeleft = gameSession.nextGameLogicProgressionTime - TimGetTicks();
    }
    return timeleft;
}

void gameSession_registerPenInput(EventPtr eventptr) {
    if (gameSession.paused || gameSession.rejectPenInput) {
        return;
    }
    // Input we get is for the entire screen, we need to offset it so that it matches our playing field area
    gameSession.lastPenInput = inputPen_updateEventDetails(gameSession.lastPenInput, eventptr, -GAMEWINDOW_X, -GAMEWINDOW_Y);
}

void gameSession_scheduleNextGameLogicProgression() {
    gameSession.nextGameLogicProgressionTime = TimGetTicks() + gameSession.timeBetweenLogicProgressions;
}

void gameSession_clearPenInput() {
    gameSession.lastPenInput.touchStartCoordinate = coordinate(0, 0);
    gameSession.lastPenInput.touchEndCoordinate = coordinate(0, 0);
    gameSession.lastPenInput.wasUpdatedFlag = false;
    gameSession.lastPenInput.touching = false;
    gameSession.lastPenInput.didMove = false;
}

static void gameSession_startNew(GameMode gameMode) {
    if (!spriteLibrary.initialized) {
        gameSession_initializeSpriteLibrary();
    }
    gameSession.originalGameMode = gameMode == LEVELEDIT ? PRACTICEGAME : gameMode;
    gameSession.gameMode = gameMode;
    gameSession.isLoading = true;
    gameSession.depth = deviceinfo_maxDepth();
    gameSession.remainingShots = GAME_MAX_ATTEMPTS;
    gameSession.paused = false;
    gameSession.rejectPenInput = false;
    gameSession.needsHeaderRefresh = true;
    gameSession.needsBackgroundRefresh = true;
    gameSession.levelCount = database_levelCount(gameSession.dbId);
    if (gameSession.levelCount == 0) {
        if (gameMode != LEVELEDIT) {
            FrmCustomAlert(GAME_ALERT_EMPTYLEVELPACK, NULL, NULL, NULL);
        }
        gameSession.gameMode = LEVELEDIT;
        gameSession.originalGameMode = PRACTICEGAME;
    }
    if (gameSession.gameMode != LEVELEDIT) {
        gameSession.currentLevel = level_load(gameSession.level, gameSession.dbId);
    } else {
        gameSession.level = gameSession.levelCount;
        gameSession.currentLevel = level_blankLevel();
    }
    gameSession.ballPosition = level_coordinateForItem(BALL, &gameSession.currentLevel);
    gameSession.timeBetweenLogicProgressions = SysTicksPerSecond() / GAME_LOGIC_TICK;
    gameSession.viewportOffset = coordinate(0, 0);
    gameSession.ballInertia = 0;
    gameSession.ballLaunchTimestamp = 0;
    gameSession.isAiming = false;
    gameSession_scheduleNextGameLogicProgression();
    gameSession_updateViewPortOffset();
}

void gameSession_end(Boolean clearMemory) {
    gameSession_clearPenInput();
    gameSession.nextGameLogicProgressionTime = evtWaitForever;
    movement_cleanupTrajectory(&gameSession.ballTrajectory);
    level_unload(gameSession.currentLevel);
    if (clearMemory) {
        gameSession_unloadSpriteLibrary();
    }
}

void gameSession_updateViewPortOffset() {
    if (gameSession.gameMode == LEVELEDIT) {
        return;
    }
    // Track the ball using the viewport
    gameSession.viewportOffset.x = fmin(gameSession.currentLevel.levelSize.x - GAMEWINDOW_WIDTH + 1, fmax(0, gameSession.ballPosition.x - GAMEWINDOW_WIDTH / 2));
    gameSession.viewportOffset.y = fmin(gameSession.currentLevel.levelSize.y - GAMEWINDOW_HEIGHT + 1, fmax(0, gameSession.ballPosition.y - GAMEWINDOW_HEIGHT / 2));
}

static Boolean gameSession_isInside(Coordinate coordinate, Coordinate targetStart, Coordinate targetSize) {
    return (coordinate.x >= targetStart.x && coordinate.x <= targetStart.x + targetSize.x && coordinate.y >= targetStart.y && coordinate.y <= targetStart.y + targetSize.y);
}

static Boolean gameSession_handleButtonTap() {
    if (gameSession.userScroll) {
        int yOffset = 0, xOffset = 0;
        if (gameSession_isInside(gameSession.lastPenInput.touchStartCoordinate, gameSession_buttonUpPosition(), coordinate(RESOURCE_GFX_BUTTON_DIMENSION, RESOURCE_GFX_BUTTON_DIMENSION))) {
            yOffset -= GAMEWINDOW_HEIGHT / 3;
        } else if (gameSession_isInside(gameSession.lastPenInput.touchStartCoordinate, gameSession_buttonDownPosition(), coordinate(RESOURCE_GFX_BUTTON_DIMENSION, RESOURCE_GFX_BUTTON_DIMENSION))) {
            yOffset += GAMEWINDOW_HEIGHT / 3;
        } else if (gameSession_isInside(gameSession.lastPenInput.touchStartCoordinate, gameSession_buttonRightPosition(), coordinate(RESOURCE_GFX_BUTTON_DIMENSION, RESOURCE_GFX_BUTTON_DIMENSION))) {
            xOffset += GAMEWINDOW_WIDTH / 3;
        } else if (gameSession_isInside(gameSession.lastPenInput.touchStartCoordinate, gameSession_buttonLeftPosition(), coordinate(RESOURCE_GFX_BUTTON_DIMENSION, RESOURCE_GFX_BUTTON_DIMENSION))) {
            xOffset -= GAMEWINDOW_WIDTH / 3;
        }
        if (!gameSession.lastPenInput.touching) {
            gameSession.viewportOffset.y += yOffset;
            gameSession.viewportOffset.x += xOffset;
        }
        return true;
    }
    return false;
}

static Boolean gameSession_handleLevelEditor() {
    int i, closestDistance, closestIndex;
    if (gameSession.gameMode != LEVELEDIT) {
        return false;
    }

    if (!gameSession.lastPenInput.touching) {
        if (gameSession.levelEditDraggingStart || gameSession.levelEditDraggingEnd) {
            gameSession.levelEditDraggingStart = false;
            gameSession.levelEditDraggingEnd = false;
            return true;
        }
    }

    if (gameSession.levelEditSelectedWallIndex >= 0 && gameSession.lastPenInput.touching) {
        // first check if we are dragging the start or endpoint of the selected line
        Coordinate touchCoordinate = gameSession.lastPenInput.touchEndCoordinate;
        if (gameSession.levelEditDraggingStart || gameSession.levelEditDraggingEnd) {
            Coordinate closestCoordinate = movement_closestWallStartOrEndpoint(&gameSession.currentLevel, gameSession.levelEditSelectedWallIndex, viewport_convertedCoordinateInverted(touchCoordinate), 3);
            if (closestCoordinate.x != -1 && closestCoordinate.y != -1) {
                touchCoordinate = viewport_convertedCoordinate(closestCoordinate);
            }
        }
        if (gameSession.levelEditDraggingStart) {
            gameSession.currentLevel.walls[gameSession.levelEditSelectedWallIndex].startpoint = viewport_convertedCoordinateInverted(touchCoordinate);
            return true;
        }
        if (gameSession.levelEditDraggingEnd) {
            gameSession.currentLevel.walls[gameSession.levelEditSelectedWallIndex].endpoint = viewport_convertedCoordinateInverted(touchCoordinate);
            return true;
        }

        if (movement_distanceBetweenCoordinates(gameSession.currentLevel.walls[gameSession.levelEditSelectedWallIndex].startpoint, viewport_convertedCoordinateInverted(gameSession.lastPenInput.touchStartCoordinate)) < 5) {
            gameSession.levelEditDraggingStart = true;
            return true;
        }
        if (movement_distanceBetweenCoordinates(gameSession.currentLevel.walls[gameSession.levelEditSelectedWallIndex].endpoint, viewport_convertedCoordinateInverted(gameSession.lastPenInput.touchStartCoordinate)) < 5) {
            gameSession.levelEditDraggingEnd = true;
            return true;
        }
    }

    if (gameSession.levelEditSelectedItemIndex >= 0 && gameSession.lastPenInput.touching) {
        int dimension = level_itemTypeDimension(gameSession.currentLevel.levelItems[gameSession.levelEditSelectedItemIndex].itemType);
        // first check if we are dragging an item
        if (gameSession.levelEditDraggingStart) {
            gameSession.currentLevel.levelItems[gameSession.levelEditSelectedItemIndex].position = viewport_convertedCoordinateInverted(gameSession.lastPenInput.touchEndCoordinate);
            return true;
        }

        if (movement_distanceBetweenCoordinates(gameSession.currentLevel.levelItems[gameSession.levelEditSelectedItemIndex].position, viewport_convertedCoordinateInverted(gameSession.lastPenInput.touchStartCoordinate)) < dimension) {
            gameSession.levelEditDraggingStart = true;
            return true;
        }
    }

    closestDistance = INT_MAX;
    // Check if user selected wall
    closestIndex = -1;
    for (i = 0; i < gameSession.currentLevel.wallCount; i++) {
        int distance = movement_distanceToLine(gameSession.currentLevel.walls[i], viewport_convertedCoordinateInverted(gameSession.lastPenInput.touchStartCoordinate));
        if (i == 0) {
            debugWindow_setTopLine("CL: ", distance);
        }
        
        if (distance > 5)
            continue;
        if (closestDistance >= distance) {
            closestDistance = distance;
            closestIndex = i;
        }
    }
    if (closestIndex >= 0) {
        gameSession.levelEditSelectedWallIndex = closestIndex;
        gameSession.levelEditSelectedItemIndex = -1;
    }

    // Check if user selected an item
    closestIndex = -1;
    for (i = 0; i < gameSession.currentLevel.levelItemCount; i++) {
        int dimension = level_itemTypeDimension(gameSession.currentLevel.levelItems[i].itemType) / 2;
        int distance = movement_distanceBetweenCoordinates(gameSession.currentLevel.levelItems[i].position, viewport_convertedCoordinateInverted(gameSession.lastPenInput.touchStartCoordinate));
        if (distance > dimension)
            continue;
        if (closestDistance > distance) {
            closestDistance = distance;
            closestIndex = i;
        }
    }
    if (closestIndex >= 0) {
        gameSession.levelEditSelectedItemIndex = closestIndex;
        gameSession.levelEditSelectedWallIndex = -1;
        return true;
    }

    return true;
}

static void gameSession_progressGameLogicPenInput() {
    if (gameSession.paused) {
        return;
    }
    if (gameSession.lastPenInput.wasUpdatedFlag) {
        gameSession.lastPenInput.wasUpdatedFlag = false;

        if (gameSession_handleButtonTap()) {
            return;
        }
        if (gameSession_handleLevelEditor()) {
            return;
        }

        if (gameSession.lastPenInput.touching && gameSession.lastPenInput.didMove) {
            gameSession.isAiming = true;
            gameSession.userScroll = false;
        } else if (!gameSession.lastPenInput.touching) {
            gameSession.isAiming = false;
            if (gameSession.lastPenInput.didMove) {
                gameSession_launchBall();
            }
        }
    }
}

static UInt8 gameSession_powerForLineLength(UInt16 lineLength) {
    return fmax(1, fmin(9.0f, (float)lineLength / 50.0f * 9.0f));
}

static void gameSession_launchBall() {
    float angleRad;
    Line targetLine;
    UInt8 power;

    if (gameSession.ballTrajectory.lineCount > 0) {
        targetLine.startpoint = gameSession.ballTrajectory.lines[gameSession.ballTrajectory.lineCount - 1].endpoint;
    } else {
        targetLine.startpoint = gameSession.ballPosition;
    }
    targetLine.endpoint = viewport_convertedCoordinateInverted(gameSession.lastPenInput.touchEndCoordinate);
    power = gameSession_powerForLineLength(movement_lineDistance(&targetLine));
    gameSession.ballInertia = power * (10 + power / 2);

    angleRad = movement_angleBetweenRad(gameSession.ballPosition, viewport_convertedCoordinateInverted(gameSession.lastPenInput.touchEndCoordinate));
    movement_lineToTarget(gameSession.ballPosition, angleRad, gameSession.ballInertia, &targetLine);
    movement_cleanupTrajectory(&gameSession.ballTrajectory);
    gameSession.ballTrajectory = movement_trajectory(&targetLine, gameSession.currentLevel.walls, gameSession.currentLevel.wallCount);
    gameSession.ballLaunchTimestamp = TimGetTicks();

    if (gameSession.gameMode == NORMALGAME) {
        gameSession.remainingShots--;
    }
    gameSession.needsHeaderRefresh = true;
    gameSession.rejectPenInput = true;
}

static void gameSession_levelComplete() {
    if (gameSession.level + 1 >= gameSession.levelCount) {
        FrmCustomAlert(GAME_ALERT_GAMECOMPLETE, NULL, NULL, NULL);
        gameSession.levelPackFinished = true;
    } else {
        gameSession_end(false);
        gameSession.level++;
        gameSession_startNew(gameSession.originalGameMode);
    }
}

void gameSession_levelRestart(int newLevel, GameMode gameMode) {
    gameSession_end(false);
    if (newLevel >= 0) {
        gameSession.level = newLevel;
    }
    gameSession_startNew(gameMode);
}

void gameSession_openBlankLevel() {
    gameSession_end(false);
    gameSession_startNew(LEVELEDIT);
}

static Boolean gameSession_checkIfBallIsInHole(int remainingIntertia) {
    Coordinate holeCoordinate;
    if (remainingIntertia > 10) { // Going to fast
        return false;
    }
    holeCoordinate = level_coordinateForItem(HOLE, &gameSession.currentLevel);
    if (movement_distanceBetweenCoordinates(gameSession.ballPosition, holeCoordinate) < 5) {
        gameSession_levelComplete();
        return true;
    }
    return false;
}

static void gameSession_checkForGameOver(float timePassedScale) {
    Int16 buttonPressed;
    if (gameSession.remainingShots > 0 || timePassedScale < 1.0f)
        return;

    buttonPressed = FrmCustomAlert(GAME_ALERT_GAMEOVER, NULL, NULL, NULL);
    switch (buttonPressed) {
    case 0: // RETRY
        gameSession_levelRestart(-1, gameSession.originalGameMode);
        break;
    case 1: // EXIT
        gameSession.levelPackFinished = true;
        break;
    }
}

static void gameSession_progressUpdateBall() {
    Int32 timeSinceBallLaunch, remainingInertia;
    float timePassedScale;
    if (gameSession.ballTrajectory.lineCount <= 0) {
        return;
    }
    timeSinceBallLaunch = TimGetTicks() - gameSession.ballLaunchTimestamp;
    timePassedScale = (float)timeSinceBallLaunch / (float)SysTicksPerSecond();
    remainingInertia = (float)gameSession.ballInertia - (float)gameSession.ballInertia * timePassedScale;

    if (remainingInertia > 0) {
        Coordinate lineOffset = movement_coordinateAtPercentageOfTrajectory(gameSession.ballTrajectory, timePassedScale);
        if (lineOffset.x != 0 && lineOffset.y != 0) {
            gameSession.ballPosition.x = lineOffset.x;
            gameSession.ballPosition.y = lineOffset.y;
        }
        gameSession_updateViewPortOffset();
    } else {
        gameSession.rejectPenInput = false;
        gameSession.ballPosition.x = gameSession.ballTrajectory.lines[gameSession.ballTrajectory.lineCount - 1].endpoint.x;
        gameSession.ballPosition.y = gameSession.ballTrajectory.lines[gameSession.ballTrajectory.lineCount - 1].endpoint.y;
    }
    if (!gameSession_checkIfBallIsInHole(remainingInertia)) {
        gameSession_checkForGameOver(timePassedScale);
    }
}

// User is aiming the ball, return a line from the center of the ball to the touch point of the user
void gameSession_aimingLine(Line *aimingLine, Line *leftArrowLine, Line *rightArrowLine, int *power) {
    float angleRad;
    int distance;
    aimingLine->startpoint.x = gameSession.ballPosition.x;
    aimingLine->startpoint.y = gameSession.ballPosition.y;
    aimingLine->endpoint.x = gameSession.lastPenInput.touchEndCoordinate.x;
    aimingLine->endpoint.y = gameSession.lastPenInput.touchEndCoordinate.y;
    aimingLine->endpoint = viewport_convertedCoordinateInverted(aimingLine->endpoint);
    distance = movement_distanceBetweenCoordinates(aimingLine->startpoint, aimingLine->endpoint);
    *power = gameSession_powerForLineLength(distance);
    angleRad = movement_angleBetweenRad(aimingLine->startpoint, aimingLine->endpoint);
    if (gameSession.level > 0) {
        movement_lineToTarget(aimingLine->startpoint, angleRad, fmin(25, distance), aimingLine);
    }
    movement_lineToTarget(aimingLine->endpoint, angleRad - DEGREESRAD45 - DEGREESRAD180, 6, leftArrowLine);
    movement_lineToTarget(aimingLine->endpoint, angleRad + DEGREESRAD45 - DEGREESRAD180, 6, rightArrowLine);
}

void gameSession_progressGameLogic() {
    gameSession_progressGameLogicPenInput();
    gameSession_progressUpdateBall();
}