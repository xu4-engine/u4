/*
 * $Id$
 */

#ifndef TILEANIM_H
#define TILEANIM_H

#include "types.h"
#include "xml.h"

struct _Image;
struct _TileAnimTransform;
struct _TileAnim;

typedef xu4_list<struct _TileAnimTransform*> TileAnimTransformList;
typedef xu4_map<string, struct _TileAnim*, std::less<string> > TileAnimSetAnimMap;

typedef enum {
    TRANSFORM_INVERT
} TileAnimTransformType;

typedef struct _TileAnimTransform {
    TileAnimTransformType type;
    int x, y, w, h;
} TileAnimTransform;

typedef struct _TileAnim {
    string name;
    TileAnimTransformList transforms;
} TileAnim;

typedef struct _TileAnimSet {
    string name;
    TileAnimSetAnimMap tileanims;
} TileAnimSet;

TileAnimSet *tileAnimSetLoadFromXml(xmlNodePtr node);
TileAnim *tileAnimLoadFromXml(xmlNodePtr node);
TileAnimTransform *tileAnimTransformLoadFromXml(xmlNodePtr node);
TileAnim *tileAnimSetGetAnimByName(TileAnimSet *set, string name);
void tileAnimDraw(TileAnim *anim, struct _Image *tiles, int tile, int scale, int x, int y);

#endif
