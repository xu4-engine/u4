/*
 * $Id$
 */

#ifndef MOONGATE_H
#define MOONGATE_H

#include "map.h"

#ifdef __cplusplus
extern "C" {
#endif

void moongateAdd(int phase, MapCoords coords);
const MapCoords *moongateGetGateCoordsForPhase(int phase);
bool moongateFindActiveGateAt(int trammel, int felucca, MapCoords src, MapCoords *dest);
bool moongateIsEntryToShrineOfSpirituality(int trammel, int felucca);

#ifdef __cplusplus
}
#endif

#endif
