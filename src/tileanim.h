/*
 * $Id$
 */

#ifndef TILEANIM_H
#define TILEANIM_H

#include <string>
#include <map>
#include <vector>

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

    virtual int draw(Tile *tile, MapTile *mapTile, int scale, int x, int y) = 0;
    virtual ~TileAnimTransform() {}

    bool isRandom() const { return random; }

private:
    bool random;
};

/**
 * A tile animation transformation that turns a piece of the tile
 * upside down.  Used for animating the flags on building and ships.
 */
class TileAnimInvertTransform : public TileAnimTransform {
public:
    TileAnimInvertTransform(int x, int y, int w, int h);
    virtual int draw(Tile *tile, MapTile *mapTile, int scale, int x, int y);
    
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
    virtual int draw(Tile *tile, MapTile *mapTile, int scale, int x, int y);

    int x, y;
    std::vector<RGBA *> colors;
};

/**
 * A tile animation transformation that scrolls the tile's contents
 * vertically within the tile's boundaries.
 */ 
class TileAnimScrollTransform : public TileAnimTransform {
public:    
    virtual int draw(Tile *tile, MapTile *mapTile, int scale, int x, int y);    
};

/**
 * A tile animation transformation that advances the tile's frame
 * by 1.
 */ 
class TileAnimFrameTransform : public TileAnimTransform {
public:
    virtual int draw(Tile *tile, MapTile *mapTile, int scale, int x, int y);
};

/**
 * A tile animation transformation that changes pixels with colors
 * that fall in a given range to another color.  Used to animate
 * the campfire in VGA mode.
 */ 
class TileAnimPixelColorTransform : public TileAnimTransform {
public:
    TileAnimPixelColorTransform(int x, int y, int w, int h);
    virtual int draw(Tile *tile, MapTile *mapTile, int scale, int x, int y);

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
        FRAME        
    } Type;

    static TileAnimContext* create(const ConfigElement &config);
    
    void add(TileAnimTransform*);
    virtual bool isInContext(Tile *t, MapTile *mapTile);
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
    virtual bool isInContext(Tile *t, MapTile *mapTile);

private:
    int frame;
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
    int draw(Tile *tile, MapTile *mapTile, int scale, int x, int y); 
    bool isControlling() const;
    bool isRandom() const;

private:
    bool controls; /* true if the tile animation in is charge of drawing the tile itself */
    bool random;   /* true if the tile animation occurs randomely (50% of the time) */
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
