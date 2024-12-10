/// Responsible for the navigation throughout the app, switching between menus and game

#ifndef APPCOORDINATOR_H_
#define APPCOORDINATOR_H_

typedef enum {
    GAME,
    STARTSCREEN
} Activity;

void appCoordinator_startEventDispatcher(Activity activity);

#endif
