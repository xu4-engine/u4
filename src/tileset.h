/*
 * $Id$
 */

#ifndef TILESET_H
#define TILESET_H

#include "u4file.h"
#include "tile.h"

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

/* FIXME: to be done (something like this)
typedef struct _TilesetGraphic {
    const char *filename;
    int tileWidth;
    int tileHeight;
    int bpp;
    int indexed;
    VideoType mode;
    struct _Image *graphic;
} TilesetGraphic;
*/

typedef struct _Tileset {    
    TilesetType type;
    CompressionType compType;    
    int numTiles;
    int totalFrames;
    int tileWidth;
    int tileHeight;    
    int bpp;
    int indexed;
    Tile *tiles;
    struct _Image *tileGraphic;
    /* FIXME: to be implemented instead of above (using TilesetGraphic above for data)
    ListNode *graphics;
    */
} Tileset;

void tilesetLoadAllTilesetsFromXml(const char *tilesetFilename);
void tilesetDeleteAllTilesets();
Tileset *tilesetGetByType(TilesetType type);
TilesetType tilesetGetTypeByStr(const char *type);
TileRule *tilesetFindRuleByName(const char *name);

#endif
