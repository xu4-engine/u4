/*
 * $Id$
 */

#ifndef MOONGATE_H
#define MOONGATE_H

typedef struct _Moongate {
    int phase;
    unsigned char x, y;
} Moongate;

void moongateAdd(int phase, unsigned char x, unsigned char y);
const Moongate *moongateGetGateForPhase(int phase);
int moongateFindActiveGateAt(int trammel, int felucca, int x, int y, int *destx, int *desty);
int moongateIsEntryToShrineOfSpirituality(int trammel, int felucca);

#endif
