/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <cstdlib>
#include <map>

#include "error.h"
#include "coords.h"
#include "moongate.h"
#include "types.h"

typedef std::map<int, Coords> MoongateList; /* map moon phase to map coordinates */

MoongateList gates;

void moongateAdd(int phase, const Coords &coords) {
    if (!gates.insert(MoongateList::value_type(phase, coords)).second)
        errorFatal("Error: A moongate for phase %d already exists", phase);
}

const Coords *moongateGetGateCoordsForPhase(int phase) {
    MoongateList::iterator moongate;

    moongate = gates.find(phase);
    if (moongate != gates.end())
        return &moongate->second;
    return NULL;
}

bool moongateFindActiveGateAt(int trammel, int felucca, const Coords &src, Coords &dest) {
    const Coords *moongate_coords;

    moongate_coords = moongateGetGateCoordsForPhase(trammel);
    if (moongate_coords && (src == *moongate_coords)) {
        moongate_coords = moongateGetGateCoordsForPhase(felucca);
        if (moongate_coords) {
            dest = *moongate_coords;
            return true;
        }
    }
    return false;
}

bool moongateIsEntryToShrineOfSpirituality(int trammel, int felucca) {
    return (trammel == 4 && felucca == 4) ? true : false;
}
