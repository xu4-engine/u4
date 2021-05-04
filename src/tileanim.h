/*
 * tileanim.h
 */

#ifndef TILEANIM_H
#define TILEANIM_H

#include <string>
#include <map>
#include <vector>

#include "direction.h"

class Image;
class Tile;
struct RGBA;

enum TileAnimType {
    ATYPE_INVERT,
    // Turn part of a tile upside down. Used for flags on buildings and ships.

    ATYPE_SCROLL,
    // Scroll the tile's contents vertically within the tile's boundaries.

    ATYPE_FRAME,
    // A transformation that advances the tile's frame by 1.

#if 0
    ATYPE_PIXEL,
    // Change single pixels to a random color selected from a list.
    // Used for animating the campfire in EGA mode.
#endif

    ATYPE_PIXEL_COLOR,
    // Changes pixels with colors that fall in a given range to another color.
    // Used to animate the campfire in VGA mode.

    ATYPE_COUNT
};

/**
 * Properties for tile animation transformations.
 */
struct TileAnimTransform {
    uint8_t animType;   // TileAnimType
    uint8_t random;
    union {
        struct {
            int16_t x, y, w, h;
        } invert;
        struct {
            int16_t increment, current, lastOffset;
        } scroll;
        struct {
            int16_t current;
        } frame;
#if 0
        struct {
            int16_t x, y;
            RGBA color[?];
        } pixel;
#endif
        struct {
            int16_t x, y, w, h;
            RGBA start;
            RGBA end;
        } pcolor;
    } var;

    void draw(Image* dest, const Tile* tile, const MapTile& mapTile);
};

/**
 * A context in which to perform the animation (controls if transforms are
 * performed or not).
 */
class TileAnimContext {
public:
    typedef std::vector<TileAnimTransform *> TileAnimTransformList;

    virtual ~TileAnimContext();
    virtual bool isInContext(const Tile *t, const MapTile &mapTile, Direction d) = 0;

    TileAnimTransformList transforms;
};

/**
 * An animation context which changes the animation based on the tile's current frame
 */
class TileAnimFrameContext : public TileAnimContext {
public:
    TileAnimFrameContext(int frame);
    virtual bool isInContext(const Tile *t, const MapTile &mapTile, Direction d);

private:
    int frame;
};

/**
 * An animation context which changes the animation based on the player's current facing direction
 */
class TileAnimPlayerDirContext : public TileAnimContext {
public:
    TileAnimPlayerDirContext(Direction dir);
    virtual bool isInContext(const Tile *t, const MapTile &mapTile, Direction d);

private:
    Direction dir;
};

/**
 * Instructions for animating a tile.  Each tile animation is made up
 * of a list of transformations which are applied to the tile after it
 * is drawn.
 */
class TileAnim {
public:
    TileAnim() : random(0) {}
    ~TileAnim();

    /* returns the frame to set the mapTile to (only relevent if persistent) */
    void draw(Image *dest, const Tile *tile, const MapTile &mapTile, Direction dir);

    std::string name;
    std::vector<TileAnimTransform *> transforms;
    std::vector<TileAnimContext *> contexts;
    int random;   /* true if the tile animation occurs randomely */
};

/**
 * A set of tile animations.  Tile animations are associated with a
 * specific image set which shares the same name.
 */
class TileAnimSet {
public:
    typedef std::map<std::string, TileAnim *> TileAnimMap;

    ~TileAnimSet();
    TileAnim *getByName(const std::string &name);

    std::string name;
    TileAnimMap tileanims;
};

#endif
