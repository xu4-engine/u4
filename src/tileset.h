/*
 * $Id$
 */

#ifndef TILESET_H
#define TILESET_H

#include "u4file.h"
#include "tile.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TILESET_BASE,
    TILESET_DUNGEON,
    TILESET_GEM,
    TILESET_MAX
} TilesetType;

typedef struct _TileRule {
    const char *name;
    unsigned short mask;    
    unsigned short movementMask;
    TileSpeed speed;
    TileEffect effect;
    int walkonDirs;
    int walkoffDirs;
} TileRule;

typedef struct _Tileset {    
    TilesetType type;
    CompressionType compType;    
    int numTiles;
    Tile *tiles;
    int totalFrames;
    int imageId;
    struct _Image *tileGraphic;
} Tileset;

void tilesetLoadAllTilesetsFromXml(const char *tilesetFilename);
void tilesetDeleteAllTilesets();
Tileset *tilesetGetByType(TilesetType type);
TilesetType tilesetGetTypeByStr(const char *type);
TileRule *tilesetFindRuleByName(const char *name);

#ifdef __cplusplus
}
#endif

#endif
