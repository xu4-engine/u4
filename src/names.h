/*
 * $Id$
 */

#ifndef NAMES_H
#define NAMES_H

#include "savegame.h"

const char *getClassName(ClassType klass);
const char *getWeaponName(WeaponType weapon);
const char *getWeaponAbbrev(WeaponType weapon);
const char *getArmorName(ArmorType armor);
const char *getReagentName(Reagent reagent);
const char *getVirtueName(Virtue virtue);
const char *getStoneName(Virtue virtue);
const char *getItemName(Item item);

#endif
