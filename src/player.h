/*
 * $Id$
 */

#ifndef PLAYER_H
#define PLAYER_H

#include "savegame.h"

#define KARMA_ADJ_FOUND_ITEM +5

int playerGetLevel(const SaveGamePlayerRecord *player);
int playerGetMaxMp(const SaveGamePlayerRecord *player);
int playerCanWear(const SaveGamePlayerRecord *player, ArmorType armor);
int playerCanReady(const SaveGamePlayerRecord *player, WeaponType weapon);
int playerAdjustKarma(SaveGame *saveGame, Virtue virt, int adj);

#endif
