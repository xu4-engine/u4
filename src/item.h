/*
 * $Id$
 */

#ifndef ITEM_H
#define ITEM_H

typedef enum {
    SC_NONE,
    SC_NEWMOONS,
    SC_FULLAVATAR
} SearchCondition;

typedef struct _ItemLocation {
    const char *name;
    unsigned char x, y;
    const Map* map;
    int (*isItemInInventory)(void *);
    void (*putItemInInventory)(void *);
    void *data;
    unsigned char conditions;
} ItemLocation;

const ItemLocation *itemAtLocation(const Map *map, unsigned char x, unsigned char y);

#endif
