/*
 * $Id$
 */

#ifndef DNGVIEW_H
#define DNGVIEW_H

typedef enum {
    DNGGRAPHIC_NONE,
    DNGGRAPHIC_WALL,
    DNGGRAPHIC_LADDER,
    DNGGRAPHIC_DOOR
} DungeonGraphicType;

unsigned char dungeonViewGetVisibleTile(int fwd, int side);
DungeonGraphicType dungeonViewTileToGraphic(unsigned char tile);

#endif
