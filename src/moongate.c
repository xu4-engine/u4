/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "moongate.h"

static const Moongate gates[] = {
    {  95, 103 },               /* britain */
    {  38, 224 },               /* jhelom */
    {  50,  37 },               /* yew */
    { 166,  19 },               /* minoc */
    { 103, 195 },               /* trinsic */
    {  23, 126 },               /* skara brae */
    { 187, 167 },               /* magincia */
    { 223, 134 }                /* moonglow */
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

