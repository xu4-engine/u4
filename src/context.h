/*
 * $Id$
 */

#ifndef CONTEXT_H
#define CONTEXT_H

struct _SaveGame;

typedef struct _Context {
    struct _SaveGame *saveGame;
    struct _Context *parent;
    const Map *map;
    int x, y;
    State state;
    int line, col;
} Context;

extern Context *c;

#endif
