#ifndef LEVEL_H_
#define LEVEL_H_
#include <PalmOS.h>
#include "models.h"
#include "drawhelper.h"

typedef enum LevelItemType {
    WALL,
    HOLE,
    BALL,
    WINDMILL,
    GRASS,
    UFO,
    CHICKEN
} LevelItemType;

typedef struct LevelItem {
    LevelItemType itemType;
    Coordinate position;
    Coordinate positionB;
} LevelItem;

typedef struct Level {
    UInt16 wallCount;
    UInt16 levelItemCount;
    LevelItem *levelItems;
    Line* walls;
    Coordinate levelSize;
} Level;

typedef struct LevelPacked {
    UInt16 levelItemCount;
    LevelItem levelItems[1];
} LevelPacked;

Level level_load(int level, LocalID dbId);
void level_loadSprites(Level *level);
void level_unload(Level level);
Coordinate level_coordinateForItem(LevelItemType itemType, Level *level);
LevelItem levelItem(LevelItemType itemType, Coordinate position);
void level_outerPolygon(Line *walls, int wallCount, Coordinate *polygonPoints, int *polyCount);
void level_addNewItem(LevelItem item, Level *level);
int level_itemTypeDimension(LevelItemType itemType);
Level level_blankLevel();
void level_removeWall(Level *level, int index);
void level_removeItem(Level *level, int index);
void level_updateStatistics(Level *level);

#ifdef DEBUG
void level_createDebugLevelDatabase();
#endif
#endif