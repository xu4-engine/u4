/*
 * $Id$
 */

#ifndef NAMES_H
#define NAMES_H

#include "savegame.h"
#include "direction.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * These routines convert the various enumerations for classes, reagents,
 * etc. into the textual representations used in the game.
 */
const char *getClassName(ClassType klass);
const char *getReagentName(Reagent reagent);
const char *getVirtueName(Virtue virtue);
const char *getPrincipleName(int principleMask);
int getPrinciples(Virtue virtue);
const char *getVirtueAdjective(Virtue virtue);
const char *getStoneName(Virtue virtue);
const char *getItemName(Item item);
const char *getDirectionName(Direction dir);

#ifdef __cplusplus
}
#endif

#endif
