/*
 * $Id$
 */

#ifndef PLAYER_H
#define PLAYER_H

#include "savegame.h"
#include "ttype.h"

#define ALL_PLAYERS -1

typedef enum {
    KA_FOUND_ITEM,
    KA_STOLE_CHEST,
    KA_GAVE_TO_BEGGAR,
    KA_BRAGGED,
    KA_HUMBLE,
    KA_HAWKWIND,
    KA_MEDITATION,
    KA_BAD_MANTRA,
    KA_ATTACKED_GOOD,
    KA_FLED_EVIL,
    KA_KILLED_EVIL,
    KA_SPARED_GOOD,    
    KA_DONATED_BLOOD,
    KA_DIDNT_DONATE_BLOOD,
    KA_CHEAT_REAGENTS,
    KA_DIDNT_CHEAT_REAGENTS,
    KA_USED_SKULL,
    KA_DESTROYED_SKULL
} KarmaAction;

typedef enum {
    HT_NONE,
    HT_CURE,
    HT_FULLHEAL,
    HT_RESURRECT,
    HT_HEAL,
    HT_RESTHEAL
} HealType;

typedef enum {
    INV_NONE,
    INV_WEAPON,
    INV_ARMOR,
    INV_FOOD,
    INV_REAGENT,
    INV_GUILDITEM,
    INV_HORSE
} InventoryItem;

typedef void (*LostEighthCallback)(Virtue);
typedef void (*AdvanceLevelCallback)(const SaveGamePlayerRecord *player);
typedef void (*ItemStatsChangedCallback)(void);

void playerSetLostEighthCallback(LostEighthCallback callback);
void playerSetAdvanceLevelCallback(AdvanceLevelCallback callback);
void playerSetItemStatsChangedCallback(ItemStatsChangedCallback callback);
void playerApplyDamage(SaveGamePlayerRecord *player, int damage);
int playerGetRealLevel(const SaveGamePlayerRecord *player);
int playerGetMaxLevel(const SaveGamePlayerRecord *player);
void playerAdvanceLevel(SaveGamePlayerRecord *player);
void playerAwardXp(SaveGamePlayerRecord *player, int xp);
int playerGetMaxMp(const SaveGamePlayerRecord *player);
int playerCanEnterShrine(const SaveGame *saveGame, Virtue virtue);
void playerAdjustKarma(SaveGame *saveGame, KarmaAction action);
int playerAttemptElevation(SaveGame *saveGame, Virtue virtue);
int playerGetChest(SaveGame *saveGame);
int playerDonate(SaveGame *saveGame, int quantity);
int playerCanPersonJoin(SaveGame *saveGame, const char *name, Virtue *v);
int playerIsPersonJoined(SaveGame *saveGame, const char *name);
int playerJoin(SaveGame *saveGame, const char *name);
void playerEndTurn(void);
void playerApplyEffect(SaveGame *saveGame, TileEffect effect, int player);
int playerPartyImmobilized(const SaveGame *saveGame);
int playerPartyDead(const SaveGame *saveGame);
void playerApplySleepSpell(SaveGamePlayerRecord *player);
int playerHeal(SaveGame *saveGame, HealType type, int player);
void playerReviveParty(SaveGame *saveGame);
int playerCanAfford(SaveGame *saveGame, int price);
int playerPurchase(SaveGame *saveGame, InventoryItem item, int type, int quantity, int price);
int playerCanSell(SaveGame *saveGame, InventoryItem item, int type, int quantity);
int playerSell(SaveGame *saveGame, InventoryItem item, int type, int quantity, int price);
int playerAttackHit(const SaveGamePlayerRecord *player);
int playerGetDamage(const SaveGamePlayerRecord *player);
int playerIsHitByAttack(const SaveGamePlayerRecord *player);
int playerLoseWeapon(SaveGame *saveGame, int player);
void playerAdjustGold(SaveGame *saveGame, int gold);
void playerAdjustFood(SaveGame *saveGame, int food);

#endif
