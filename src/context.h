/*
 * $Id$
 */

#ifndef CONTEXT_H
#define CONTEXT_H

struct _SaveGame;
struct _Map;

typedef struct _Context {
    struct _SaveGame *saveGame;
    struct _Context *parent;
    const struct _Map *map;
    int line, col;
} Context;

extern Context *c;

#endif
