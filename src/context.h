/*
 * $Id$
 */

#ifndef CONTEXT_H
#define CONTEXT_H

struct _SaveGame;
struct _Map;
struct _Annotation;
struct _Person;

typedef struct {
    const struct _Person *talker;
    int question;
    char buffer[5];
} Conversation;

typedef struct _Context {
    struct _SaveGame *saveGame;
    struct _Context *parent;
    const struct _Map *map;
    struct _Annotation *annotation;
    Conversation conversation;
    int line, col;
} Context;

extern Context *c;

#endif
