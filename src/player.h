/*
 * $Id$
 */

#ifndef PLAYER_H
#define PLAYER_H

#include "savegame.h"

int playerGetLevel(const SaveGamePlayerRecord *player);
int playerGetMaxMp(const SaveGamePlayerRecord *player);
int playerCanWear(const SaveGamePlayerRecord *player, ArmorType armor);
int playerCanReady(const SaveGamePlayerRecord *player, WeaponType weapon);

#endif
