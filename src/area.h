/*
 * $Id$
 */

#ifndef AREA_H
#define AREA_H

#include "map.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AREA_MONSTERS 16
#define AREA_PLAYERS 8

typedef struct _Area {
    MapCoords monster_start[AREA_MONSTERS];
    MapCoords player_start[AREA_PLAYERS];
} Area;

#ifdef __cplusplus
}
#endif

#endif
