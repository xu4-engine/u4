/*
 * $Id$
 */

#ifndef CAMP_H
#define CAMP_H

#define CAMP_HEAL_INTERVAL  100   /* Number of moves before camping will heal the party */

#define CAMP_REST_SECONDS   10
#define INN_REST_SECONDS    8

void campBegin(void);
void innBegin(void);

#endif
