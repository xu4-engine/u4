/*
 * $Id$
 */

#ifndef AREA_H
#define AREA_H

#include "map.h"

#define AREA_MONSTERS 16
#define AREA_PLAYERS 8

struct Area {
    MapCoords monster_start[AREA_MONSTERS];
    MapCoords player_start[AREA_PLAYERS];
};

#endif
