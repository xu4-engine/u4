/*
 * $Id$
 */

#ifndef PLAYER_H
#define PLAYER_H

#include "savegame.h"
#include "ttype.h"

typedef enum {
    KA_FOUND_ITEM,
    KA_STOLE_CHEST,
    KA_GAVE_TO_BEGGAR,
    KA_BRAGGED,
    KA_HUMBLE,
    KA_HAWKWIND,
    KA_MEDITATION,
    KA_BAD_MANTRA,
    KA_ATTACKED_NONEVIL,
    KA_FLED,
    KA_KILLED_EVIL,
    KA_SPARED_NONEVIL,
    KA_DONATED_BLOOD,
    KA_DIDNT_DONATE_BLOOD,
    KA_USED_SKULL,
    KA_DESTROYED_SKULL
} KarmaAction;

typedef enum {
    INV_WEAPON,
    INV_ARMOR,
    INV_FOOD,
    INV_REAGENT,
    INV_GUILDITEM,
    INV_HORSE
} InventoryItem;

void playerApplyDamage(SaveGamePlayerRecord *player, int damage);
int playerGetRealLevel(const SaveGamePlayerRecord *player);
int playerGetMaxLevel(const SaveGamePlayerRecord *player);
void playerAdvanceLevel(SaveGamePlayerRecord *player);
int playerGetMaxMp(const SaveGamePlayerRecord *player);
int playerCanWear(const SaveGamePlayerRecord *player, ArmorType armor);
int playerCanReady(const SaveGamePlayerRecord *player, WeaponType weapon);
int playerCanEnterShrine(const SaveGame *saveGame, Virtue virtue);
int playerAdjustKarma(SaveGame *saveGame, KarmaAction action);
int playerAttemptElevation(SaveGame *saveGame, Virtue virtue);
int playerGetChest(SaveGame *saveGame);
int playerDonate(SaveGame *saveGame, int quantity);
void playerJoin(SaveGame *saveGame, const char *name);
void playerEndTurn(SaveGame *saveGame);
void playerApplyEffect(SaveGame *saveGame, TileEffect effect);
int playerPartyImmobilized(const SaveGame *saveGame);
int playerPartyDead(const SaveGame *saveGame);
void playerApplySleepSpell(SaveGame *saveGame);
void playerRevive(SaveGame *saveGame);
int playerPurchase(SaveGame *saveGame, InventoryItem item, int type, int quantity, int price);
int playerSell(SaveGame *saveGame, InventoryItem item, int type, int quantity, int price);
int playerAttackHit(const SaveGamePlayerRecord *player);
int playerGetDamage(const SaveGamePlayerRecord *player);

#endif
