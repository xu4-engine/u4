/*
 * $Id$
 */

#ifndef ITEM_H
#define ITEM_H

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
    const Map* map;
    int (*isItemInInventory)(void *);
    void (*putItemInInventory)(void *);
    void *data;
    unsigned char conditions;
} ItemLocation;

const ItemLocation *itemAtLocation(const Map *map, unsigned short x, unsigned short y, unsigned short z);
void itemUse(const char *shortname);

#endif
