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
class RGBA;

class  TileAnimTransform {
public:
    virtual void draw(Image *tiles, int tile, int scale, int x, int y) = 0;
    virtual ~TileAnimTransform() {}
};

class TileAnimInvertTransform : public TileAnimTransform {
public:
    TileAnimInvertTransform(int x, int y, int w, int h);
    virtual void draw(Image *tiles, int tile, int scale, int x, int y);
    
private:
    int x, y, w, h;
};

class TileAnimPixelTransform : public TileAnimTransform {
public:
    TileAnimPixelTransform(int x, int y);
    virtual void draw(Image *tiles, int tile, int scale, int x, int y);

    int x, y;
    std::vector<RGBA *> colors;
};

class TileAnim {
public:
    std::string name;
    std::vector<TileAnimTransform *> transforms;

    void draw(Image *tiles, int tile, int scale, int x, int y);
};

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
