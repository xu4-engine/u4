/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "moongate.h"

static const Moongate gates[] = {
    { 224, 133 },               /* moonglow: I'F" O'A" */
    {  96, 103 },               /* britain: G'H" G'A" */
    {  38, 224 },               /* jhelom: O'A" C'G" */
    {  50,  37 },               /* yew: C'F" D'C" */
    { 166,  19 },               /* minoc: B'A" K'G" */
    { 103, 194 },               /* trinsic: M'C" G'H" */
    {  23, 126 },               /* skara brae: H'O" B'H" */
    { 187, 167 }                /* magincia: K'H" L'L" */    
};

const Moongate *moongateGetGateForPhase(int phase) {
    assert(phase < (sizeof(gates) / sizeof(gates[0])));
    return &(gates[phase]);
}

int moongateFindActiveGateAt(int trammel, int felucca, int x, int y, int *destx, int *desty) {
    const Moongate *entry, *dest;

    entry = moongateGetGateForPhase(trammel);
    if (entry->x == x &&
        entry->y == y) {
        dest = moongateGetGateForPhase(felucca);
        *destx = dest->x;
        *desty = dest->y;
        return 1;
    }

    return 0;
}

int moongateIsEntryToShrineOfSpirituality(int trammel, int felucca) {
    return 
        trammel == 4 && 
        felucca == 4;
}
