/*
 * $Id$
 */

#ifndef MOONGATE_H
#define MOONGATE_H

typedef struct _Moongate {
    unsigned char x, y;
} Moongate;

const Moongate *moongateGetGateForPhase(int phase);
int moongateFindActiveGateAt(int trammel, int felucca, int x, int y, int *destx, int *desty);
int moongateIsEntryToShrineOfSpirituality(int trammel, int felucca);

#endif
