/*
 * $Id$
 */

#ifndef MOONGATE_H
#define MOONGATE_H

class Coords;

void moongateAdd(int phase, const Coords &coords);
const Coords *moongateGetGateCoordsForPhase(int phase);
bool moongateFindActiveGateAt(int trammel, int felucca, const Coords &src, Coords &dest);
bool moongateIsEntryToShrineOfSpirituality(int trammel, int felucca);

#endif
