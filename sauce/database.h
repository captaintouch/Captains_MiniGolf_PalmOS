#ifndef DATABASE_H_
#define DATABASE_H_
#include "game/level.h"

#define MAX_DB_COUNT    30
#define DB_NAME_LEN     dmDBNameLength + 1

void database_writePackedLevel(Level *level, LocalID dbId);
void database_readPackedLevel(Level *level, UInt16 index, LocalID dbId);
void database_moveRecord(UInt16 oldIndex, UInt16 newIndex, Boolean overwrite, LocalID dbId);
LocalID database_create(char *dbName);
Boolean database_levelPackExists(LocalID dbId);
int database_levelCount(LocalID dbId);
void database_levelPackDatabasesList(char dbNames[MAX_DB_COUNT][DB_NAME_LEN], LocalID *dbIds, UInt16 *dbCount);
#endif