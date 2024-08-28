#include "scoreWindow.h"
#include "../constants.h"
#include "../resources.h"
#include "colors.h"
#include "drawhelper.h"
#include "gamesession.h"
#include "models.h"
#include <PalmOS.h>

static Coordinate scoreWindow_scrollButtonPosition() {
    return coordinate(SCOREWINDOW_WIDTH - 15, SCOREWINDOW_HEIGHT - 15);
}

static void scoreWindow_openMenu() {
    EventType event;
    MemSet(&event, sizeof(EventType), 0);
    event.eType = keyDownEvent;
    event.data.keyDown.chr = vchrMenu;
    event.data.keyDown.modifiers = commandKeyMask;
    EvtAddEventToQueue(&event);
}

Boolean scoreWindow_handleTaps(EventPtr eventptr) {
    Coordinate scrollButtonPosition;
    if (eventptr->eType != penDownEvent) {
        return false;
    }

    scrollButtonPosition = scoreWindow_scrollButtonPosition();

    if (eventptr->screenX >= scrollButtonPosition.x && eventptr->screenX <= scrollButtonPosition.x + 15 && eventptr->screenY >= scrollButtonPosition.y && eventptr->screenY <= scrollButtonPosition.y + 15) {
        gameSession.userScroll = !gameSession.userScroll;
        gameSession.needsHeaderRefresh = true;
        gameSession_clearPenInput();
        return true;
    } else if (eventptr->screenX <= SCOREWINDOW_WIDTH && eventptr->screenY <= SCOREWINDOW_HEIGHT) {
        scoreWindow_openMenu();
        gameSession_clearPenInput();
        return true;
    }
    return false;
}

static void scoreWindow_drawRemainingShots(UInt8 total, UInt8 remaining, Coordinate position) {
    int i;
    switch (gameSession.gameMode) {
    case NORMALGAME:
        drawhelper_applyForeColor(CLOUDS);
        for (i = 0; i < remaining; i++) {
            Coordinate drawPosition = coordinate(spriteLibrary.ballSprite.size.x / 2 + position.x + i * spriteLibrary.ballSprite.size.x + i, position.y);
            drawhelper_drawSprite(&spriteLibrary.ballSprite, drawPosition);
        }
        break;
    case PRACTICEGAME:
        drawhelper_drawTextFromResource(STRING_PRACTICEMODE, coordinate(position.x, position.y - 5));
        break;
    case LEVELEDIT:
        drawhelper_drawTextFromResource(STRING_OPTIONSINMENU, coordinate(position.x, position.y - 5));
        break;
    }
}

static void scoreWindow_updateDynamicValues() {
    char valueText[10];
    char text[20];
    RectangleType rect;

    rect.topLeft.x = 34;
    rect.topLeft.y = 0;
    rect.extent.x = 65;
    rect.extent.y = 30;
    drawhelper_applyForeColor(BELIZEHOLE);
    WinDrawRectangle(&rect, 0);

    drawhelper_applyTextColor(CLOUDS);
    drawhelper_applyBackgroundColor(BELIZEHOLE);

    // Level info
    StrCopy(text, "Level: ");
    StrIToA(valueText, gameSession.level + 1);
    StrCat(text, valueText);
    StrCat(text, "/");
    StrIToA(valueText, gameSession.levelCount);
    StrCat(text, valueText);
    drawhelper_drawText(text, coordinate(34, 3));

    // Remaining shots
    scoreWindow_drawRemainingShots(GAME_MAX_ATTEMPTS, gameSession.remainingShots, coordinate(34, 20));
}

static void scoreWindow_drawBackdrop() {
    FontID oldFont;
    RectangleType lamerect, rect;
    WinHandle mainWindow = WinGetDrawWindow();
    Err err = errNone;
    WinHandle screenBuffer = WinCreateOffscreenWindow(SCOREWINDOW_WIDTH, SCOREWINDOW_HEIGHT + 1, screenFormat, &err);
    WinSetDrawWindow(screenBuffer);

    rect.topLeft.x = 0;
    rect.topLeft.y = 0;
    rect.extent.x = SCOREWINDOW_WIDTH;
    rect.extent.y = SCOREWINDOW_HEIGHT + 1;
    drawhelper_applyForeColor(BELIZEHOLE);

    WinDrawRectangle(&rect, 0);
    drawhelper_loadAndDrawImage(RESOURCE_GFX_HEADER, coordinate(3, 3));

    drawhelper_applyTextColor(CLOUDS);
    drawhelper_applyBackgroundColor(BELIZEHOLE);
    oldFont = FntSetFont(largeBoldFont);
    if (gameSession.gameMode == LEVELEDIT) {
        drawhelper_drawTextFromResource(STRING_LEVEL, coordinate(SCOREWINDOW_WIDTH - 50, 0));
        drawhelper_drawTextFromResource(STRING_EDITOR, coordinate(SCOREWINDOW_WIDTH - 50, 15));
    } else {
        drawhelper_drawTextFromResource(STRING_APP_TITLE_PREFIX, coordinate(SCOREWINDOW_WIDTH - 60, 0));
        drawhelper_drawTextFromResource(STRING_APP_TITLE_SUFFIX, coordinate(SCOREWINDOW_WIDTH - 60, 15));
    }
    FntSetFont(oldFont);

    drawhelper_loadAndDrawImage(RESOURCE_GFX_BUTTON_MOVE, coordinate(SCOREWINDOW_WIDTH - 15, SCOREWINDOW_HEIGHT - 15));

    if (gameSession.userScroll) {
        Coordinate coord = scoreWindow_scrollButtonPosition();
        Line line;
        line.startpoint = coordinate(coord.x + 2, coord.y + 12);
        line.endpoint = coordinate(coord.x + 15 - 2, coord.y + 12);
        drawhelper_applyForeColor(ALIZARIN);
        drawhelper_drawLine(&line);
    }

    scoreWindow_updateDynamicValues();

    drawhelper_applyForeColor(CLOUDS);
    WinDrawLine(0,
                SCOREWINDOW_HEIGHT - 1,
                SCOREWINDOW_WIDTH,
                SCOREWINDOW_HEIGHT - 1);

    RctSetRectangle(&lamerect, 0, 0, SCOREWINDOW_WIDTH, SCOREWINDOW_HEIGHT + 1);
    WinCopyRectangle(screenBuffer, mainWindow, &lamerect, SCOREWINDOW_X, SCOREWINDOW_Y, winPaint);

    WinSetDrawWindow(mainWindow);
    WinDeleteWindow(screenBuffer, false);
}

void scoreWindow_draw() {
    if (gameSession.needsHeaderRefresh) {
        gameSession.needsHeaderRefresh = false;
        scoreWindow_drawBackdrop();
    }
}