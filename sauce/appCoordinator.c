#include "appCoordinator.h"
#include "game/colors.h"
#include "game/game.h"
#include "startscreen/startscreen.h"
#include "resources.h"
#include <PalmOS.h>

Activity currentActivity = STARTSCREEN;

// STATIC FUNCTION DEFINITIONS
static Boolean appCoordinator_handle(Activity activity, EventPtr eventptr);
static int appCoordinator_eventDelayTime(Activity activity);
static void appCoordinator_launchGame(LocalID dbId, GameMode gameMode);
static void appCoordinator_launchStartScreen();

// PUBLIC
void appCoordinator_cleanup() {
    FormType *formP;
    switch (currentActivity) {
    case STARTSCREEN:
        startscreen_cleanup();
        break;
    case GAME:
        game_end(true);
        break;
    }

    formP = FrmGetActiveForm();
    if (formP != NULL) {
        FrmDeleteForm(formP);
    }
}

void appCoordinator_startEventDispatcher(Activity activity) {
    EventType event;
    UInt16 err;

    if (game_restore()) {
        currentActivity = GAME;
    } else {
        appCoordinator_launchStartScreen();
        FrmCustomAlert(MAINMENU_ALERT_LAUNCHWARNING, NULL, NULL, NULL);
    }

    do {
        EvtGetEvent(&event, appCoordinator_eventDelayTime(currentActivity));
        if (!SysHandleEvent(&event)) {
            if (!MenuHandleEvent(NULL, &event, &err)) {
                if (!appCoordinator_handle(currentActivity, &event)) {
                    FrmDispatchEvent(&event);
                }
            }
        }

    } while (event.eType != appStopEvent);
}

// CALLBACKS

static void appCoordinator_launchGame(LocalID dbId, GameMode gameMode) {
    FormType *formP = FrmGetActiveForm();
    game_setup(dbId, gameMode);
    currentActivity = GAME;
    if (formP != NULL) {
        FrmDeleteForm(formP);
    }
}

static void appCoordinator_launchStartScreen() {
    FormType *formP = FrmGetActiveForm();
    currentActivity = STARTSCREEN;
    startscreen_setup();
    if (formP != NULL) {
        FrmDeleteForm(formP);
    }
}

// STATIC

static Boolean appCoordinator_handle(Activity activity, EventPtr eventptr) {
    switch (activity) {
    case GAME:
        return game_mainLoop(eventptr, appCoordinator_launchStartScreen);
    case STARTSCREEN:
        return startscreen_eventHandler(eventptr, appCoordinator_launchGame);
    default:
        break;
    }
    return false;
}

static int appCoordinator_eventDelayTime(Activity activity) {
    switch (activity) {
    case GAME:
        return game_eventDelayTime();
    case STARTSCREEN:
        return 0;
    }
}