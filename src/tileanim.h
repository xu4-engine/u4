/*
 * $Id$
 */

#ifndef TILEANIM_H
#define TILEANIM_H

#include <string>
#include <map>
#include <vector>

#include "direction.h"
#include "xml.h"

class ConfigElement;
class Image;
class Tile;
struct RGBA;

/**
 * The interface for tile animation transformations.
 */
class  TileAnimTransform {
public:
    static TileAnimTransform *create(const ConfigElement &config);    
    static RGBA *loadColorFromConf(const ConfigElement &conf);
    
    virtual void draw(Tile *tile, MapTile *mapTile) = 0;
    virtual ~TileAnimTransform() {}
    virtual bool drawsTile() const = 0;
    
    // Properties
    int random;

private:    
    bool replaces;
};

/**
 * A tile animation transformation that turns a piece of the tile
 * upside down.  Used for animating the flags on building and ships.
 */
class TileAnimInvertTransform : public TileAnimTransform {
public:
    TileAnimInvertTransform(int x, int y, int w, int h);
    virtual void draw(Tile *tile, MapTile *mapTile);
    virtual bool drawsTile() const;
    
private:
    int x, y, w, h;
};

/**
 * A tile animation transformation that changes a single pixels to a
 * random color selected from a list.  Used for animating the
 * campfire in EGA mode.
 */
class TileAnimPixelTransform : public TileAnimTransform {
public:
    TileAnimPixelTransform(int x, int y);
    virtual void draw(Tile *tile, MapTile *mapTile);
    virtual bool drawsTile() const;

    int x, y;
    std::vector<RGBA *> colors;
};

/**
 * A tile animation transformation that scrolls the tile's contents
 * vertically within the tile's boundaries.
 */ 
class TileAnimScrollTransform : public TileAnimTransform {
public:    
    TileAnimScrollTransform(int increment);
    virtual void draw(Tile *tile, MapTile *mapTile);    
    virtual bool drawsTile() const;
private:
    int increment, current, lastOffset;
};

/**
 * A tile animation transformation that advances the tile's frame
 * by 1.
 */ 
class TileAnimFrameTransform : public TileAnimTransform {
public:
    virtual void draw(Tile *tile, MapTile *mapTile);
    virtual bool drawsTile() const;
};

/**
 * A tile animation transformation that changes pixels with colors
 * that fall in a given range to another color.  Used to animate
 * the campfire in VGA mode.
 */ 
class TileAnimPixelColorTransform : public TileAnimTransform {
public:
    TileAnimPixelColorTransform(int x, int y, int w, int h);
    virtual void draw(Tile *tile, MapTile *mapTile);
    virtual bool drawsTile() const;

    int x, y, w, h;
    RGBA *start, *end;
};

/**
 * A context in which to perform the animation
 */ 
class TileAnimContext {    
public:
    typedef std::vector<TileAnimTransform *> TileAnimTransformList;
    typedef enum {
        FRAME,
        DIR
    } Type;

    static TileAnimContext* create(const ConfigElement &config);
    
    void add(TileAnimTransform*);
    virtual bool isInContext(Tile *t, MapTile *mapTile, Direction d) = 0;
    TileAnimTransformList& getTransforms();
    virtual ~TileAnimContext() {}
private:    
    
    TileAnimTransformList animTransforms;
};

/**
 * An animation context which changes the animation based on the tile's current frame
 */ 
class TileAnimFrameContext : public TileAnimContext {
public:
    TileAnimFrameContext(int frame);    
    virtual bool isInContext(Tile *t, MapTile *mapTile, Direction d);

private:
    int frame;
};

/**
 * An animation context which changes the animation based on the player's current facing direction
 */ 
class TileAnimPlayerDirContext : public TileAnimContext {
public:
    TileAnimPlayerDirContext(Direction dir);
    virtual bool isInContext(Tile *t, MapTile *mapTile, Direction d);

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
    TileAnim(const ConfigElement &conf);

    std::string name;
    std::vector<TileAnimTransform *> transforms;
    std::vector<TileAnimContext *> contexts;

    /* returns the frame to set the mapTile to (only relevent if persistent) */
    void draw(Tile *tile, MapTile *mapTile, Direction dir);     

    int random;   /* true if the tile animation occurs randomely */
};

/**
 * A set of tile animations.  Tile animations are associated with a
 * specific image set which shares the same name.
 */
class TileAnimSet {
    typedef std::map<std::string, TileAnim *> TileAnimMap;

public:
    TileAnimSet(const ConfigElement &conf);

    TileAnim *getByName(const std::string &name);

    std::string name;
    TileAnimMap tileanims;
};

#endif
