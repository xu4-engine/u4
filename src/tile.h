/*
 * $Id$
 */

#ifndef TILE_H
#define TILE_H

#include <string>
#include <vector>
#include "direction.h"
#include "types.h"

using std::string;

class TileRule;

typedef std::vector<class Tile *> TileVector;

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
    static Tile *findByName(string name);
    static void loadProperties(Tile *tile, void *xmlNode);    
    static MapTile translate(int index, string tileMap = "base");
    static unsigned int getIndex(TileId id);

    string name;    
    int index;
    int frames;
    bool animated; /* FIXME: this will be changed to 'animation' of type TileAnimationStyle */
    bool opaque;
    TileRule *rule;
};

#endif
