/*
 * $Id$
 */

#ifndef SHRINE_H
#define SHRINE_H

#include "savegame.h"
#include "portal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SHRINE_MEDITATION_INTERVAL  100
#define MEDITATION_MANTRAS_PER_CYCLE 16

typedef struct _Shrine {
    Virtue virtue;
    const char *mantra;
} Shrine;

int shrineCanEnter(const Portal *p);
void shrineEnter(const Shrine *s);

#ifdef __cplusplus
}
#endif

#endif
