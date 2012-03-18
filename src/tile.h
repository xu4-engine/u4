/*
 * $Id$
 */

#ifndef TILE_H
#define TILE_H

#include <string>
#include <vector>

#include "direction.h"
#include "types.h"
#include "tileset.h"

using std::string;
using std::vector;

class ConfigElement;
class Image;
class Tileset;
class TileAnim;

/* attr masks */
#define MASK_SHIP               	0x0001
#define MASK_HORSE              	0x0002
#define MASK_BALLOON            	0x0004
#define MASK_DISPEL             	0x0008
#define MASK_TALKOVER           	0x0010
#define MASK_DOOR               	0x0020
#define MASK_LOCKEDDOOR         	0x0040
#define MASK_CHEST              	0x0080
#define MASK_ATTACKOVER         	0x0100
#define MASK_CANLANDBALLOON     	0x0200
#define MASK_REPLACEMENT        	0x0400
#define MASK_WATER_REPLACEMENT		0x0800
#define MASK_FOREGROUND         	0x1000
#define MASK_LIVING_THING			0x2000


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

    TileId getId() const                {return id;}
    const string &getName() const       {return name;}
    int getWidth() const                {return w;}
    int getHeight() const               {return h;}
    int getFrames() const               {return frames;}
    int getScale() const                {return scale;}
    TileAnim *getAnim() const           {return anim;}
    Image *getImage();
    const string &getLooksLike() const  {return looks_like;}

    bool isTiledInDungeon() const       {return tiledInDungeon;}
    bool isLandForeground() const       {return foreground;}
    bool isWaterForeground() const      {return waterForeground;}

    bool canWalkOn(Direction d) const   {return DIR_IN_MASK(d, rule->walkonDirs);}
    bool canWalkOff(Direction d) const  {return DIR_IN_MASK(d, rule->walkoffDirs);}

    /**
     * All tiles that you can walk, swim, or sail on, can be attacked over. All others must declare
     * themselves
     */
    bool canAttackOver() const {
        return isWalkable() || (rule->mask & (MASK_ATTACKOVER | MASK_SAILABLE | MASK_SWIMABLE));
    }

    bool canLandBalloon() const     {return rule->mask & MASK_CANLANDBALLOON;}
    bool isLivingObject() const     {return rule->mask & MASK_LIVING_THING;}
    bool isReplacement() const      {return rule->mask & MASK_REPLACEMENT;}
    bool isWaterReplacement() const {return rule->mask & MASK_WATER_REPLACEMENT;}

    bool isWalkable() const         {return rule->walkonDirs > 0;}
    bool isCreatureWalkable() const {return canWalkOn(DIR_ADVANCE) && !(rule->movementMask & MASK_CREATURE_UNWALKABLE);}
    bool isDungeonWalkable() const;
    bool isDungeonFloor() const;
    bool isSwimable() const         {return rule->movementMask & MASK_SWIMABLE;}
    bool isSailable() const         {return rule->movementMask & MASK_SAILABLE;}
    bool isWater() const            {return (isSwimable() || isSailable());}
    bool isFlyable() const          {return !(rule->movementMask & MASK_UNFLYABLE);}
    bool isDoor() const             {return rule->mask & MASK_DOOR;}
    bool isLockedDoor() const       {return rule->mask & MASK_LOCKEDDOOR;}
    bool isChest() const            {return rule->mask & MASK_CHEST;}
    bool isShip() const             {return rule->mask & MASK_SHIP;}
    bool isPirateShip() const       {return name == "pirate_ship";}
    bool isHorse() const            {return rule->mask & MASK_HORSE;}
    bool isBalloon() const          {return rule->mask & MASK_BALLOON;}
    bool canDispel() const          {return rule->mask & MASK_DISPEL;}
    bool canTalkOver() const        {return rule->mask & MASK_TALKOVER;}
    TileSpeed getSpeed() const      {return rule->speed;}
    TileEffect getEffect() const    {return rule->effect;}

    bool isOpaque() const;
    bool isForeground() const;
    Direction directionForFrame(int frame) const;
    int frameForDirection(Direction d) const;

    TileId id;          /**< an id that is unique across all tilesets */

    static void resetNextId()                       {nextId = 0;}
    static bool canTalkOverTile(const Tile *tile)   {return tile->canTalkOver();}
    static bool canAttackOverTile(const Tile *tile) {return tile->canAttackOver();}
    void deleteImage();

private:
    void loadImage();

public:
    string name;        /**< The name of this tile */
    Tileset *tileset;   /**< The tileset this tile belongs to */
    int w, h;           /**< width and height of the tile */
    int frames;         /**< The number of frames this tile has */
    int scale;          /**< The scale of the tile */
    TileAnim *anim;     /**< The tile animation for this tile */    
    bool opaque;        /**< Is this tile opaque? */

    bool foreground;    /**< As a maptile, is a foreground that will search neighbour maptiles for a land-based background replacement. ex: chests */
    bool waterForeground;/**< As a maptile, is a foreground that will search neighbour maptiles for a water-based background replacement. ex: chests */

    TileRule *rule;     /**< The rules that govern the behavior of this tile */
    string imageName;   /**< The name of the image that belongs to this tile */
    string looks_like;  /**< The name of the tile that this tile looks exactly like (if any) */    

    Image *image;       /**< The original image for this tile (with all of its frames) */
    bool tiledInDungeon;
    vector<Direction> directions;

    // disallow assignments, copy contruction
    Tile(const Tile&);
    const Tile &operator=(const Tile&);
    string animationRule;


    static TileId nextId;
};


#endif
