/*
 * $Id$
 */

#ifndef TILEANIM_H
#define TILEANIM_H

#include <string>
#include <map>
#include <vector>

#include "xml.h"

class Image;
struct RGBA;

/**
 * The interface for tile animation transformations.
 */
class  TileAnimTransform {
public:
    virtual void draw(Image *tiles, int tile, int scale, int x, int y) = 0;
    virtual ~TileAnimTransform() {}
};

/**
 * A tile animation transformation that turns a piece of the tile
 * upside down.  Used for animating the flags on building and ships.
 */
class TileAnimInvertTransform : public TileAnimTransform {
public:
    TileAnimInvertTransform(int x, int y, int w, int h);
    virtual void draw(Image *tiles, int tile, int scale, int x, int y);
    
private:
    int x, y, w, h;
};

/**
 * A tile animation transformation that changes a single pixels to a
 * random color selected from a list.  Used for animating the
 * campfire.
 */
class TileAnimPixelTransform : public TileAnimTransform {
public:
    TileAnimPixelTransform(int x, int y);
    virtual void draw(Image *tiles, int tile, int scale, int x, int y);

    int x, y;
    std::vector<RGBA *> colors;
};

/**
 * Instructions for animating a tile.  Each tile animation is made up
 * of a list of transformations which are applied to the tile after it
 * is drawn.
 */
class TileAnim {
public:
    std::string name;
    std::vector<TileAnimTransform *> transforms;

    void draw(Image *tiles, int tile, int scale, int x, int y);
};

/**
 * A set of tile animations.  Tile animations are associated with a
 * specific image set which shares the same name.
 */
struct TileAnimSet {
    std::string name;
    std::map<std::string, TileAnim *> tileanims;
};


TileAnimSet *tileAnimSetLoadFromXml(xmlNodePtr node);
TileAnim *tileAnimLoadFromXml(xmlNodePtr node);
TileAnimTransform *tileAnimTransformLoadFromXml(xmlNodePtr node);
TileAnim *tileAnimSetGetAnimByName(TileAnimSet *set, const std::string &name);
RGBA *tileAnimColorLoadFromXml(xmlNodePtr node);

#endif
