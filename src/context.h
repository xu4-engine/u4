/*
 * $Id$
 */

#ifndef CONTEXT_H
#define CONTEXT_H

typedef struct _Context{
    struct _Context *parent;
    const Map *map;
    int x, y;
    State state;
    int line, col;
} Context;

extern Context *c;

#endif
