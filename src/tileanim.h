/*
 * $Id$
 */

#ifndef TILEANIM_H
#define TILEANIM_H

#include <string>
#include <map>
#include <vector>

#include "xml.h"

struct _Image;
struct _RGBA;

struct TileAnimPixelTransform {
    int x, y;
    std::vector<struct _RGBA *> colors;
};

struct TileAnimInvertTransform {
    int x, y, w, h;
};

struct  TileAnimTransform {
    enum TileAnimTransformType {
        TRANSFORM_INVERT,
        TRANSFORM_PIXEL
    } type;
    TileAnimInvertTransform invert;
    TileAnimPixelTransform pixel;
};

struct TileAnim {
    std::string name;
    std::vector<TileAnimTransform *> transforms;
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
void tileAnimDraw(TileAnim *anim, Image *tiles, int tile, int scale, int x, int y);

#endif
