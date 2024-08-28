#include "database.h"
#include "game/level.h"
#include <PalmOS.h>
#include "constants.h"

#define DB_TYPE 'DATA'
#define DB_PREFIX "MiniGolf "

void database_levelPackDatabasesList(char dbNames[MAX_DB_COUNT][DB_NAME_LEN], LocalID *dbIds, UInt16 *dbCount) {
    UInt16 cardNo = 0;
    LocalID dbID = 0;
    DmSearchStateType searchState;
    char dbName[DB_NAME_LEN];

    *dbCount = 0;

    DmGetNextDatabaseByTypeCreator(true, &searchState, 0, APP_CREATOR_ID, false, &cardNo, &dbID);
    while (dbID != 0) {
        if (DmDatabaseInfo(cardNo, dbID, dbName, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL) == errNone) {
            if (StrNCompare(dbName, DB_PREFIX, StrLen(DB_PREFIX)) == 0 && *dbCount < MAX_DB_COUNT) {
                int i;
                int nameLength = StrLen(dbName);
                int prefixLength = StrLen(DB_PREFIX);
                for (i = 0; i < nameLength - prefixLength; i++) {
                    dbName[i] = dbName[i + prefixLength];
                }
                dbName[nameLength - prefixLength] = '\0';
                StrCopy(dbNames[*dbCount], dbName);
                dbIds[*dbCount] = dbID;
                (*dbCount)++;
            }
        }
        DmGetNextDatabaseByTypeCreator(false, &searchState, 0, APP_CREATOR_ID, false, &cardNo, &dbID);
    }
}

Boolean database_levelPackExists(LocalID dbId) {
    int i;
    for (i = 0; i < DmNumDatabases(0); i++) {
        if (DmGetDatabase(0, i) == dbId) {
            return true;
        }
    }
    return false;
}

LocalID database_create(char *dbName) {
    char finalName[40] = DB_PREFIX;
    LocalID dbId;
    DmOpenRef dbRef;
    Err error;

    StrCat(finalName, dbName);

    dbId = DmFindDatabase(0, finalName);
    if (dbId > 0) {
        DmDeleteDatabase(0, dbId);
    }

    error = DmCreateDatabase(0, finalName, APP_CREATOR_ID, DB_TYPE, false);
    if (error) {
        return 0;
    }

    dbId = DmFindDatabase(0, finalName);
    dbRef = DmOpenDatabase(0, dbId, dmModeReadWrite);
    if (!dbRef) {
        return 0;
    }
    DmCloseDatabase(dbRef);
    return dbId;
}

int database_levelCount(LocalID dbId) {
    UInt16 numRecords = 0;
    DmOpenRef dbRef;
    dbRef = DmOpenDatabase(0, dbId, dmModeReadOnly);
    if (!dbRef) {
        return 0;
    }
    numRecords = DmNumRecords(dbRef);
    DmCloseDatabase(dbRef);
    return numRecords;
}

void database_deleteRecord(UInt16 index, LocalID dbId) {
    DmOpenRef dbRef = DmOpenDatabase(0, dbId, dmModeReadWrite);
    if (!dbRef) {
        return;
    }

    if (DmQueryRecord(dbRef, index)) {
        DmDeleteRecord(dbRef, index);
    }

    DmCloseDatabase(dbRef);
}

void database_moveRecord(UInt16 oldIndex, UInt16 newIndex, Boolean overwrite, LocalID dbId) {
    MemHandle oldRecHandle, newRecHandle;
    UInt32 oldRecSize;
    void *oldRecPtr, *newRecPtr;

    DmOpenRef dbRef;

    dbRef = DmOpenDatabase(0, dbId, dmModeReadWrite);
    if (!dbRef) {
        return;
    }

    oldRecHandle = DmQueryRecord(dbRef, oldIndex);

    if (!oldRecHandle)
        return;
    oldRecPtr = MemHandleLock(oldRecHandle);
    oldRecSize = MemHandleSize(oldRecHandle);

    if (overwrite && DmQueryRecord(dbRef, newIndex)) {
        DmRemoveRecord(dbRef, newIndex);
    }

    newRecHandle = DmNewRecord(dbRef, &newIndex, oldRecSize);
    if (!newRecHandle) {
        MemHandleUnlock(oldRecHandle);
        return;
    }
    newRecPtr = MemHandleLock(newRecHandle);

    DmWrite(newRecPtr, 0, oldRecPtr, oldRecSize);

    MemHandleUnlock(newRecHandle);
    MemHandleUnlock(oldRecHandle);

    if (DmQueryRecord(dbRef, oldIndex)) {
        DmRemoveRecord(dbRef, oldIndex);
    }

    DmCloseDatabase(dbRef);
}

void database_readPackedLevel(Level *level, UInt16 index, LocalID dbId) {
    int i;
    DmOpenRef dbRef;
    MemHandle recordHandle;
    LevelPacked *packed;
    UInt16 wallCount = 0;
    UInt16 levelItemCount = 0;

    dbRef = DmOpenDatabase(0, dbId, dmModeReadOnly);
    if (!dbRef) {
        return;
    }

    recordHandle = DmQueryRecord(dbRef, index);
    if (!recordHandle) {
        DmCloseDatabase(dbRef);
        return;
    }

    packed = MemHandleLock(recordHandle);
    if (!packed) {
        DmCloseDatabase(dbRef);
        return;
    }

    for (i = 0; i < packed->levelItemCount; i++) {
        if (packed->levelItems[i].itemType != WALL) {
            break;
        }
        wallCount++;
    }

    MemMove(&level->wallCount, &wallCount, sizeof(UInt16));
    levelItemCount = packed->levelItemCount - wallCount;
    MemMove(&level->levelItemCount, &levelItemCount, sizeof(UInt16));

    level->walls = (Line *)MemPtrNew(sizeof(Line) * level->wallCount);
    for (i = 0; i < wallCount; i++) {
        Line line;
        line.startpoint = packed->levelItems[i].position;
        line.endpoint = packed->levelItems[i].positionB;
        MemMove(&level->walls[i], &line, sizeof(Line));
        //level->walls[i] = packed->walls[i];
    }

    level->levelItems = (LevelItem *)MemPtrNew(sizeof(LevelItem) * levelItemCount);
    for (i = wallCount; i < wallCount + levelItemCount; i++) {
        MemMove(&level->levelItems[i - wallCount], &packed->levelItems[i], sizeof(LevelItem));
    }

    //level->walls[i] = packed->walls[i];

    MemHandleUnlock(recordHandle);

    DmCloseDatabase(dbRef);
}

void database_writePackedLevel(Level *level, LocalID dbId) {
    int i;
    UInt32 totalOffset;
    DmOpenRef dbRef;
    UInt16 recordIndex;
    MemHandle recordHandle;
    LevelPacked *packed;
    Err err;
    UInt32 recordSize;
    UInt32 levelItemsOffset;
    UInt16 totalItemsCount = level->levelItemCount + level->wallCount;

    dbRef = DmOpenDatabase(0, dbId, dmModeReadWrite);
    recordSize = sizeof(LevelPacked) + ((level->levelItemCount - 1) * sizeof(LevelItem)) + ((level->wallCount) * sizeof(LevelItem));

    recordHandle = DmNewHandle(dbRef, recordSize);
    if (recordHandle == NULL) {
        return;
    }

    packed = (LevelPacked *)MemHandleLock(recordHandle);

    DmWrite(packed, OffsetOf(LevelPacked, levelItemCount), &totalItemsCount, sizeof(UInt16));

    totalOffset = 0;
    levelItemsOffset = OffsetOf(LevelPacked, levelItems);

    for (i = 0; i < level->wallCount; i++) {
        LevelItem levelItem;
        levelItem.itemType = WALL;
        levelItem.position = level->walls[i].startpoint;
        levelItem.positionB = level->walls[i].endpoint;
        DmWrite(packed, levelItemsOffset + totalOffset, &levelItem, sizeof(LevelItem));
        totalOffset += sizeof(LevelItem);
    }

    for (i = 0; i < level->levelItemCount; i++) {
        DmWrite(packed, levelItemsOffset + totalOffset, &level->levelItems[i], sizeof(LevelItem));
        totalOffset += sizeof(LevelItem);
    }

    MemHandleUnlock(recordHandle);

    recordIndex = dmMaxRecordIndex;

    err = DmAttachRecord(dbRef, &recordIndex, recordHandle, NULL);
    if (err != errNone) {
        return;
    }

    DmCloseDatabase(dbRef);
}
