/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include "utils.h"
#include <cstdlib>
#include <ctime>

/**
 * Seed the random number generator.
 */
void xu4_srandom() {
    srand(time(NULL));
}

/**
 * Generate a random number between 0 and (upperRange - 1).  This
 * routine uses the upper bits of random number provided by rand() to
 * compensate for older generators that have low entropy in the lower
 * bits (e.g. MacOS X).
 */
int xu4_random(int upperRange) {
    int r = rand();
    return (int) ((((double)upperRange) * r) / (RAND_MAX+1.0));
}
