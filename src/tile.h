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
using std::vector;

class ConfigElement;
class Image;
class Tileset;
class TileAnim;
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
#define MASK_FOREGROUND         0x0800

/* movement masks */
#define MASK_SWIMABLE           0x0001
#define MASK_SAILABLE           0x0002
#define MASK_UNFLYABLE          0x0004
#define MASK_CREATURE_UNWALKABLE 0x0008

/**
 * A Tile object represents a specific tile type.  Every tile is a
 * member of a Tileset.  
 */
class Tile {
public:
    Tile(Tileset *tileset);

    void loadProperties(const ConfigElement &conf);

    const string &getName() const { return name; }
    int getWidth() const { return w; }
    int getHeight() const { return h; }
    int getFrames() const { return frames; }
    int getScale() const { return scale; }
    TileAnim *getAnim() const { return anim; }
    Image *getImage();
    const string &getLooksLike() const { return looks_like; }
    bool isLarge() const;

    bool canWalkOn(Direction d) const;
    bool canWalkOff(Direction d) const;
    bool canAttackOver() const;
    bool canLandBalloon() const;
    bool isReplacement() const;
    bool isWalkable() const;
    bool isCreatureWalkable() const;
    bool isDungeonWalkable() const;
    bool isDungeonFloor() const;
    bool isSwimable() const;
    bool isSailable() const;
    bool isWater() const;
    bool isFlyable() const;
    bool isDoor() const;
    bool isLockedDoor() const;
    bool isChest() const;    
    bool isShip() const;    
    bool isPirateShip() const;
    bool isHorse() const;    
    bool isBalloon() const;    
    bool canDispel() const;
    bool canTalkOver() const;
    TileSpeed getSpeed() const;
    TileEffect getEffect() const;
    bool isOpaque() const;
    bool isForeground() const;
    Direction directionForFrame(int frame) const;
    int frameForDirection(Direction d) const;

    static void resetNextId() { nextId = 0; }
    static bool canTalkOverTile(const Tile *tile);
    static bool canAttackOverTile(const Tile *tile);

    TileId id;          /**< an id that is unique across all tilesets */
private:
    string name;        /**< The name of this tile */
    Tileset *tileset;   /**< The tileset this tile belongs to */
    int w, h;           /**< width and height of the tile */
    int frames;         /**< The number of frames this tile has */
    int scale;          /**< The scale of the tile */
    TileAnim *anim;     /**< The tile animation for this tile */    
    bool opaque;        /**< Is this tile opaque? */
    TileRule *rule;     /**< The rules that govern the behavior of this tile */
    string imageName;   /**< The name of the image that belongs to this tile */
    string looks_like;  /**< The name of the tile that this tile looks exactly like (if any) */    

    void loadImage();

    Image *image;       /**< The original image for this tile (with all of its frames) */
    bool large;
    vector<Direction> directions;

    // disallow assignments, copy contruction
    Tile(const Tile&);
    const Tile &operator=(const Tile&);

    static TileId nextId;
};

#endif
