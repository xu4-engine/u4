/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include "player.h"

#include "armor.h"
#include "context.h"
#include "debug.h"
#include "game.h"
#include "location.h"
#include "types.h"
#include "utils.h"
#include "weapon.h"

LostEighthCallback lostEighthCallback = NULL;
AdvanceLevelCallback advanceLevelCallback = NULL;
ItemStatsChangedCallback itemStatsChangedCallback = NULL;
PartyStarvingCallback partyStarvingCallback = NULL;
SetTransportCallback setTransportCallback = NULL;

/**
 * Sets up a callback to handle player losing an eighth of his or her
 * avatarhood.
 */
void playerSetLostEighthCallback(LostEighthCallback callback) {
    lostEighthCallback = callback;
}

void playerSetAdvanceLevelCallback(AdvanceLevelCallback callback) {
    advanceLevelCallback = callback;
}

void playerSetItemStatsChangedCallback(ItemStatsChangedCallback callback) {
    itemStatsChangedCallback = callback;
}

void playerSetPartyStarvingCallback(PartyStarvingCallback callback) {
    partyStarvingCallback = callback;
}

void playerSetSetTransportCallback(SetTransportCallback callback) {
    setTransportCallback = callback;
}

/**
 * Applies damage to a player, and changes status to dead if hit
 * points drop to zero or below.
 */
void playerApplyDamage(SaveGamePlayerRecord *player, int damage) {
    int newHp = player->hp;

    if (player->status == STAT_DEAD)
        return;

    newHp -= damage;

    if (newHp < 0) {
        player->status = STAT_DEAD;
        newHp = 0;
    }
    player->hp = newHp;
}

/**
 * Determine what level a character has.
 */
int playerGetRealLevel(const SaveGamePlayerRecord *player) {
    return player->hpMax / 100;
}

/**
 * Determine the highest level a character could have with the number
 * of experience points he has.
 */
int playerGetMaxLevel(const SaveGamePlayerRecord *player) {
    int level = 1;
    int next = 100;

    while (player->xp >= next && level < 8) {
        level++;
        next *= 2;
    }

    return level;
}

void playerAdvanceLevel(SaveGamePlayerRecord *player) {
    if (playerGetRealLevel(player) == playerGetMaxLevel(player))
        return;
    player->status = STAT_GOOD;
    player->hpMax = playerGetMaxLevel(player) * 100;
    player->hp = player->hpMax;

    /* improve stats by 1-8 each */
    player->str   += xu4_random(8) + 1;
    player->dex   += xu4_random(8) + 1;
    player->intel += xu4_random(8) + 1;

    if (player->str > 50) player->str = 50;
    if (player->dex > 50) player->dex = 50;
    if (player->intel > 50) player->intel = 50;

    if (advanceLevelCallback)
        (*advanceLevelCallback)(player);
}

/**
 * Award a player experience points.  Maxs out the players xp at 9999.
 */
void playerAwardXp(SaveGamePlayerRecord *player, int xp) {
    player->xp += xp;
    if (player->xp > 9999)
        player->xp = 9999;
}

/**
 * Determine the most magic points a character could given his class
 * and intelligence.
 */
int playerGetMaxMp(const SaveGamePlayerRecord *player) {
    int max_mp = -1;

    switch (player->klass) {
    case CLASS_MAGE:            /*  mage: 200% of int */
        max_mp = player->intel * 2;
        break;

    case CLASS_DRUID:           /* druid: 150% of int */
        max_mp = player->intel * 3 / 2;
        break;

    case CLASS_BARD:            /* bard, paladin, ranger: 100% of int */
    case CLASS_PALADIN:
    case CLASS_RANGER:
        max_mp = player->intel;
        break;

    case CLASS_TINKER:          /* tinker: 50% of int */
        max_mp = player->intel / 2;
        break;

    case CLASS_FIGHTER:         /* fighter, shepherd: no mp at all */
    case CLASS_SHEPHERD:
        max_mp = 0;
        break;

    default:
        ASSERT(0, "invalid player class: %d", player->klass);
    }

    /* mp always maxes out at 99 */
    if (max_mp > 99)
        max_mp = 99;

    return max_mp;
}

int playerCanEnterShrine(Virtue virtue) {
    if (c->saveGame->runes & (1 << (int) virtue))
        return 1;
    else
        return 0;
}

/**
 * Adjusts the avatar's karma level for the given action.  Activate
 * the lost eighth callback if the player has lost avatarhood.
 */
void playerAdjustKarma(KarmaAction action) {
    int timeLimited = 0;
    int v, newKarma[VIRT_MAX], maxVal[VIRT_MAX];

    /*
     * make a local copy of all virtues, and adjust it according to
     * the game rules
     */
    for (v = 0; v < VIRT_MAX; v++) {
        newKarma[v] = c->saveGame->karma[v] == 0 ? 100 : c->saveGame->karma[v];
        maxVal[v] = c->saveGame->karma[v] == 0 ? 100 : 99;
    }

    switch (action) {
    case KA_FOUND_ITEM:
        AdjustValueMax(newKarma[VIRT_HONOR], 5, maxVal[VIRT_HONOR]);
        break;
    case KA_STOLE_CHEST:
        AdjustValueMin(newKarma[VIRT_HONESTY], -1, 1);
        AdjustValueMin(newKarma[VIRT_JUSTICE], -1, 1);
        AdjustValueMin(newKarma[VIRT_HONOR], -1, 1);
        break;
    case KA_GAVE_TO_BEGGAR:
        timeLimited = 1;
        AdjustValueMax(newKarma[VIRT_COMPASSION], 2, maxVal[VIRT_COMPASSION]);
        AdjustValueMax(newKarma[VIRT_HONOR], 3, maxVal[VIRT_HONOR]); /* FIXME: verify if honor goes up */
        break;
    case KA_BRAGGED:
        AdjustValueMin(newKarma[VIRT_HUMILITY], -5, 1);
        break;
    case KA_HUMBLE:
        timeLimited = 1;
        AdjustValueMax(newKarma[VIRT_HUMILITY], 10, maxVal[VIRT_HUMILITY]);
        break;
    case KA_HAWKWIND:
    case KA_MEDITATION:
        AdjustValueMax(newKarma[VIRT_SPIRITUALITY], 3, maxVal[VIRT_SPIRITUALITY]);
        break;
    case KA_BAD_MANTRA:
        AdjustValueMin(newKarma[VIRT_SPIRITUALITY], -3, 1);
        break;
    case KA_ATTACKED_GOOD:
        AdjustValueMin(newKarma[VIRT_COMPASSION], -5, 1);
        AdjustValueMin(newKarma[VIRT_JUSTICE], -5, 1);
        AdjustValueMin(newKarma[VIRT_HONOR], -5, 1);
        break;
    case KA_FLED_EVIL:
        AdjustValueMin(newKarma[VIRT_VALOR], -2, 1);
        break;
    case KA_HEALTHY_FLED_EVIL:
        AdjustValueMin(newKarma[VIRT_VALOR], -2, 1);
        AdjustValueMin(newKarma[VIRT_SACRIFICE], -2, 1);
        break;
    case KA_KILLED_EVIL:
        AdjustValueMax(newKarma[VIRT_VALOR], xu4_random(2), maxVal[VIRT_VALOR]); /* gain one valor half the time, zero the rest */
        break;
    case KA_SPARED_GOOD:        
        AdjustValueMax(newKarma[VIRT_COMPASSION], 1, maxVal[VIRT_COMPASSION]);
        AdjustValueMax(newKarma[VIRT_JUSTICE], 1, maxVal[VIRT_JUSTICE]);
        break;    
    case KA_DONATED_BLOOD:
        AdjustValueMax(newKarma[VIRT_SACRIFICE], 5, maxVal[VIRT_SACRIFICE]);
        break;
    case KA_DIDNT_DONATE_BLOOD:
        AdjustValueMin(newKarma[VIRT_SACRIFICE], -5, 1);
        break;
    case KA_CHEAT_REAGENTS:
        AdjustValueMin(newKarma[VIRT_HONESTY], -10, 1);
        AdjustValueMin(newKarma[VIRT_JUSTICE], -10, 1);
        AdjustValueMin(newKarma[VIRT_HONOR], -10, 1);
        break;
    case KA_DIDNT_CHEAT_REAGENTS:
        timeLimited = 1;
        AdjustValueMax(newKarma[VIRT_HONESTY], 2, maxVal[VIRT_HONESTY]);
        AdjustValueMax(newKarma[VIRT_JUSTICE], 2, maxVal[VIRT_JUSTICE]);
        AdjustValueMax(newKarma[VIRT_HONOR], 2, maxVal[VIRT_HONOR]);
        break;
    case KA_USED_SKULL:
        /* using the skull is very, very bad... */
        for (v = 0; v < VIRT_MAX; v++)
            AdjustValueMin(newKarma[v], -5, -1);
        break;
    case KA_DESTROYED_SKULL:
        /* ...but destroying it is very, very good */
        for (v = 0; v < VIRT_MAX; v++)
            AdjustValueMax(newKarma[v], 10, maxVal[v]);
        break;
    }

    /*
     * check if enough time has passed since last virtue award if
     * action is time limited -- if not, throw away new values
     */
    if (timeLimited) {
        if (((c->saveGame->moves / 16) >= 0x10000) || (((c->saveGame->moves / 16) & 0xFFFF) != c->saveGame->lastvirtue))
            c->saveGame->lastvirtue = (c->saveGame->moves / 16) & 0xFFFF;
        else
            return;
    }

    /*
     * return to u4dos compatibility and handle losing of eighths
     */
    for (v = 0; v < VIRT_MAX; v++) {
        if (maxVal[v] == 100) { /* already an avatar */
            if (newKarma[v] < 100) { /* but lost it */
                if (lostEighthCallback)
                    (*lostEighthCallback)((Virtue)v);
                c->saveGame->karma[v] = newKarma[v];
            }
            else c->saveGame->karma[v] = 0; /* return to u4dos compatibility */
        }
        else c->saveGame->karma[v] = newKarma[v];
    }
}

int playerAttemptElevation(Virtue virtue) {
    if (c->saveGame->karma[virtue] == 99) {
        c->saveGame->karma[virtue] = 0;
        return 1;
    } else
        return 0;
}

int playerGetChest() {
    int gold = xu4_random(50) + xu4_random(8) + 10;
    playerAdjustGold(gold);    

    return gold;
}

int playerDonate(int quantity) {
    if (quantity > c->saveGame->gold)
        return 0;

    playerAdjustGold(-quantity);
    playerAdjustKarma(KA_GAVE_TO_BEGGAR);

    if (itemStatsChangedCallback)
        (*itemStatsChangedCallback)();

    return 1;
}

int playerCanPersonJoin(string name, Virtue *v) {
    int i;

    if (name.empty())
        return 0;    

    for (i = 1; i < 8; i++) {
        if (name == c->players[i].name) {
            if (v)
                *v = (Virtue) c->players[i].klass;
            return 1;
        }
    }
    return 0;
}

int playerIsPersonJoined(string name) {
    int i;

    if (name.empty())
        return 0;

    for (i = 1; i < c->saveGame->members; i++) {
        if (name == c->players[i].name)
            return 1;
    }
    return 0;
}

CannotJoinError playerJoin(string name) {
    int i;
    SaveGamePlayerRecord tmp;

    for (i = c->saveGame->members; i < 8; i++) {
        if (name == c->players[i].name) {

            /* ensure avatar is experienced enough */
            if (c->saveGame->members + 1 > (c->players[0].hpMax / 100))
                return JOIN_NOT_EXPERIENCED;

            /* ensure character has enough karma */
            if ((c->saveGame->karma[c->players[i].klass] > 0) &&
                (c->saveGame->karma[c->players[i].klass] < 40))
                return JOIN_NOT_VIRTUOUS;

            tmp = c->players[c->saveGame->members];
            c->players[c->saveGame->members] = c->players[i];
            c->players[i] = tmp;
            c->saveGame->members++;
            return JOIN_SUCCEEDED;
        }
    }

    return JOIN_NOT_EXPERIENCED;
}

void playerEndTurn(void) {
    int i;        
    SaveGame *saveGame = c->saveGame;
    
    saveGame->moves++;   
   
    for (i = 0; i < saveGame->members; i++) {

        /* Handle player status (only for non-combat turns) */
        if ((c->location->context & CTX_NON_COMBAT) == c->location->context) {
            
            /* party members eat food (also non-combat) */
            if (c->players[i].status != STAT_DEAD)
                playerAdjustFood(-1);

            switch (c->players[i].status) {
            case STAT_SLEEPING:            
                if (xu4_random(5) == 0)
                    c->players[i].status = STAT_GOOD;
                break;

            case STAT_POISONED:            
                playerApplyDamage(&c->players[i], 2);
                break;

            default:
                break;
            }
        }

        /* regenerate magic points */
        if (!playerIsDisabled(i) &&        
            c->players[i].mp < playerGetMaxMp(&(c->players[i])))
            c->players[i].mp++;        
    }

    /* The party is starving! */
    if ((saveGame->food == 0) && ((c->location->context & CTX_NON_COMBAT) == c->location->context))
        (*partyStarvingCallback)();
    
    /* heal ship (25% chance it is healed each turn) */
    if ((c->location->context == CTX_WORLDMAP) && (saveGame->shiphull < 50) && xu4_random(4) == 0)
        saveGame->shiphull++;
}

void playerApplyEffect(TileEffect effect, int player) {
    int i;
    
    for (i = 0; i < c->saveGame->members; i++) {

        if (i != player && player != ALL_PLAYERS)
            continue;

        if (c->players[i].status == STAT_DEAD)
            continue;

        switch (effect) {
        case EFFECT_NONE:
            break;
        case EFFECT_LAVA:
        case EFFECT_FIRE:
            if (i == player)
                playerApplyDamage(&(c->players[i]), 16 + (xu4_random(32)));                
            else if (player == ALL_PLAYERS && xu4_random(2) == 0)
                playerApplyDamage(&(c->players[i]), 10 + (xu4_random(25)));
            break;
        case EFFECT_SLEEP:
            if (i == player || xu4_random(5) == 0)
                c->players[i].status = STAT_SLEEPING;
            break;
        case EFFECT_POISONFIELD:
        case EFFECT_POISON:
            if (i == player || xu4_random(5) == 0)
                c->players[i].status = STAT_POISONED;
            break;
        case EFFECT_ELECTRICITY: break;
        default:
            ASSERT(0, "invalid effect: %d", effect);
        }
    }
}

/**
 * Whether or not the party can make an action.
 */
int playerPartyImmobilized() {
    int i, immobile = 1;

    for (i = 0; i < c->saveGame->members; i++) {
        if (!playerIsDisabled(i))        
            immobile = 0;        
    }

    return immobile;
}

/**
 * Whether or not all the party members are dead.
 */
int playerPartyDead() {
    int i, dead = 1;

    for (i = 0; i < c->saveGame->members; i++) {
        if (c->players[i].status != STAT_DEAD) {
            dead = 0;
        }
    }

    return dead;
}

/**
 * Applies a sleep spell to the party.
 */
void playerApplySleepSpell(SaveGamePlayerRecord *player) {
    if (player->status != STAT_DEAD && xu4_random(2) == 0)
        player->status = STAT_SLEEPING;
}

int playerHeal(HealType type, int player) {
    if (player >= c->saveGame->members)
        return 0;

    switch(type) {

    case HT_NONE:
        return 1;

    case HT_CURE:
        if (c->players[player].status != STAT_POISONED)
            return 0;
        c->players[player].status = STAT_GOOD;
        break;

    case HT_FULLHEAL:
        if (c->players[player].status == STAT_DEAD ||
            c->players[player].hp == c->players[player].hpMax)
            return 0;
        c->players[player].hp = c->players[player].hpMax;
        break;

    case HT_RESURRECT:
        if (c->players[player].status != STAT_DEAD)
            return 0;
        c->players[player].status = STAT_GOOD;        
        break;

    case HT_HEAL:
        if (c->players[player].status == STAT_DEAD ||
            c->players[player].hp == c->players[player].hpMax)
            return 0;        

        c->players[player].hp += 75 + (xu4_random(0x100) % 0x19);
        break;

    case HT_CAMPHEAL:
        if (c->players[player].status == STAT_DEAD ||
            c->players[player].hp == c->players[player].hpMax)
            return 0;        
        c->players[player].hp += 99 + (xu4_random(0x100) & 0x77);
        break;

    case HT_INNHEAL:
        if (c->players[player].status == STAT_DEAD ||
            c->players[player].hp == c->players[player].hpMax)
            return 0;        
        c->players[player].hp += 100 + (xu4_random(50) * 2);
        break;

    default:
        return 0;
    }

    if (c->players[player].hp > c->players[player].hpMax)
        c->players[player].hp = c->players[player].hpMax;
    
    return 1;
}

void playerReviveParty() {
    int i;

    for (i = 0; i < c->saveGame->members; i++) {
        c->players[i].status = STAT_GOOD;
        c->players[i].hp = c->players[i].hpMax;
    }

    c->saveGame->food = 20099;
    c->saveGame->gold = 200;    
    (*setTransportCallback)(AVATAR_TILE);
}

/**
 * Determine whether a players attack hits or not.
 */
int playerAttackHit(const SaveGamePlayerRecord *player) {   
    if (weaponAlwaysHits(player->weapon) || player->dex >= 40)
        return 1;

    if ((player->dex + 128) >= xu4_random(0x100))
        return 1;
    else
        return 0;
}

/**
 * Calculate damage for an attack.
 */
int playerGetDamage(const SaveGamePlayerRecord *player) {
    int maxDamage;

    maxDamage = weaponGetDamage(player->weapon);
    maxDamage += player->str;
    if (maxDamage > 255)
        maxDamage = 255;

    return xu4_random(maxDamage);
}

/**
 * Determine whether a player is hit by a melee attack.
 */
int playerIsHitByAttack(const SaveGamePlayerRecord *player) {
    return xu4_random(0x100) > armorGetDefense(player->armor);
}

/**
 * Lose the equipped weapon for the player (flaming oil, ranged daggers, etc.)
 * Returns the number of weapons left of that type, including the one in
 * the players hand
 */

int playerLoseWeapon(int player) {
    int weapon = c->players[player].weapon;
    if (c->saveGame->weapons[weapon] > 0)
        return (--c->saveGame->weapons[weapon]) + 1;
    else {
        c->players[player].weapon = WEAP_HANDS;    
        return 0;
    }
}

void playerAdjustGold(int gold) {
    long curr = c->saveGame->gold;
    AdjustValue(curr, gold, 9999, 0);
    c->saveGame->gold = (unsigned short)curr;
}

void playerAdjustFood(int food) {
    c->saveGame->food += food;
    if (c->saveGame->food > 999900)
        c->saveGame->food = (food > 0) ? 999900 : 0;
}

int playerIsDisabled(int player) {
    return (c->players[player].status == STAT_GOOD ||
        c->players[player].status == STAT_POISONED) ? 0 : 1;
}
