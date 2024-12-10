#include "game.h"

#include <PalmOS.h>
#include <string.h>

#include "../constants.h"
#include "../database.h"
#include "../resources.h"
#include "../startscreen/about.h"
#include "colors.h"
#include "debugWindow.h"
#include "drawhelper.h"
#include "gamesession.h"
#include "inputPen.h"
#include "level.h"
#include "movement.h"
#include "polygon.h"
#include "scoreWindow.h"
#include "viewport.h"

// Local function definitions
static void game_drawLayout();
static void game_drawGameWindow();

WinHandle backgroundBuffer = NULL;

int game_eventDelayTime() {
    if (gameSession.paused) {
        return evtWaitForever;
    } else {
        return 0;
    }
}

static void game_toggleLevelEditor() {
    if (gameSession.gameMode != LEVELEDIT) {
        gameSession.gameMode = LEVELEDIT;
        gameSession.levelEditSelectedWallIndex = -1;
        gameSession.levelEditSelectedItemIndex = -1;
    } else {
        if (gameSession.originalGameMode == LEVELEDIT) {
            gameSession.gameMode = PRACTICEGAME;
        } else {
            gameSession.gameMode = gameSession.originalGameMode;
        }
    }
    gameSession.viewportOffset = coordinate(0, 0);
    gameSession.needsHeaderRefresh = true;
    gameSession.needsBackgroundRefresh = true;
    level_updateStatistics(&gameSession.currentLevel);
    if (gameSession.gameMode != LEVELEDIT) { // ask to write changes
        Int16 formId = gameSession.level == gameSession.levelCount ? GAME_ALERT_SAVENEWLEVEL: GAME_ALERT_SAVELEVEL;
        Int16 buttonPressed = FrmCustomAlert(formId, NULL, NULL, NULL);
        if (formId == GAME_ALERT_SAVENEWLEVEL) {
            buttonPressed++;
        }
        switch (buttonPressed) {
        case 0: // SAVE
            database_writePackedLevel(&gameSession.currentLevel, gameSession.dbId);
            database_moveRecord(fmax(0, database_levelCount(gameSession.dbId) - 1), gameSession.level, true, gameSession.dbId);
            break;
        case 1: // SAVE AS NEW LEVEL
            database_writePackedLevel(&gameSession.currentLevel, gameSession.dbId);
            gameSession_levelRestart(fmax(0, database_levelCount(gameSession.dbId) - 1), gameSession.originalGameMode);
            break;
        default:
            break;
        }
    }
}

static void game_openLevelEditAddDialog() {
    FormType *frmP = FrmInitForm(GAME_FORM_LEVELEDIT_ADD);
    FrmSetActiveForm(frmP);
    FrmDrawForm(frmP);
    if (gameSession.gameMode != LEVELEDIT) {
        game_toggleLevelEditor();
    }
}

static void game_addItemType(LevelItemType levelItemType) {
    LevelItem item;
    item.itemType = levelItemType;
    item.position = viewport_convertedCoordinateInverted(coordinate(40, 40));
    level_addNewItem(item, &gameSession.currentLevel);
    gameSession.levelEditSelectedItemIndex = gameSession.currentLevel.levelItemCount - 1;
}

static void game_applyLevelEditAddSelection() {
    LevelItem item;
    Int16 selectedIndex;
    ListType *listP;
    UInt16 listIndex;
    listIndex = FrmGetObjectIndex(FrmGetActiveForm(), GAME_FORM_LEVELEDIT_ADD_LIST);
    listP = FrmGetObjectPtr(FrmGetActiveForm(), listIndex);
    selectedIndex = LstGetSelection(listP);

    gameSession.levelEditSelectedWallIndex = -1;
    gameSession.levelEditSelectedItemIndex = -1;

    switch (selectedIndex) {
    case 0: // ADD WALL
        item.itemType = WALL;
        item.position = viewport_convertedCoordinateInverted(coordinate(40, 40));
        item.positionB = viewport_convertedCoordinateInverted(coordinate(80, 40));
        level_addNewItem(item, &gameSession.currentLevel);
        gameSession.levelEditSelectedWallIndex = gameSession.currentLevel.wallCount - 1;
        break;
    case 1: // ADD WINDMILL
        game_addItemType(WINDMILL);
        break;
    case 2: // GRASS
        game_addItemType(GRASS);
        break;
    case 3: // UFO
        game_addItemType(UFO);
        break;
    case 4: // CHICKEN
        game_addItemType(CHICKEN);
        break;
    default:
        break;
    }
}

static Boolean game_levelEditAddDialogHandler(EventType *eventptr) {
    switch (eventptr->eType) {
    case ctlSelectEvent:
        switch (eventptr->data.ctlSelect.controlID) {
        case GAME_FORM_LEVELEDIT_ADD_BUTTON_CANCEL:
            FrmReturnToForm(GAME_FORM);
            return true;
        case GAME_FORM_LEVELEDIT_ADD_BUTTON_ADD:
            game_applyLevelEditAddSelection();
            FrmReturnToForm(GAME_FORM);
            return true;
        default:
            return true;
        }
    default:
        return false;
    }
}

static Boolean game_menuEventHandler(EventType *eventptr) {
    if (eventptr->eType == menuEvent) {
        gameSession_clearPenInput();
        switch (eventptr->data.menu.itemID) {
        case GAME_MENU_ACTION_SCROLL:
            gameSession.userScroll = !gameSession.userScroll;
            gameSession.needsHeaderRefresh = true;
            if (!gameSession.userScroll) {
                gameSession_updateViewPortOffset();
            }
            return true;
        case GAME_MENU_ACTION_RESTART_LEVEL:
            gameSession_levelRestart(-1, gameSession.originalGameMode);
            return true;
        case GAME_MENU_ACTION_EXIT:
            gameSession.levelPackFinished = true;
            return true;
        case GAME_MENU_ACTION_LEVELEDIT_TOGGLE:
            game_toggleLevelEditor();
            return true;
        case GAME_MENU_ACTION_LEVELEDIT_ADD:
            game_openLevelEditAddDialog();
            return true;
        case GAME_MENU_ACTION_LEVELEDIT_NEW_LEVEL:
            gameSession_openBlankLevel();
            return true;
        case GAME_MENU_ACTION_LEVELEDIT_DELETE_SELECTED:
            if (gameSession.levelEditSelectedWallIndex >= 0) {
                level_removeWall(&gameSession.currentLevel, gameSession.levelEditSelectedWallIndex);
                gameSession.levelEditSelectedWallIndex = -1;
            } else if (gameSession.levelEditSelectedItemIndex >= 0) {
                level_removeItem(&gameSession.currentLevel, gameSession.levelEditSelectedItemIndex);
                gameSession.levelEditSelectedItemIndex = -1;
            }
            return true;
        case GAME_MENU_ABOUT:
            about_open();
            return true;
        default:
            return false;
            break;
        }
    }

    return false;
}

static Boolean game_checkIfGameIsPaused(EventType *eventptr) {
    if (eventptr->eType == winExitEvent) {
        if (eventptr->data.winExit.exitWindow ==
            (WinHandle)FrmGetFormPtr(GAME_FORM)) {
            gameSession.paused = true;
        }
    } else if (eventptr->eType == winEnterEvent) {
        if (eventptr->data.winEnter.enterWindow ==
                (WinHandle)FrmGetFormPtr(GAME_FORM) &&
            eventptr->data.winEnter.enterWindow == (WinHandle)FrmGetFirstForm()) {
            gameSession.paused = false;
        }
    }

    return gameSession.paused;
}

Boolean game_restore() {
    FormType *frmP;
    GameRestorableSessionData *sessionData;
    sessionData = gameSession_loadGameState();
    if (sessionData == NULL) {
        gameSession_clearSavedGameState();
        return false;
    }
    frmP = FrmInitForm(GAME_FORM);
    FrmSetActiveForm(frmP);
    debugWindow_setup();
    MemSet(&gameSession, sizeof(GameSession), 0);
    gameSession.dbId = sessionData->dbId;
    gameSession.levelPackFinished = false;
    gameSession_levelRestart(sessionData->level, sessionData->gameMode);
    gameSession.ballPosition = sessionData->ballPosition;
    gameSession.remainingShots = sessionData->remainingShots;
    gameSession_clearSavedGameState();
    gameSession_updateViewPortOffset();
    return true;
}

void game_setup(LocalID dbId, GameMode gameMode) {
    FormType *frmP = FrmInitForm(GAME_FORM);
    FrmSetActiveForm(frmP);
    debugWindow_setup();
    MemSet(&gameSession, sizeof(GameSession), 0);
    gameSession.dbId = dbId;
    gameSession.levelPackFinished = false;
    gameSession_levelRestart(0, gameMode);
}

void game_end(Boolean saveGameState) {
    if (saveGameState) {
        gameSession_saveGameState();
    }
    gameSession_end(true);
    if (backgroundBuffer != NULL) {
        WinDeleteWindow(backgroundBuffer, false);
        backgroundBuffer = NULL;
    }
}

Boolean game_mainLoop(EventPtr eventptr, openMainMenuCallback_t requestMainMenu) {
    if (gameSession.levelPackFinished) {
        game_end(false);
        requestMainMenu();
        return true;
    }

    if (game_menuEventHandler(eventptr)) {
        return true;
    }

    if (about_buttonHandler(eventptr, GAME_FORM)) {
        return true;
    }

    if (scoreWindow_handleTaps(eventptr)) {
        return true;
    }

    if (game_levelEditAddDialogHandler(eventptr)) {
        return true;
    }

    if (game_checkIfGameIsPaused(eventptr)) {
        return false;
    }

    gameSession_registerPenInput(eventptr);
    if (eventptr->eType != nilEvent)
        return false;
    if (game_timeUntilNextEvent() <= 0) {
        gameSession_scheduleNextGameLogicProgression();
        gameSession_progressGameLogic();
    }
    game_drawLayout();
    return true;
}

static IndexedColorType game_colorForPower(int power) {
    switch (power) {
    case 0:
    case 1:
    case 2:
        return EMERALD;
    case 3:
    case 4:
    case 5:
        return CARROT;
        break;
    case 6:
    case 7:
    case 8:
    default:
        return ALIZARIN;
    }
}

static void game_drawPowerMeter(int power) {
    int i;
    int segmentHeight = 8;
    int bottomY = GAMEWINDOW_HEIGHT / 2 + 3 * segmentHeight;
    RectangleType rect;
    rect.topLeft.x = GAMEWINDOW_WIDTH - 15;
    rect.extent.x = 10;
    rect.extent.y = segmentHeight;

    for (i = 0; i < power; i++) {
        IndexedColorType color = game_colorForPower(i);
        drawhelper_applyForeColor(color);
        rect.topLeft.y = bottomY - segmentHeight * i - i;
        WinDrawRectangle(&rect, 0);
        drawhelper_applyForeColor(CLOUDS);
        WinDrawRectangleFrame(rectangleFrame, &rect);
    }
}

static void game_drawAimingLineIfNeeded() {
    Line aimingLine, leftArrowLine, rightArrowLine;
    IndexedColorType color;
    int power;
    if (!gameSession.isAiming)
        return;

    gameSession_aimingLine(&aimingLine, &leftArrowLine, &rightArrowLine, &power);
    aimingLine = viewport_convertedLine(aimingLine);
    leftArrowLine = viewport_convertedLine(leftArrowLine);
    rightArrowLine = viewport_convertedLine(rightArrowLine);

    color = game_colorForPower(power);
    drawhelper_applyForeColor(color);
    drawhelper_drawLine(&aimingLine);
    if (power > 1) {
        drawHelper_drawTriangle(leftArrowLine.startpoint, leftArrowLine.endpoint, rightArrowLine.endpoint);
    }
    game_drawPowerMeter(power);
}

static void game_drawWalls(Boolean useRelativeCoordinates) {
    int i;

    for (i = 0; i < gameSession.currentLevel.wallCount; i++) {
        Line line = gameSession.currentLevel.walls[i];
        if (useRelativeCoordinates) {
            line = viewport_convertedLine(line);
        }
        if (gameSession.gameMode == LEVELEDIT && gameSession.levelEditSelectedWallIndex == i) {
            drawhelper_applyForeColor(ALIZARIN);
            drawhelper_drawBoxAround(line.startpoint, 0);
            drawhelper_drawBoxAround(line.endpoint, 0);
        } else {
            drawhelper_applyForeColor(CLOUDS);
        }
        drawhelper_drawLine(&line);
        #ifdef DEBUG
        drawhelper_drawTextWithValue("x", i, line.startpoint);
        #endif
    }
}

static void game_drawPixel4Bit(UInt8 *framebuffer, UInt16 screenWidth, int x, int y, UInt8 colorIndex) {
    UInt16 rowBytes = (screenWidth + 1) / 2;
    UInt32 offset = y * rowBytes + (x / 2);
    UInt8 currentByte = framebuffer[offset];
    if (x % 2 == 0) {
        currentByte = (currentByte & 0x0F) | (colorIndex << 4);
    } else {
        currentByte = (currentByte & 0xF0) | (colorIndex & 0x0F);
    }
    framebuffer[offset] = currentByte;
}

static void game_drawGround() {
    UInt8 *framebuffer;
    UInt16 screenWidth;
    int x, y;
    int outerWallsCount;
    Coordinate *outerWalls;
    if (gameSession.gameMode == LEVELEDIT || gameSession.currentLevel.wallCount <= 2)
        return;

    outerWalls = (Coordinate *)MemPtrNew(sizeof(Coordinate) * gameSession.currentLevel.wallCount * 2);
    polygon_outerPolygon(gameSession.currentLevel.walls, gameSession.currentLevel.wallCount, outerWalls, &outerWallsCount);
    MemPtrResize(outerWalls, outerWallsCount * sizeof(Coordinate));

    framebuffer = (void *)BmpGetBits(WinGetBitmap(backgroundBuffer));
    screenWidth = fmax(gameSession.currentLevel.levelSize.x, GAMEWINDOW_WIDTH);

    for (x = 0; x < gameSession.currentLevel.levelSize.x; x++) {
        for (y = 0; y < gameSession.currentLevel.levelSize.y; y++) {
            if (movement_isInsideWalls(outerWalls, outerWallsCount, coordinate(x, y))) {
                if (gameSession.depth == 4) {
                    game_drawPixel4Bit(framebuffer, screenWidth, x, y, colors_reference[DIRT]);
                } else {
                    framebuffer[y * screenWidth + x] = colors_reference[DIRT];
                }
            }
        }
    }

    MemPtrFree(outerWalls);
}

static void game_drawTrajectory() {
    int i;
#ifndef DEBUG
    return;
#endif
    if (gameSession.ballTrajectory.lineCount == 0)
        return;
    for (i = 0; i < gameSession.ballTrajectory.lineCount; i++) {
        Line line = viewport_convertedLine(gameSession.ballTrajectory.lines[i]);
        if (i % 2 == 0) {
            drawhelper_applyForeColor(CARROT);
        } else {
            drawhelper_applyForeColor(EMERALD);
        }
        drawhelper_drawLine(&line);
    }
}

static void game_drawPositioningButtons() {
    if (!gameSession.userScroll)
        return;

    drawhelper_loadAndDrawImage(RESOURCE_GFX_BUTTON_UP,
                                gameSession_buttonUpPosition());
    drawhelper_loadAndDrawImage(RESOURCE_GFX_BUTTON_DOWN,
                                gameSession_buttonDownPosition());
    drawhelper_loadAndDrawImage(RESOURCE_GFX_BUTTON_LEFT,
                                gameSession_buttonLeftPosition());
    drawhelper_loadAndDrawImage(RESOURCE_GFX_BUTTON_RIGHT,
                                gameSession_buttonRightPosition());
}

static void game_drawBall() {
    Coordinate position;
    if (gameSession.gameMode == LEVELEDIT) {
        position = level_coordinateForItem(BALL, &gameSession.currentLevel);
    } else {
        position = gameSession.ballPosition;
    }
    drawhelper_drawSprite(
        &spriteLibrary.ballSprite,
        viewport_convertedCoordinate(position));
}

static void game_drawSelectedItemBox() {
    if (gameSession.gameMode != LEVELEDIT || gameSession.levelEditSelectedItemIndex < 0)
        return;
    drawhelper_applyForeColor(ALIZARIN);
    drawhelper_drawBoxAround(viewport_convertedCoordinate(gameSession.currentLevel.levelItems[gameSession.levelEditSelectedItemIndex].position), level_itemTypeDimension(gameSession.currentLevel.levelItems[gameSession.levelEditSelectedItemIndex].itemType));
}

static void game_drawDecoration(Boolean animated, Boolean useRelativeCoordinates) {
    int i;
    Coordinate coord;
    for (i = 0; i < gameSession.currentLevel.levelItemCount; i++) {
        coord = gameSession.currentLevel.levelItems[i].position;
        if (useRelativeCoordinates) {
            coord = viewport_convertedCoordinate(coord);
        }
        switch (gameSession.currentLevel.levelItems[i].itemType) {
        case WALL:
        case BALL:
            break;
        case HOLE:
            if (animated)
                continue;
            drawhelper_drawSprite(&spriteLibrary.holeSprite, coord);
            break;
        case GRASS:
            if (animated)
                continue;
            drawhelper_drawSprite(&spriteLibrary.grassSprite, coord);
            break;
        case UFO:
            if (animated)
                continue;
            drawhelper_drawSprite(&spriteLibrary.ufoSprite, coord);
            break;
        case WINDMILL:
            if (!animated)
                continue;
            drawhelper_drawAnimationSprite(&spriteLibrary.windmillSprite, coord, 6);
            break;
        case CHICKEN:
            if (!animated)
                continue;
            drawhelper_drawAnimationSprite(&spriteLibrary.chickenSprite, coord, 2);
            break;
        }
    }
}

static WinHandle game_drawBackground() {
    Err err = errNone;
    if (!gameSession.needsBackgroundRefresh && backgroundBuffer != NULL && gameSession.gameMode != LEVELEDIT) {
        return backgroundBuffer;
    }
    gameSession.needsBackgroundRefresh = false;
    if (backgroundBuffer != NULL) {
        WinDeleteWindow(backgroundBuffer, false);
    }
    backgroundBuffer = WinCreateOffscreenWindow(
        fmax(GAMEWINDOW_WIDTH, gameSession.currentLevel.levelSize.x), fmax(GAMEWINDOW_HEIGHT, gameSession.currentLevel.levelSize.y), screenFormat, &err);
    WinSetDrawWindow(backgroundBuffer);
    game_drawGameWindow();
    game_drawGround();
    if (gameSession.gameMode != LEVELEDIT) { // Not editing, items won't change position so it's safe to draw them on the background
        game_drawWalls(false);
        game_drawDecoration(false, false);
    }
    return backgroundBuffer;
}

static void game_drawLoadingIndicator() {
    FontID oldFont = FntSetFont(largeBoldFont);
    RectangleType rect;
    rect.topLeft.x = 20;
    rect.topLeft.y = 70;
    rect.extent.x = 120;
    rect.extent.y = 34;
    drawhelper_applyForeColor(BELIZEHOLE);
    WinDrawRectangle(&rect, 0);
    drawhelper_loadAndDrawImage(RESOURCE_GFX_FLOPPY, coordinate(rect.topLeft.x + 2, rect.topLeft.y + 2));
    drawhelper_applyTextColor(CLOUDS);
    drawhelper_applyBackgroundColor(BELIZEHOLE);
    drawhelper_drawText("Loading level", coordinate(rect.topLeft.x + 46, rect.topLeft.y + 12));
    drawhelper_applyForeColor(CLOUDS);
    WinDrawRectangleFrame(rectangleFrame, &rect);
    FntSetFont(oldFont);
}

static void game_drawLayout() {
    RectangleType lamerect;
    WinHandle mainWindow = WinGetDrawWindow();
    Err err = errNone;
    WinHandle backgroundBuffer;
    WinHandle screenBuffer = WinCreateOffscreenWindow(
        GAMEWINDOW_WIDTH, GAMEWINDOW_HEIGHT, screenFormat, &err);

    if (gameSession.isLoading) {
        gameSession.isLoading = false;
        game_drawLoadingIndicator();
    }

    WinSetDrawWindow(screenBuffer);
    game_drawGameWindow();

    backgroundBuffer = game_drawBackground();
    RctSetRectangle(&lamerect, gameSession.viewportOffset.x, gameSession.viewportOffset.y, GAMEWINDOW_WIDTH, GAMEWINDOW_HEIGHT);
    WinCopyRectangle(backgroundBuffer, screenBuffer, &lamerect, 0, 0, winPaint);

    WinSetDrawWindow(screenBuffer);
    game_drawTrajectory();
    if (gameSession.gameMode == LEVELEDIT) { // Editing, items will change position so it's draw them on the foreground buffer
        if (gameSession.levelEditSelectedWallIndex > 0) {
            game_drawDecoration(false, true);
            game_drawDecoration(true, true);
            game_drawWalls(true);
        } else {
            game_drawDecoration(false, true);
            game_drawDecoration(true, true);
            game_drawWalls(true);
        }
    }
    game_drawBall();
    if (gameSession.gameMode != LEVELEDIT) {
        game_drawDecoration(true, true);
    }
    
    game_drawSelectedItemBox();
    game_drawAimingLineIfNeeded();
    game_drawPositioningButtons();

    RctSetRectangle(&lamerect, 0, 0, GAMEWINDOW_WIDTH, GAMEWINDOW_HEIGHT);
    WinCopyRectangle(screenBuffer, mainWindow, &lamerect, GAMEWINDOW_X,
                     GAMEWINDOW_Y,
                     winPaint);

    WinSetDrawWindow(mainWindow);
    WinDeleteWindow(screenBuffer, false);

    scoreWindow_draw();
    debugWindow_drawDebugInfo();
}

static void game_drawGameWindow() {
    RectangleType rect;
    rect.topLeft.x = 0;
    rect.topLeft.y = 0;
    rect.extent.x = fmax(gameSession.currentLevel.levelSize.x, GAMEWINDOW_WIDTH);
    rect.extent.y = fmax(gameSession.currentLevel.levelSize.y, GAMEWINDOW_HEIGHT);
    if (gameSession.gameMode == LEVELEDIT) {
        drawhelper_applyForeColor(DIRT);
    } else {
        drawhelper_applyForeColor(EMERALD);
    }
    WinDrawRectangle(&rect, 0);
}
