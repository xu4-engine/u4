/*
 * $Id$
 */

#ifndef SHRINE_H
#define SHRINE_H

#include "savegame.h"

typedef struct _Shrine {
    Virtue virtue;
    const char *mantra;
} Shrine;

void shrineEnter(const Shrine *s);

#endif
