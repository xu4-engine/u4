/*
 * $Id$
 */

#ifndef ITEM_H
#define ITEM_H

#include "map.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SC_NONE         = 0x00,
    SC_NEWMOONS     = 0x01,
    SC_FULLAVATAR   = 0x02,
    SC_REAGENTDELAY = 0x04
} SearchCondition;

typedef struct _ItemLocation {
    const char *name;
    const char *shortname;
    int x, y, z;
    MapId mapid;
    int (*isItemInInventory)(void *);
    void (*putItemInInventory)(void *);
    void (*useItem)(void *);
    void *data;
    unsigned char conditions;
} ItemLocation;

typedef void (*DestroyAllCreaturesCallback)(void);

void itemSetDestroyAllCreaturesCallback(DestroyAllCreaturesCallback callback);
const ItemLocation *itemAtLocation(const Map *map, Coords coords);
void itemUse(string shortname);

#ifdef __cplusplus
}
#endif

#endif
