#include "startscreen.h"
#include "../constants.h"
#include "../database.h"
#include "../game/drawhelper.h"
#include "../game/movement.h"
#include "../resources.h"
#include "about.h"
#include <PalmOS.h>

typedef enum ActiveScreen {
    MAINMENU,
    DATABASELIST,
    NEWDATABASENAMEINPUT,
    ABOUT
} ActiveScreen;

typedef struct StartScreenSession {
    char dbNames[MAX_DB_COUNT][DB_NAME_LEN];
    LocalID dbIds[MAX_DB_COUNT];
    UInt16 *dbCount;
    Line ballPath;
    Coordinate ballPosition;
    Int32 ballLaunchTimestamp;
    Int32 ballInertia;
    Boolean movingBall;
    WinHandle backgroundBuffer;
    GameMode selectedGameMode;
} StartScreenSession;

StartScreenSession session;

// STATIC DEF
static void startscreen_drawLayout();
static Boolean startscreen_handlePenInput();
static void startscreen_showDatabaseList(Boolean practiceMode);
static ActiveScreen startscreen_activeScreen();
static void startscreen_fillDatabaseList();
static Boolean startscreen_handleDatabaseSelection(EventPtr eventptr, startGameCallback_t callback);
static Boolean startscreen_handleDatabaseNameInput(EventPtr eventptr);
static void startscreen_closeOpenDialogs();
static void startscreen_drawMenuChoices();
static void startscreen_drawHeader();
static void startscreen_drawMovingBall();
static void startscreen_updateBallPosition();
static Coordinate startscreen_randomCoordinate(Boolean outsideScreen);
static void startscreen_requestNewDatabaseName();

// PUBLIC

void startscreen_setup() {
    FormType *frmP = FrmInitForm(MAINMENU_FORM);
    FrmSetActiveForm(frmP);
    session.ballPath.endpoint = startscreen_randomCoordinate(false);
}

Boolean startscreen_eventHandler(EventPtr eventptr, startGameCallback_t callback) {
    if (startscreen_handleDatabaseSelection(eventptr, callback)) {
        return true;
    }
    if (startscreen_handleDatabaseNameInput(eventptr)) {
        return true;
    }
    if (about_buttonHandler(eventptr, MAINMENU_FORM)) {
        return true;
    }
    if (startscreen_activeScreen() == MAINMENU) {
        if (startscreen_handlePenInput(eventptr))
            return true;
        startscreen_drawLayout();
        startscreen_updateBallPosition();
    }
    return false;
}

void startscreen_cleanup() {
    FormType *frmP;
    startscreen_closeOpenDialogs();
    frmP = FrmGetActiveForm();
    if (frmP != NULL) {
        FrmDeleteForm(frmP);
        frmP = NULL;
    }
    if (session.dbCount != NULL) {
        MemPtrFree(session.dbCount);
        session.dbCount = NULL;
    }
    if (session.backgroundBuffer != NULL) {
        WinDeleteWindow(session.backgroundBuffer, false);
        session.backgroundBuffer = NULL;
    }
}

// STATIC

static void startscreen_closeOpenDialogs() {
    if (startscreen_activeScreen() != MAINMENU) {
        FrmReturnToForm(MAINMENU_FORM);
    }
}

static Boolean startscreen_handleDatabaseNameInput(EventPtr eventptr) {
    FieldType *fld;
    FormPtr frm;
    if (startscreen_activeScreen() != NEWDATABASENAMEINPUT || eventptr->eType != ctlSelectEvent)
        return false;

    switch (eventptr->eType) {
    case ctlSelectEvent:
        switch (eventptr->data.ctlSelect.controlID) {
        case MAINMENU_FORM_NEWDATABASE_BUTTON_CREATE:
            frm = FrmGetActiveForm();
            fld = (FieldType *)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, MAINMENU_FORM_NEWDATABASE_INPUTFIELD));
            database_create(FldGetTextPtr(fld));
            FrmReturnToForm(MAINMENU_FORM_LEVELPACK_SELECTION);
            startscreen_fillDatabaseList();
            return true;
        default:
            FrmReturnToForm(MAINMENU_FORM_LEVELPACK_SELECTION);
            return true;
        }
    default:
        return false;
    }
}

static Boolean startscreen_handleDatabaseSelection(EventPtr eventptr, startGameCallback_t callback) {
    Int16 selectedIndex;
    ListType *listP;
    UInt16 listIndex;
    FormType *frmP;

    if (startscreen_activeScreen() != DATABASELIST)
        return false;

    switch (eventptr->eType) {
    case ctlSelectEvent:
        switch (eventptr->data.ctlSelect.controlID) {
        case MAINMENU_FORM_LEVELPACK_BUTTON_GO:
            frmP = FrmGetActiveForm();
            listIndex = FrmGetObjectIndex(frmP, MAINMENU_FORM_LEVELPACK_LIST);
            listP = FrmGetObjectPtr(frmP, listIndex);
            selectedIndex = LstGetSelection(listP);
            startscreen_cleanup();
            callback(session.dbIds[selectedIndex], session.selectedGameMode);
            return true;
        case MAINMENU_FORM_LEVELPACK_BUTTON_NEW:
            startscreen_requestNewDatabaseName();
            return true;
        default:
            startscreen_closeOpenDialogs();
            return true;
        }
    default:
        return false;
    }
}

static ActiveScreen startscreen_activeScreen() {
    switch (FrmGetActiveFormID()) {
    case MAINMENU_FORM_LEVELPACK_SELECTION:
        return DATABASELIST;
    case ABOUT_FORM:
        return ABOUT;
    case MAINMENU_FORM_NEWDATABASE:
        return NEWDATABASENAMEINPUT;
    default:
        return MAINMENU;
    }
}

static void startscreen_drawList(Int16 itemNum, RectangleType *bounds, Char **itemsText) {
    WinDrawChars(session.dbNames[itemNum], StrLen(session.dbNames[itemNum]), bounds->topLeft.x, bounds->topLeft.y);
}

static void startscreen_fillDatabaseList() {
    ListType *lst;
    FormType *frmP;

    if (startscreen_activeScreen() != DATABASELIST)
        return;

    if (session.dbCount == NULL) {
        session.dbCount = (UInt16 *)MemPtrNew(sizeof(UInt16));
    }

    database_levelPackDatabasesList(session.dbNames, session.dbIds, session.dbCount);

    frmP = FrmGetActiveForm();
    lst = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, MAINMENU_FORM_LEVELPACK_LIST));
    LstSetDrawFunction(lst, startscreen_drawList);
    LstSetListChoices(lst, NULL, *session.dbCount);
    FrmDrawForm(frmP);
}

static void startscreen_requestNewDatabaseName() {
    FormType *frmP = FrmInitForm(MAINMENU_FORM_NEWDATABASE);
    FrmSetActiveForm(frmP);
    FrmDrawForm(frmP);
}

static void startscreen_showDatabaseList(Boolean practiceMode) {
    FormType *frmP = FrmInitForm(MAINMENU_FORM_LEVELPACK_SELECTION);
    FrmSetActiveForm(frmP);
    FrmDrawForm(frmP);
    startscreen_fillDatabaseList();
}

static void startscreen_didSelectMenuItem(int index) {
    switch (index) {
    case 0: // practice
        session.selectedGameMode = PRACTICEGAME;
        startscreen_showDatabaseList(false);
        break;
    case 1: // new game
        session.selectedGameMode = NORMALGAME;
        startscreen_showDatabaseList(false);
        break;
    case 2: // LEVEL EDIT
        session.selectedGameMode = LEVELEDIT;
        startscreen_showDatabaseList(false);
        break;
    case 3: // ABOUT
        about_open();
        break;
    default:
        break;
    }
}

static Boolean startscreen_handlePenInput(EventPtr eventptr) {
    int segmentHeight, i;
    if (eventptr->eType != penDownEvent)
        return false;

    switch (startscreen_activeScreen()) {
    case MAINMENU:
        segmentHeight = (MAINMENU_HEIGHT - MAINMENU_HEADER_HEIGHT) / 4;
        for (i = 0; i < 4; i++) {
            int startY = MAINMENU_HEADER_HEIGHT + i * segmentHeight;
            if (eventptr->screenY >= startY && eventptr->screenY <= startY + segmentHeight) {
                startscreen_didSelectMenuItem(i);
                return true;
            }
        }
        break;
    case DATABASELIST:
    case NEWDATABASENAMEINPUT:
    case ABOUT:
        break;
    }

    return false;
}

static void startscreen_drawHeader() {
    FontID oldFont = FntSetFont(boldFont);
    int i, xOffset;
    MemHandle resourceHandle = DmGetResource('tSTR', STRING_APP_TITLE);
    char *titleText = (char *)MemHandleLock(resourceHandle);
    RectangleType rect;
    rect.topLeft.x = 0;
    rect.topLeft.y = 0;
    rect.extent.x = MAINMENU_WIDTH;
    rect.extent.y = MAINMENU_HEADER_HEIGHT;

    drawhelper_applyForeColor(BELIZEHOLE);
    WinDrawRectangle(&rect, 0);
    drawhelper_applyBackgroundColor(BELIZEHOLE);
    drawhelper_applyTextColor(CLOUDS);
    drawhelper_drawText(titleText, coordinate((MAINMENU_WIDTH - FntCharsWidth(titleText, StrLen(titleText))) / 2, 5));
    MemHandleUnlock(resourceHandle);
    DmReleaseResource(resourceHandle);
    FntSetFont(oldFont);

    xOffset = 5;
    for (i = 0; i < 2; i++) {
        drawhelper_loadAndDrawImage(RESOURCE_GFX_BALL, coordinate(xOffset + (RESOURCE_GFX_BALLDIMENSION + 1) * i, 7));
    }

    xOffset = 5;
    for (i = 0; i < 2; i++) {
        drawhelper_loadAndDrawImage(RESOURCE_GFX_BALL, coordinate(MAINMENU_WIDTH - xOffset - RESOURCE_GFX_BALLDIMENSION + -(RESOURCE_GFX_BALLDIMENSION + 1) * i, 7));
    }
}

static void startscreen_drawMovingBall() {
    if (session.movingBall) {
        drawhelper_loadAndDrawImage(RESOURCE_GFX_HOLE, session.ballPath.endpoint);
        drawhelper_loadAndDrawImage(RESOURCE_GFX_BALL, session.ballPosition);
    } else { // moving hole
        drawhelper_loadAndDrawImage(RESOURCE_GFX_HOLE, session.ballPosition);
    }
}

static void startscreen_drawMenuChoices() {
    FontID oldFont;
    MemHandle resourceHandle;
        char *text;
    int i;
    int segmentCount = 4;
    int segmentColors[4] = {EMERALD, BELIZEHOLE, CARROT, ALIZARIN};
    int segmentHeight = (MAINMENU_HEIGHT - MAINMENU_HEADER_HEIGHT) / segmentCount;

    RectangleType rect;
    rect.topLeft.x = 0;
    rect.extent.x = MAINMENU_WIDTH;
    rect.extent.y = segmentHeight;

    oldFont = FntSetFont(largeBoldFont);
    for (i = 0; i < segmentCount; i++) {
        UInt16 textResourceId;
        int y, grassCount, gfxResource;
        drawhelper_applyForeColor(segmentColors[i]);
        drawhelper_applyBackgroundColor(segmentColors[i]);
        drawhelper_applyTextColor(CLOUDS);
        rect.topLeft.y = MAINMENU_HEADER_HEIGHT + i * segmentHeight;
        WinDrawRectangle(&rect, 0);

        switch (i) {
        case 0:
            gfxResource = RESOURCE_GFX_Z21;
            grassCount = 3;
            textResourceId = STRING_PRACTICE;
            break;
        case 1:
            gfxResource = RESOURCE_GFX_VX;
            grassCount = 4;
            textResourceId = STRING_NEWGAME;
            break;
        case 2:
            gfxResource = RESOURCE_GFX_TC;
            grassCount = 4;
            textResourceId = STRING_CREATELEVEL;
            break;
        case 3:
            gfxResource = RESOURCE_GFX_TREO650;
            grassCount = 3;
            textResourceId = STRING_ABOUT;
            break;
        default:
            break;
        }

        resourceHandle = DmGetResource('tSTR', textResourceId);
        text = (char *)MemHandleLock(resourceHandle);
        drawhelper_drawText(text, coordinate(MAINMENU_WIDTH - FntCharsWidth(text, StrLen(text)) - 10,
                                             rect.topLeft.y + (segmentHeight / 2) - 5));
        MemHandleUnlock(resourceHandle);
        DmReleaseResource(resourceHandle);

        drawhelper_loadAndDrawImage(gfxResource, coordinate(5, rect.topLeft.y + 8));

        for (y = 0; y < grassCount; y++) {
            drawhelper_loadAndDrawImage(RESOURCE_GFX_GRASS, coordinate(MAINMENU_WIDTH - RESOURCE_GFX_GRASSDIMENSION + -RESOURCE_GFX_GRASSDIMENSION * y, rect.topLeft.y + segmentHeight - RESOURCE_GFX_GRASSDIMENSION + 5));
        }
    }
    FntSetFont(oldFont);


    resourceHandle = DmGetResource('tver', 1);
    text = (char *)MemHandleLock(resourceHandle);
    drawhelper_drawText(text, coordinate(45, MAINMENU_HEIGHT - 10));
     MemHandleUnlock(resourceHandle);
    
}

static UInt16 startscreen_randomIntBetween(UInt16 min, UInt16 max) {
    UInt16 range = max - min + 1;
    UInt16 randomValue = SysRandom(0) % range;
    return min + randomValue;
}

static Coordinate startscreen_randomCoordinate(Boolean outsideScreen) {
    if (outsideScreen) {
        return coordinate(-startscreen_randomIntBetween(0, MAINMENU_WIDTH - 30), startscreen_randomIntBetween(0, MAINMENU_HEIGHT));
    } else {
        return coordinate(startscreen_randomIntBetween(0, MAINMENU_WIDTH - 30), startscreen_randomIntBetween(0, MAINMENU_HEIGHT));
    }
}

static void startscreen_updateBallPosition() {
    Int32 timeSinceBallLaunch, remainingInertia;
    float timePassedScale;
    float easingPercentage;
    timeSinceBallLaunch = TimGetTicks() - session.ballLaunchTimestamp;
    timePassedScale = (float)timeSinceBallLaunch / (float)SysTicksPerSecond();
    remainingInertia = (float)session.ballInertia - (float)session.ballInertia * timePassedScale;

    if (remainingInertia <= 0) {
        session.movingBall = !session.movingBall;
        if (!session.movingBall) {
            session.ballPath.startpoint = session.ballPath.endpoint;
            session.ballPath.endpoint = startscreen_randomCoordinate(false);
            session.ballPosition = session.ballPath.startpoint;
        } else {
            session.ballPath.startpoint = startscreen_randomCoordinate(true);
            session.ballPosition = session.ballPath.startpoint;
        }
        session.ballLaunchTimestamp = TimGetTicks();
        session.ballInertia = movement_lineDistance(&session.ballPath);
    } else {
        easingPercentage = 1.0 - (1.0 - timePassedScale) * (1.0 - timePassedScale);
        session.ballPosition = movement_coordinateAtPercentageOfLine(session.ballPath, easingPercentage);
    }
}

static void startscreen_drawLayout() {
    RectangleType lamerect;
    WinHandle mainWindow = WinGetDrawWindow();
    Err err = errNone;
    WinHandle screenBuffer = WinCreateOffscreenWindow(MAINMENU_WIDTH, MAINMENU_HEIGHT, screenFormat, &err);

    if (session.backgroundBuffer == NULL) {
        session.backgroundBuffer = WinCreateOffscreenWindow(MAINMENU_WIDTH, MAINMENU_HEIGHT, screenFormat, &err);
        WinSetDrawWindow(session.backgroundBuffer);
        startscreen_drawHeader();
        startscreen_drawMenuChoices();
    }

    RctSetRectangle(&lamerect, 0, 0, MAINMENU_WIDTH, MAINMENU_HEIGHT);
    WinCopyRectangle(session.backgroundBuffer, screenBuffer, &lamerect, 0, 0, winPaint);

    WinSetDrawWindow(screenBuffer);
    startscreen_drawMovingBall();

    RctSetRectangle(&lamerect, 0, 0, MAINMENU_WIDTH, MAINMENU_HEIGHT);
    WinCopyRectangle(screenBuffer, mainWindow, &lamerect, 0, 0, winPaint);

    WinSetDrawWindow(mainWindow);
    WinDeleteWindow(screenBuffer, false);
}