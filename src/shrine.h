/*
 * $Id$
 */

#ifndef SHRINE_H
#define SHRINE_H

#include "savegame.h"
#include "portal.h"

#define SHRINE_MEDITATION_INTERVAL  100

typedef struct _Shrine {
    Virtue virtue;
    const char *mantra;
} Shrine;

int shrineCanEnter(const Portal *p);
void shrineEnter(const Shrine *s);

#endif
