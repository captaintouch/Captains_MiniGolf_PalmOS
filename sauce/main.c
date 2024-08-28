#include "appCoordinator.h"
#include <PalmOS.h>
#include "MathLib.h"
#include "appCoordinator.h"
#include "database.h"
#include "deviceinfo.h"
#include "game/colors.h"
#include "game/level.h"
#include <PalmOS.h>

static UInt32 applyScreenMode() {
    Int32 oldDepth = deviceinfo_currentDepth();
    Int32 depth = deviceinfo_maxDepth();
    if (depth < 4 || WinScreenMode(winScreenModeSet, NULL, NULL, &depth, NULL) != errNone) {
        ErrFatalDisplay("Unsupported device");
    }
    
    colors_setupReferenceColors(deviceinfo_colorSupported(), depth);
    return oldDepth;
}

static void checkHiResSupport() {
    #ifdef HIRESBUILD
    UInt32 version;
    if (FtrGet(sysFtrCreator, sysFtrNumWinVersion, &version) != errNone || version < 4) {
        ErrFatalDisplay("Device not compatible with hires version");
    }
    #endif
}

static void loadMathLib() {
    Err error;
    error = SysLibFind(MathLibName, &MathLibRef);
    if (error)
        error = SysLibLoad(LibType, MathLibCreator, &MathLibRef);
    ErrFatalDisplayIf(error, "Can't find MathLib");
    error = MathLibOpen(MathLibRef, MathLibVersion);
    ErrFatalDisplayIf(error, "Can't open MathLib");
}

static void closeMathLib() {
    UInt16 usecount;
    MathLibClose(MathLibRef, &usecount);
    if (usecount == 0)
        SysLibRemove(MathLibRef);
}

static UInt32 startApplication() {
    checkHiResSupport();
    loadMathLib();
    #ifdef DEBUG
    level_createDebugLevelDatabase();
    #endif
    return applyScreenMode();
}

static void endApplication(UInt32 oldDepth) {
    appCoordinator_cleanup();
    closeMathLib();
    WinScreenMode(winScreenModeSet, NULL, NULL, &oldDepth, NULL);
}

UInt32 PilotMain(UInt16 cmd, void *cmdPBP, UInt16 launchFlags) {
    UInt32 oldDepth;
    if (cmd == sysAppLaunchCmdNormalLaunch) {
        oldDepth = startApplication();
        appCoordinator_startEventDispatcher(STARTSCREEN);
        endApplication(oldDepth);
    }
    return 0;
}
