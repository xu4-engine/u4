/*
 * $Id$
 */

#ifndef TILEANIM_H
#define TILEANIM_H

#include "xml.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _ListNode;
struct _Image;
struct _RGBA;

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

typedef struct {
    char *name;
    struct _ListNode *transforms;
} TileAnim;

typedef struct {
    char *name;
    struct _ListNode *tileanims;
} TileAnimSet;

TileAnimSet *tileAnimSetLoadFromXml(xmlNodePtr node);
TileAnim *tileAnimLoadFromXml(xmlNodePtr node);
TileAnimTransform *tileAnimTransformLoadFromXml(xmlNodePtr node);
struct _RGBA *tileAnimColorLoadFromXml(xmlNodePtr node);
TileAnim *tileAnimSetGetAnimByName(TileAnimSet *set, const char *name);
void tileAnimDraw(TileAnim *anim, struct _Image *tiles, int tile, int scale, int x, int y);

#ifdef __cplusplus
}
#endif

#endif
