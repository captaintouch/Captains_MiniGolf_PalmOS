#include "level.h"
#include "../database.h"
#include "../resources.h"
#include "debugWindow.h"
#include "drawhelper.h"
#include "models.h"
#include <PalmOS.h>
#include "../constants.h"

LevelItem levelItem(LevelItemType itemType, Coordinate position) {
    LevelItem levelItem;
    levelItem.itemType = itemType;
    levelItem.position = position;
    return levelItem;
}

int level_itemTypeDimension(LevelItemType itemType) {
    switch (itemType) {
    case WINDMILL:
        return RESOURCE_GFX_WINDMILLDIMENSION;
    case CHICKEN:
        return RESOURCE_GFX_CHICKENDIMENSION;
    case UFO:
        return RESOURCE_GFX_UFODIMENSION;
    case HOLE:
        return RESOURCE_GFX_HOLEDIMENSION;
    case BALL:
        return RESOURCE_GFX_BALLDIMENSION;
    case GRASS:
        return RESOURCE_GFX_GRASSDIMENSION;
    case WALL:
        return 5;
    }
}



void level_removeWall(Level *level, int index) {
    int i;
    for (i = index; i < level->wallCount - 1; i++) {
        level->walls[i] = level->walls[i + 1];
    }
    level->wallCount -= 1;
    MemPtrResize(level->walls, sizeof(Line) * level->wallCount);
}

void level_removeItem(Level *level, int index) {
    int i;
    if (level->levelItems[index].itemType == HOLE || level->levelItems[index].itemType == BALL) {
        // Can't delete the hole nor the ball
        return;
    }

    for (i = index; i < level->levelItemCount - 1; i++) {
        level->levelItems[i] = level->levelItems[i + 1];
    }
    level->levelItemCount -= 1;
    MemPtrResize(level->levelItems, sizeof(LevelItem) * level->levelItemCount);
}

static Coordinate level_size(Line *walls, int wallCount) {
    int i, maxX = 0, maxY = 0;
    for (i = 0; i < wallCount; i++) {
        if (walls[i].endpoint.x > maxX)
            maxX = walls[i].endpoint.x;
        if (walls[i].startpoint.x > maxX)
            maxX = walls[i].startpoint.x;
        if (walls[i].endpoint.y > maxY)
            maxY = walls[i].endpoint.y;
        if (walls[i].startpoint.y > maxY)
            maxY = walls[i].startpoint.y;
    }
    return coordinate(maxX, maxY);
}

void level_updateStatistics(Level *level) {
    level->levelSize = level_size(level->walls, level->wallCount);
}

static Level level_create(Line *walls, int wallCount, LevelItem *levelItems, int levelItemCount) {
    Level level;
    level.wallCount = wallCount;
    level.walls = walls;
    level.levelItems = levelItems;
    level.levelItemCount = levelItemCount;
    level_updateStatistics(&level);
    return level;
}

Level level_blankLevel() {
    int wallCount = 4;
    int levelItemCount = 2;
    Line *walls = (Line *)MemPtrNew(sizeof(Line) * wallCount);
    LevelItem *levelItems = (LevelItem *)MemPtrNew(sizeof(LevelItem) * levelItemCount);

    walls[0].startpoint.x = 5;
    walls[0].startpoint.y = 5;
    walls[0].endpoint.x = GAMEWINDOW_WIDTH - 5;
    walls[0].endpoint.y = 5;

    walls[1].startpoint.x = GAMEWINDOW_WIDTH - 5;
    walls[1].startpoint.y = 5;
    walls[1].endpoint.x = GAMEWINDOW_WIDTH - 5;
    walls[1].endpoint.y = GAMEWINDOW_HEIGHT - 5;

    walls[2].startpoint.x = GAMEWINDOW_WIDTH - 5;
    walls[2].startpoint.y = GAMEWINDOW_HEIGHT - 5;
    walls[2].endpoint.x = 5;
    walls[2].endpoint.y = GAMEWINDOW_HEIGHT - 5;

    walls[3].startpoint.x = 5;
    walls[3].startpoint.y = GAMEWINDOW_HEIGHT - 5;
    walls[3].endpoint.x = 5;
    walls[3].endpoint.y = 5;

    levelItems[0] = levelItem(HOLE, coordinate(20, 40));
    levelItems[1] = levelItem(BALL, coordinate(60, 40));

    return level_create(walls, wallCount, levelItems, levelItemCount);
}

#ifdef DEBUG
void level_createDebugLevelDatabase() {
    LocalID dbId;
    int wallCount, i, levelItemCount;
    int availableLevels = 2;
    Line *walls;
    LevelItem *levelItems;
    Level originalLevel;
    LevelItem hole;
    LevelItem ball;

    hole.itemType = HOLE;
    ball.itemType = BALL;
    dbId = database_create("Debug Levels");
    for (i = 0; i < availableLevels; i++) {
        switch (i) {
        case 0:
            wallCount = 5;
            walls = (Line *)MemPtrNew(sizeof(Line) * wallCount);
            MemSet(walls, sizeof(Line) * wallCount, 0);

            walls[0].startpoint.x = 0;
            walls[0].startpoint.y = 50;
            walls[0].endpoint.x = 200;
            walls[0].endpoint.y = 0;

            walls[1].startpoint.x = 0;
            walls[1].startpoint.y = 50;
            walls[1].endpoint.x = 0;
            walls[1].endpoint.y = 159 - 30;

            walls[2].startpoint.x = 0;
            walls[2].startpoint.y = 159 - 30;
            walls[2].endpoint.x = 200;
            walls[2].endpoint.y = 159 - 30;

            walls[3].startpoint.x = 200;
            walls[3].startpoint.y = 0;
            walls[3].endpoint.x = 200;
            walls[3].endpoint.y = 159 - 30;

            walls[4].startpoint.x = 60;
            walls[4].startpoint.y = 60;
            walls[4].endpoint.x = 60;
            walls[4].endpoint.y = 159 - 30;

            levelItemCount = 3;
            levelItems = (LevelItem *)MemPtrNew(sizeof(LevelItem) * levelItemCount);
            MemSet(levelItems, sizeof(LevelItem) * levelItemCount, 0);

            levelItems[0] = levelItem(HOLE, coordinate(110, 100));
            levelItems[1] = levelItem(BALL, coordinate(30, 100));
            levelItems[2] = levelItem(WINDMILL, coordinate(60, 60));
            break;
        case 1:
            wallCount = 9;
            walls = (Line *)MemPtrNew(sizeof(Line) * wallCount);
            MemSet(walls, sizeof(Line) * wallCount, 0);

            walls[0].startpoint.x = 60;
            walls[0].startpoint.y = 0;
            walls[0].endpoint.x = 159;
            walls[0].endpoint.y = 0;

            walls[1].startpoint.x = 0;
            walls[1].startpoint.y = 60;
            walls[1].endpoint.x = 30;
            walls[1].endpoint.y = 159 - 30;

            walls[2].startpoint.x = 30;
            walls[2].startpoint.y = 159 - 30;
            walls[2].endpoint.x = 159;
            walls[2].endpoint.y = 159 - 30;

            walls[3].startpoint.x = 159;
            walls[3].startpoint.y = 0;
            walls[3].endpoint.x = 159;
            walls[3].endpoint.y = 159 - 30;

            walls[4].startpoint.x = 60;
            walls[4].startpoint.y = 60;
            walls[4].endpoint.x = 90;
            walls[4].endpoint.y = 159 - 30;

            walls[5].startpoint.x = 60;
            walls[5].startpoint.y = 60;
            walls[5].endpoint.x = 125;
            walls[5].endpoint.y = 60;

            walls[6].startpoint.x = 140;
            walls[6].startpoint.y = 60;
            walls[6].endpoint.x = 159;
            walls[6].endpoint.y = 60;

            walls[7].startpoint.x = 60;
            walls[7].startpoint.y = 0;
            walls[7].endpoint.x = 60;
            walls[7].endpoint.y = 45;

            walls[8].startpoint.x = 0;
            walls[8].startpoint.y = 60;
            walls[8].endpoint.x = 60;
            walls[8].endpoint.y = 45;

            levelItemCount = 2;
            levelItems = (LevelItem *)MemPtrNew(sizeof(LevelItem) * levelItemCount);
            MemSet(levelItems, sizeof(LevelItem) * levelItemCount, 0);

            levelItems[0] = levelItem(HOLE, coordinate(55, 120));
            levelItems[1] = levelItem(BALL, coordinate(125, 120));
            break;
        default:
            break;
        }

        originalLevel.wallCount = wallCount;
        originalLevel.walls = walls;
        originalLevel.levelItemCount = levelItemCount;
        originalLevel.levelItems = levelItems;
        database_writePackedLevel(&originalLevel, dbId);
        MemPtrFree(walls);
        MemPtrFree(levelItems);
    }
}
#endif

Coordinate level_coordinateForItem(LevelItemType itemType, Level *level) {
    int i;
    for (i = 0; i < level->levelItemCount; i++) {
        if (level->levelItems[i].itemType == itemType) {
            return level->levelItems[i].position;
        }
    }
    return coordinate(0, 0);
}

static void level_addNewWall(Line wall, Level *level) {
    int i;
    Line *walls = (Line *)MemPtrNew(sizeof(Line) * (level->wallCount + 1));
    for (i = 0; i < level->wallCount; i++) {
        walls[i] = level->walls[i];
    }
    walls[level->wallCount] = wall;
    MemPtrFree(level->walls);
    level->walls = walls;
    level->wallCount = level->wallCount + 1;
}

void level_addNewItem(LevelItem item, Level *level) {
    int i;
    LevelItem *levelItems;
    if (item.itemType == WALL) {
        Line wall;
        wall.startpoint = item.position;
        wall.endpoint = item.positionB;
        level_addNewWall(wall, level);
        return;
    }
    levelItems = (LevelItem *)MemPtrNew(sizeof(LevelItem) * (level->levelItemCount + 1));
    for (i = 0; i < level->levelItemCount; i++) {
        levelItems[i] = level->levelItems[i];
    }
    levelItems[level->levelItemCount] = item;
    MemPtrFree(level->levelItems);
    level->levelItems = levelItems;
    level->levelItemCount = level->levelItemCount + 1;
}

Level level_load(int level, LocalID dbId) {
    int availableLevels = database_levelCount(dbId);
    Level targetLevel;
    level = level % availableLevels;
    database_readPackedLevel(&targetLevel, level, dbId);
    level_updateStatistics(&targetLevel);
    return targetLevel;
}

void level_unload(Level level) {
    if (level.walls != NULL) {
        MemPtrFree(level.walls);
        level.walls = NULL;
    }
    if (level.levelItems != NULL) {
        MemPtrFree(level.levelItems);
        level.levelItems = NULL;
    }
}