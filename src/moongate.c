/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "list.h"
#include "moongate.h"

ListNode *gates = NULL;

void moongateAdd(int phase, unsigned char x, unsigned char y) {
    Moongate *gate = malloc(sizeof(Moongate));

    if (!gate)
        return;

    gate->phase = phase;
    gate->x = x;
    gate->y = y;

    gates = listAppend(gates, gate);
}

static int moongateCompareToPhase(Moongate *gate, int phase) {
    return gate->phase - phase;
}

const Moongate *moongateGetGateForPhase(int phase) {
    ListNode *n;
    n = listFind(gates, (void *) phase, (ListComparator)&moongateCompareToPhase);
    if (!n)
        return NULL;

    return n->data;
}

int moongateFindActiveGateAt(int trammel, int felucca, int x, int y, int *destx, int *desty) {
    const Moongate *entry, *dest;

    entry = moongateGetGateForPhase(trammel);
    if (entry &&
        entry->x == x &&
        entry->y == y) {
        dest = moongateGetGateForPhase(felucca);
        if (dest) {
            *destx = dest->x;
            *desty = dest->y;
            return 1;
        }
    }

    return 0;
}

int moongateIsEntryToShrineOfSpirituality(int trammel, int felucca) {
    return 
        trammel == 4 && 
        felucca == 4;
}
