/*
 * $Id$
 */

#ifndef CONTEXT_H
#define CONTEXT_H

struct _SaveGame;
struct _Map;
struct _Person;

typedef struct _Context {
    struct _SaveGame *saveGame;
    struct _Context *parent;
    const struct _Map *map;
    const struct _Person *talker;
    int line, col;
} Context;

extern Context *c;

#endif
