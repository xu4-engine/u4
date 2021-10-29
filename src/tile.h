/*
 * tile.h
 */

#ifndef TILE_H
#define TILE_H

#include "direction.h"
#include "types.h"

class Config;
class Image;
class TileAnim;

#define SYM_UP_LADDER       Tile::sym.dungeonTiles[1]
#define SYM_DOWN_LADDER     Tile::sym.dungeonTiles[2]
#define SYM_UP_DOWN_LADDER  Tile::sym.dungeonTiles[3]
#define SYM_MAGIC_ORB       Tile::sym.dungeonTiles[7]
#define SYM_POISON_FIELD    Tile::sym.fields[0]
#define SYM_ENERGY_FIELD    Tile::sym.fields[1]
#define SYM_FIRE_FIELD      Tile::sym.fields[2]
#define SYM_SLEEP_FIELD     Tile::sym.fields[3]

/* attr masks */
#define MASK_SHIP                   0x0001
#define MASK_HORSE                  0x0002
#define MASK_BALLOON                0x0004
#define MASK_DISPEL                 0x0008
#define MASK_TALKOVER               0x0010
#define MASK_DOOR                   0x0020
#define MASK_LOCKEDDOOR             0x0040
#define MASK_CHEST                  0x0080
#define MASK_ATTACKOVER             0x0100
#define MASK_CANLANDBALLOON         0x0200
#define MASK_REPLACEMENT            0x0400
#define MASK_WATER_REPLACEMENT      0x0800
#define MASK_FOREGROUND             0x1000
#define MASK_LIVING_THING           0x2000


/* movement masks */
#define MASK_SWIMABLE           0x0001
#define MASK_SAILABLE           0x0002
#define MASK_UNFLYABLE          0x0004
#define MASK_CREATURE_UNWALKABLE 0x0008

/**
 * TileRule struct
 */
struct TileRule {
    Symbol name;
    uint16_t mask;
    uint16_t movementMask;
    uint16_t speed;         // TileSpeed
    uint16_t effect;        // TileEffect
    uint16_t walkonDirs;
    uint16_t walkoffDirs;
};

struct TileSymbols {
    Symbol brickFloor;
    Symbol dungeonFloor;
    Symbol avatar;
    Symbol black;
    Symbol beggar;
    Symbol bridge;
    Symbol chest;
    Symbol corpse;
    Symbol door;
    Symbol guard;
    Symbol grass;
    Symbol horse;
    Symbol balloon;
    Symbol ship;
    Symbol pirateShip;
    Symbol wisp;
    Symbol moongate;
    Symbol whirlpool;
    Symbol lockelake;
    Symbol hitFlash;
    Symbol missFlash;
    Symbol magicFlash;
    Symbol classTiles[8];
    Symbol dungeonTiles[17];
    Symbol fields[5];
    Symbol dungeonMaps[6];
    Symbol combatMaps[21];
};

/**
 * A Tile object represents a specific tile type.  Every tile is a
 * member of a Tileset.
 */
class Tile {
public:
    static TileSymbols sym;

    static void initSymbols(Config*);
    static bool canTalkOverTile(const Tile *tile)   {return tile->canTalkOver() != 0;}
    static bool canAttackOverTile(const Tile *tile) {return tile->canAttackOver() != 0;}


    void setDirections(const char* dirs);
    const char* nameStr() const;

    TileId getId() const                {return id;}
    int getWidth() const                {return w;}
    int getHeight() const               {return h;}
    int getFrames() const               {return frames;}
    int getScale() const                {return scale;}
    TileAnim *getAnim() const           {return anim;}
    const Image *getImage() const       {return image;}

    bool isTiledInDungeon() const       {return tiledInDungeon;}
    bool isLandForeground() const       {return foreground;}
    bool isWaterForeground() const      {return waterForeground;}

    int canWalkOn(Direction d) const   {return DIR_IN_MASK(d, rule->walkonDirs);}
    int canWalkOff(Direction d) const  { return DIR_IN_MASK(d, rule->walkoffDirs); }

    /**
     * All tiles that you can walk, swim, or sail on, can be attacked over. All others must declare
     * themselves
     */
    int  canAttackOver() const      {return isWalkable() || isSwimable() || isSailable() || (rule->mask & MASK_ATTACKOVER); }
    int  canLandBalloon() const     {return rule->mask & MASK_CANLANDBALLOON; }
    int  isLivingObject() const     {return rule->mask & MASK_LIVING_THING; }
    int  isReplacement() const      {return rule->mask & MASK_REPLACEMENT; }
    int  isWaterReplacement() const {return rule->mask & MASK_WATER_REPLACEMENT; }

    int  isWalkable() const         {return rule->walkonDirs > 0; }
    bool isCreatureWalkable() const {return canWalkOn(DIR_ADVANCE) && !(rule->movementMask & MASK_CREATURE_UNWALKABLE);}
    bool isDungeonWalkable() const;
    bool isDungeonFloor() const;
    int  isSwimable() const         {return rule->movementMask & MASK_SWIMABLE;}
    int  isSailable() const         {return rule->movementMask & MASK_SAILABLE;}
    bool isWater() const            {return (isSwimable() || isSailable());}
    int  isFlyable() const          {return !(rule->movementMask & MASK_UNFLYABLE);}
    int  isDoor() const             {return rule->mask & MASK_DOOR;}
    int  isLockedDoor() const       {return rule->mask & MASK_LOCKEDDOOR;}
    int  isChest() const            {return rule->mask & MASK_CHEST;}
    int  isShip() const             {return rule->mask & MASK_SHIP;}
    bool isPirateShip() const       {return name == sym.pirateShip;}
    int  isHorse() const            {return rule->mask & MASK_HORSE;}
    int  isBalloon() const          {return rule->mask & MASK_BALLOON;}
    int  canDispel() const          {return rule->mask & MASK_DISPEL;}
    int  canTalkOver() const        {return rule->mask & MASK_TALKOVER;}
    TileSpeed getSpeed() const      {return (TileSpeed) rule->speed;}
    TileEffect getEffect() const    {return (TileEffect) rule->effect;}

    bool isOpaque() const;
    bool isForeground() const;
    Direction directionForFrame(int frame) const;
    int frameForDirection(Direction d) const;

    void loadImage();
    void deleteImage();
    uint16_t startFrameAnim() const;

    TileId id;          /**< an id that is unique across all tilesets */
    Symbol name;        /**< The name of this tile */
    Symbol imageName;   /**< The name of the image that belongs to this tile */
    Symbol animationRule;
    int16_t w;          /**< Pixel width of the tile */
    int16_t h;          /**< Pixel height of the tile */
    int16_t frames;     /**< The number of frames this tile has */
    int16_t scale;      /**< The scale of the tile */
    uint8_t opaque;     /**< Visibility blocking shape (1=square, 2=round) */

    bool foreground;    /**< As a maptile, is a foreground that will search neighbour maptiles for a land-based background replacement. ex: chests */
    bool waterForeground;/**< As a maptile, is a foreground that will search neighbour maptiles for a water-based background replacement. ex: chests */
    bool tiledInDungeon;

    const TileRule *rule; /**< The rules that govern the behavior of this tile */
    Image *image;       /**< The original image for this tile (with all of its frames) */
    TileAnim *anim;     /**< The tile animation for this tile */
    uint8_t directionCount;
    uint8_t directions[7];  /**< Directions used = frames (if present) */
};

struct TileRenderData {
    VisualId vid;       /**< The default rendering resource identifier */
    VisualId scroll;    /**< The scrolling rendering resource identifier */
    int16_t  animType;  /**< TileAnimType */
};

#endif
