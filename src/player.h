/*
 * $Id$
 */

#ifndef PLAYER_H
#define PLAYER_H

#include "savegame.h"

typedef enum {
    KA_FOUND_ITEM,
    KA_STOLE_CHEST,
    KA_GAVE_TO_BEGGAR,
    KA_BRAGGED,
    KA_HUMBLE,
    KA_HAWKWIND
} KarmaAction;

int playerGetRealLevel(const SaveGamePlayerRecord *player);
int playerGetMaxLevel(const SaveGamePlayerRecord *player);
void playerAdvanceLevel(SaveGamePlayerRecord *player);
int playerGetMaxMp(const SaveGamePlayerRecord *player);
int playerCanWear(const SaveGamePlayerRecord *player, ArmorType armor);
int playerCanReady(const SaveGamePlayerRecord *player, WeaponType weapon);
int playerAdjustKarma(SaveGame *saveGame, KarmaAction action);
void playerGetChest(SaveGame *saveGame);
int playerDonate(SaveGame *saveGame, int quantity);
void playerJoin(SaveGame *saveGame, const char *name);

#endif
