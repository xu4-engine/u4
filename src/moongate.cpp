/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>

#include "error.h"
#include "map.h"
#include "moongate.h"
#include "types.h"

typedef xu4_map<int, MapCoords, std::less<int> > MoongateList; /* map moon phase to map coordinates */

MoongateList gates; 

void moongateAdd(int phase, MapCoords coords) {
    if (!gates.insert(MoongateList::value_type(phase, coords)).second)
        errorFatal("Error: A moongate for phase %d already exists", phase);    
}

const MapCoords *moongateGetGateCoordsForPhase(int phase) {
    MoongateList::iterator moongate;

    moongate = gates.find(phase);
    if (moongate != gates.end())
        return &moongate->second;
    return NULL;
}

bool moongateFindActiveGateAt(int trammel, int felucca, MapCoords src, MapCoords *dest) {
    const MapCoords *moongate_coords;

    moongate_coords = moongateGetGateCoordsForPhase(trammel);
    if (moongate_coords && (src == *moongate_coords)) {
        moongate_coords = moongateGetGateCoordsForPhase(felucca);
        if (moongate_coords) {
            *dest = *moongate_coords;
            return true;
        }
    }
    return false;
}

bool moongateIsEntryToShrineOfSpirituality(int trammel, int felucca) {
    return (trammel == 4 && felucca == 4) ? true : false;
}
