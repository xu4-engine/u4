/*
 * $Id$
 */

#ifndef TILE_H
#define TILE_H

#include <string>
#include <vector>
#include "direction.h"
#include "image.h"
#include "tileanim.h"
#include "types.h"

using std::string;

class Tileset;
class TileRule;

/* attr masks */
#define MASK_SHIP               0x0001
#define MASK_HORSE              0x0002
#define MASK_BALLOON            0x0004
#define MASK_DISPEL             0x0008
#define MASK_TALKOVER           0x0010
#define MASK_DOOR               0x0020
#define MASK_LOCKEDDOOR         0x0040
#define MASK_CHEST              0x0080
#define MASK_ATTACKOVER         0x0100
#define MASK_CANLANDBALLOON     0x0200
#define MASK_REPLACEMENT        0x0400

/* movement masks */
#define MASK_SWIMABLE           0x0001
#define MASK_SAILABLE           0x0002
#define MASK_UNFLYABLE          0x0004
#define MASK_CREATURE_UNWALKABLE 0x0008

/**
 * Tile class
 */
class Tile : public MapTile {
public:
    Tile() : w(0), h(0), index(0), frames(0), anim(NULL), opaque(false), rule(NULL), image(NULL),
        tileset(NULL) {}

    static void loadProperties(Tile *tile, void *xmlNode);    
    static MapTile translate(int index, string tileMap = "base");
    static unsigned int getIndex(TileId id);

	void draw(int x, int y, int frame, bool focused = false);	
	void drawFocus(int x, int y) const;
    Image *getImage();

    string name;        /* The name of this tile */
    int w, h;           /* width and height of the tile */
    int index;          /* The physical tile index of this tile on its parent image (the whole tileset image) */
    int frames;         /* The number of frames this tile has */
    TileAnim *anim;     /* The tile animation for this tile */
    //TileAnimationStyle animation;   /* The animation style of this tile */
    bool opaque;        /* Is this tile opaque? */
    TileRule *rule;     /* The rules that govern the behavior of this tile */
	Image *image;       /* The actual image for this tile (with all of its frames) */
    Tileset *tileset;   /* The tileset this tile belongs to */
    string looks_like;  /* The name of the tile that this tile looks exactly like (if any) */    
};

#endif
