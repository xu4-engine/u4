/*
 * $Id$
 */

#ifndef DNGVIEW_H
#define DNGVIEW_H

#include "types.h"
#include "location.h"

typedef enum {
    DNGGRAPHIC_NONE,
    DNGGRAPHIC_WALL,
    DNGGRAPHIC_LADDERUP,
    DNGGRAPHIC_LADDERDOWN,
    DNGGRAPHIC_LADDERUPDOWN,
    DNGGRAPHIC_DOOR,
    DNGGRAPHIC_DNGTILE,
    DNGGRAPHIC_BASETILE
} DungeonGraphicType;

MapTileList dungeonViewGetTiles(int fwd, int side);
DungeonGraphicType dungeonViewTilesToGraphic(MapTileList tiles);

#endif
