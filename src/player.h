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
    KA_HAWKWIND,
    KA_MEDITATION,
    KA_BAD_MANTRA,
    KA_FLED,
    KA_DONATED_BLOOD,
    KA_DIDNT_DONATE_BLOOD
} KarmaAction;

int playerGetRealLevel(const SaveGamePlayerRecord *player);
int playerGetMaxLevel(const SaveGamePlayerRecord *player);
void playerAdvanceLevel(SaveGamePlayerRecord *player);
int playerGetMaxMp(const SaveGamePlayerRecord *player);
int playerCanWear(const SaveGamePlayerRecord *player, ArmorType armor);
int playerCanReady(const SaveGamePlayerRecord *player, WeaponType weapon);
int playerCanEnterShrine(const SaveGame *saveGame, Virtue virtue);
int playerAdjustKarma(SaveGame *saveGame, KarmaAction action);
void playerGetChest(SaveGame *saveGame);
int playerDonate(SaveGame *saveGame, int quantity);
void playerJoin(SaveGame *saveGame, const char *name);

#endif
