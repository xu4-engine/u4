/*
 * $Id$
 */

#ifndef ITEM_H
#define ITEM_H

struct _Map;

typedef enum {
    SC_NONE         = 0x00,
    SC_NEWMOONS     = 0x01,
    SC_FULLAVATAR   = 0x02,
    SC_REAGENTDELAY = 0x04
} SearchCondition;

typedef struct _ItemLocation {
    const char *name;
    const char *shortname;
    unsigned short x, y, z;
    unsigned char mapid;
    int (*isItemInInventory)(void *);
    void (*putItemInInventory)(void *);
    void (*useItem)(void *);
    void *data;
    unsigned char conditions;
} ItemLocation;

typedef void (*DestroyAllMonstersCallback)(void);

void itemSetDestroyAllMonstersCallback(DestroyAllMonstersCallback callback);
const ItemLocation *itemAtLocation(const struct _Map *map, int x, int y, int z);
void itemUse(const char *shortname);

#endif
