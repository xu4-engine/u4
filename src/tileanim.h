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
    TRANSFORM_INVERT,
    TRANSFORM_PIXEL
} TileAnimTransformType;

typedef struct {
    int x, y, w, h;
} TileAnimInvertTransform;

typedef struct {
    int x, y;
    struct _ListNode *colors;
} TileAnimPixelTransform;

typedef struct {
    TileAnimTransformType type;
    union {
        TileAnimInvertTransform invert;
        TileAnimPixelTransform pixel;
    };
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
struct _RGBA *tileAnimColorLoadFromXml(xmlNodePtr node);
TileAnim *tileAnimSetGetAnimByName(TileAnimSet *set, string name);
void tileAnimDraw(TileAnim *anim, struct _Image *tiles, int tile, int scale, int x, int y);

#endif
