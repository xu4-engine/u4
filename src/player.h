/*
 * $Id$
 */

#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include "savegame.h"
#include "tile.h"
#include "types.h"

using std::string;

#ifdef __cplusplus
extern "C" {
#endif

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
    KA_HEALTHY_FLED_EVIL,
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
    HT_CAMPHEAL,
    HT_INNHEAL
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

typedef enum {    
    JOIN_SUCCEEDED,
    JOIN_NOT_EXPERIENCED,
    JOIN_NOT_VIRTUOUS
} CannotJoinError;

typedef void (*LostEighthCallback)(Virtue);
typedef void (*AdvanceLevelCallback)(const SaveGamePlayerRecord *player);
typedef void (*ItemStatsChangedCallback)(void);
typedef void (*PartyStarvingCallback)(void);
typedef void (*SetTransportCallback)(MapTile tile);

void playerSetLostEighthCallback(LostEighthCallback callback);
void playerSetAdvanceLevelCallback(AdvanceLevelCallback callback);
void playerSetItemStatsChangedCallback(ItemStatsChangedCallback callback);
void playerSetPartyStarvingCallback(PartyStarvingCallback callback);
void playerSetSetTransportCallback(SetTransportCallback callback);
void playerApplyDamage(SaveGamePlayerRecord *player, int damage);
int playerGetRealLevel(const SaveGamePlayerRecord *player);
int playerGetMaxLevel(const SaveGamePlayerRecord *player);
void playerAdvanceLevel(SaveGamePlayerRecord *player);
void playerAwardXp(SaveGamePlayerRecord *player, int xp);
int playerGetMaxMp(const SaveGamePlayerRecord *player);
int playerCanEnterShrine(Virtue virtue);
void playerAdjustKarma(KarmaAction action);
int playerAttemptElevation(Virtue virtue);
int playerGetChest();
int playerDonate(int quantity);
int playerCanPersonJoin(string name, Virtue *v);
int playerIsPersonJoined(string name);
CannotJoinError playerJoin(string name);
void playerEndTurn(void);
void playerApplyEffect(TileEffect effect, int player);
int playerPartyImmobilized();
int playerPartyDead();
void playerApplySleepSpell(SaveGamePlayerRecord *player);
int playerHeal(HealType type, int player);
void playerReviveParty();
int playerCanAfford(int price);
int playerPurchase(InventoryItem item, int type, int quantity, int price);
int playerCanSell(InventoryItem item, int type, int quantity);
int playerSell(InventoryItem item, int type, int quantity, int price);
int playerAttackHit(const SaveGamePlayerRecord *player);
int playerGetDamage(const SaveGamePlayerRecord *player);
int playerIsHitByAttack(const SaveGamePlayerRecord *player);
int playerLoseWeapon(int player);
void playerAdjustGold(int gold);
void playerAdjustFood(int food);
int playerIsDisabled(int player);

#ifdef __cplusplus
}
#endif

#endif
