/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "player.h"
#include "map.h"

LostEighthCallback lostEighthCallback = NULL;
AdvanceLevelCallback advanceLevelCallback = NULL;

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
    player->hpMax = playerGetMaxLevel(player) * 100;
    player->hp = player->hpMax;

    /* FIXME: adjust stats */

    if (advanceLevelCallback)
        (*advanceLevelCallback)(player);
}

/**
 * Determine the most magic points a character could given his class
 * and intelligence.
 */
int playerGetMaxMp(const SaveGamePlayerRecord *player) {
    switch (player->klass) {
    case CLASS_MAGE:
        return player->intel * 2;

    case CLASS_DRUID:
        return player->intel * 3 / 2;

    case CLASS_BARD:
    case CLASS_PALADIN:
    case CLASS_RANGER:
        return player->intel;

    case CLASS_TINKER:
        return player->intel / 2;

    case CLASS_FIGHTER:
    case CLASS_SHEPHERD:
        return 0;
    }

    assert(0);                  /* shouldn't happen */
}

/**
 * Determines whether a player can wear the given armor type.
 */
int playerCanWear(const SaveGamePlayerRecord *player, ArmorType armor) {

    /* anybody can wear mystic robes */
    if (armor == ARMR_MYSTICROBES)
        return 1;

    switch (player->klass) {
    case CLASS_MAGE:
        return armor <= ARMR_CLOTH;
    case CLASS_BARD:
    case CLASS_DRUID:
    case CLASS_RANGER:
    case CLASS_SHEPHERD:
        return armor <= ARMR_LEATHER;

    case CLASS_TINKER:
        return armor <= ARMR_PLATE;

    case CLASS_FIGHTER:
        return armor <= ARMR_MAGICCHAIN;

    case CLASS_PALADIN:
        return 1;
    }

    assert(0);                  /* shoudn't happen */
}

/**
 * Determines whether a player can ready the given weapon type.
 */
int playerCanReady(const SaveGamePlayerRecord *player, WeaponType weapon) {
    static const int weapMask[] = {
        /* WEAP_NONE */     0xff /* all */,
        /* WEAP_STAFF */    0xff /* all */,
        /* WEAP_DAGGER */   0xff /* all */,
        /* WEAP_SLING */    0xff /* all */,
        /* WEAP_MACE */     0xff & (~(1 << CLASS_MAGE)),
        /* WEAP_AXE */      0xff & (~((1 << CLASS_MAGE) | (1 << CLASS_SHEPHERD) | (1 << CLASS_DRUID))),
        /* WEAP_SWORD */    0xff & (~((1 << CLASS_MAGE) | (1 << CLASS_SHEPHERD) | (1 << CLASS_DRUID))),
        /* WEAP_BOW */      0xff & (~((1 << CLASS_MAGE) | (1 << CLASS_SHEPHERD))),
        /* WEAP_CROSSBOW */ 0xff & (~((1 << CLASS_MAGE) | (1 << CLASS_SHEPHERD))),
        /* WEAP_OIL */      0xff /* all */,
        /* WEAP_HALBERD */  (1 << CLASS_FIGHTER) | (1 << CLASS_TINKER) | (1 << CLASS_PALADIN),
        /* WEAP_MAGICAXE */ (1 << CLASS_TINKER) | (1 << CLASS_PALADIN),
        /* WEAP_MAGICSWORD*/(1 << CLASS_FIGHTER) | (1 << CLASS_TINKER) | (1 << CLASS_PALADIN) | (1 << CLASS_RANGER),
        /* WEAP_MAGICBOW */ 0xff & (~((1 << CLASS_MAGE) | (1 << CLASS_FIGHTER) | (1 << CLASS_SHEPHERD))),
        /* WEAP_MAGICWAND */(1 << CLASS_MAGE) | (1 << CLASS_BARD) | (1 << CLASS_DRUID),
        /* WEAP_MYSTICSWORD */0xff /* all */
    };

    if (weapon < WEAP_MAX)
        return ((weapMask[weapon] & (1 << player->klass)) != 0);

    assert(0);                  /* shouldn't happen */
}

int playerCanEnterShrine(const SaveGame *saveGame, Virtue virtue) {
    if (saveGame->runes & (1 << (int) virtue))
        return 1;
    else
        return 0;
}

/**
 * Adjusts the avatar's karma level for the given action.  Activate
 * the lost eighth callback if the player has lost avatarhood.
 */
void playerAdjustKarma(SaveGame *saveGame, KarmaAction action) {
    int timeLimited = 0;
    int v, newKarma[VIRT_MAX];

    /*
     * make a local copy of all virtues, and adjust it according to
     * the game rules
     */
    for (v = 0; v < VIRT_MAX; v++)
        newKarma[v] = saveGame->karma[v];

    switch (action) {
    case KA_FOUND_ITEM:
        newKarma[VIRT_HONOR] += 5;
        break;
    case KA_STOLE_CHEST:
        newKarma[VIRT_HONESTY]--;
        newKarma[VIRT_JUSTICE]--;
        newKarma[VIRT_HONOR]--;
        break;
    case KA_GAVE_TO_BEGGAR:
        timeLimited = 1;
        newKarma[VIRT_COMPASSION] += 2;
        newKarma[VIRT_HONOR] += 3; /* FIXME: verify if honor goes up */
        break;
    case KA_BRAGGED:
        newKarma[VIRT_HUMILITY] -= 5;
        break;
    case KA_HUMBLE:
        timeLimited = 1;
        newKarma[VIRT_HUMILITY] += 10;
        break;
    case KA_HAWKWIND:
    case KA_MEDITATION:
        newKarma[VIRT_SPIRITUALITY] += 3;
        break;
    case KA_BAD_MANTRA:
        newKarma[VIRT_SPIRITUALITY] -= 3;
        break;
    case KA_ATTACKED_NONEVIL:
        newKarma[VIRT_COMPASSION] -= 5;
        newKarma[VIRT_JUSTICE] -= 3;
        newKarma[VIRT_HONOR] -= 3;
        break;
    case KA_FLED:
        newKarma[VIRT_VALOR] -= 2;
        newKarma[VIRT_SACRIFICE] -= 2;
        break;
    case KA_KILLED_EVIL:
        newKarma[VIRT_VALOR] += rand() % 2; /* gain one valor half the time, zero the rest */
        break;
    case KA_SPARED_NONEVIL:
        newKarma[VIRT_COMPASSION]++;
        newKarma[VIRT_JUSTICE]++;
        break;
    case KA_DONATED_BLOOD:
        newKarma[VIRT_SACRIFICE] += 5;
        break;
    case KA_DIDNT_DONATE_BLOOD:
        newKarma[VIRT_SACRIFICE] -= 5;
        break;
    case KA_CHEAT_REAGENTS:
        newKarma[VIRT_HONESTY] -= 10;
        newKarma[VIRT_JUSTICE] -= 10;
        newKarma[VIRT_HONOR] -= 10;
        break;
    case KA_DIDNT_CHEAT_REAGENTS:
        newKarma[VIRT_HONESTY] += 2;
        newKarma[VIRT_JUSTICE] += 2;
        newKarma[VIRT_HONOR] += 2;
        break;
    case KA_USED_SKULL:
        /* using the skull is very, very bad... */
        for (v = 0; v < VIRT_MAX; v++)
            newKarma[v] -= 5;
        break;
    case KA_DESTROYED_SKULL:
        /* ...but destroying it is very, very good */
        for (v = 0; v < VIRT_MAX; v++)
            newKarma[v] += 10;
        break;
    }

    /*
     * check if enough time has passed since last virtue award if
     * action is time limited -- if not, throw away new values
     */
    if (timeLimited) {
        if (((saveGame->moves / 16) & 0xFFFF) != saveGame->lastvirtue)
            saveGame->lastvirtue = (saveGame->moves / 16) & 0xFFFF;
        else
            return;
    }

    /*
     * update the real virtues and handle min/maxing at 1 and 99 along
     * with losing of eighths
     */
    for (v = 0; v < VIRT_MAX; v++) {
        if (saveGame->karma[v] == 0) {               /* already an avatar */
            if (newKarma[v] < 0) {
                saveGame->karma[v] = newKarma[v] + 100;
                if (lostEighthCallback)
                    (*lostEighthCallback)(v);
            }
        } else {
            if (newKarma[v] <= 0)
                newKarma[v] = 1;
            if (newKarma[v] >= 99)
                newKarma[v] = 99;
            saveGame->karma[v] = newKarma[v];
        }
    }
}

int playerAttemptElevation(SaveGame *saveGame, Virtue virtue) {
    if (saveGame->karma[virtue] == 99) {
        saveGame->karma[virtue] = 0;
        return 1;
    } else
        return 0;
}

int playerGetChest(SaveGame *saveGame) {
    int gold = (rand() % 50) + (rand() % 8) + 10;
    saveGame->gold += gold;

    return gold;
}

int playerDonate(SaveGame *saveGame, int quantity) {
    if (quantity > saveGame->gold)
        return 0;

    saveGame->gold -= quantity;
    playerAdjustKarma(saveGame, KA_GAVE_TO_BEGGAR);
    return 1;
}

int playerCanPersonJoin(SaveGame *saveGame, const char *name, Virtue *v) {
    int i;

    if (!name)
        return 0;

    for (i = 1; i < 8; i++) {
        if (strcmp(saveGame->players[i].name, name) == 0) {
            if (v)
                *v = (Virtue) saveGame->players[i].klass;
            return 1;
        }
    }
    return 0;
}

int playerIsPersonJoined(SaveGame *saveGame, const char *name) {
    int i;

    if (!name)
        return 0;
    
    for (i = 1; i < saveGame->members; i++) {
        if (strcmp(saveGame->players[i].name, name) == 0)
            return 1;
    }
    return 0;
}

int playerJoin(SaveGame *saveGame, const char *name) {
    int i;
    SaveGamePlayerRecord tmp;

    for (i = saveGame->members; i < 8; i++) {
        if (strcmp(saveGame->players[i].name, name) == 0) {

            /* ensure character has enough karma */
            if (saveGame->karma[saveGame->players[i].klass] < 40)
                return 0;

            tmp = saveGame->players[saveGame->members];
            saveGame->players[saveGame->members] = saveGame->players[i];
            saveGame->players[i] = tmp;
            saveGame->members++;
            return 1;
        }
    }

    return 0;
}

void playerEndTurn(SaveGame *saveGame) {
    int i;

    saveGame->moves++;
    saveGame->food -= saveGame->members;
    if (saveGame->food < 0) {
        /* FIXME: handle starving */
        saveGame->food = 0;
    }

    for (i = 0; i < saveGame->members; i++) {
        switch (saveGame->players[i].status) {
        case STAT_SLEEPING:
            if (rand() % 5 == 0)
                saveGame->players[i].status = STAT_GOOD;
            break;

        case STAT_POISONED:
            playerApplyDamage(&saveGame->players[i], 2);
            break;

        default:
            break;
        }

        /* regenerate magic points */
        if ((saveGame->players[i].status == STAT_GOOD || 
             saveGame->players[i].status == STAT_POISONED) &&
            saveGame->players[i].mp < playerGetMaxMp(&(saveGame->players[i])))
            saveGame->players[i].mp++;
    }
}

void playerApplyEffect(SaveGame *saveGame, TileEffect effect) {
    int i;

    for (i = 0; i < saveGame->members; i++) {

        if (saveGame->players[i].status == STAT_DEAD)
            continue;

        switch (effect) {
        case EFFECT_NONE:
            break;
        case EFFECT_FIRE:
            break;
        case EFFECT_SLEEP:
            if (rand() % 5 == 0)
                saveGame->players[i].status = STAT_SLEEPING;
            break;
        case EFFECT_POISON:
            if (rand() % 5 == 0)
                saveGame->players[i].status = STAT_POISONED;
            break;
        default:
            assert(0);
        }
    }

}

/**
 * Whether or not the party can make an action.
 */
int playerPartyImmobilized(const SaveGame *saveGame) {
    int i, immobile = 1;

    for (i = 0; i < saveGame->members; i++) {
        if (saveGame->players[i].status == STAT_GOOD ||
            saveGame->players[i].status == STAT_POISONED) {
            immobile = 0;
        }
    }

    return immobile;
}

/**
 * Whether or not all the party members are dead.
 */
int playerPartyDead(const SaveGame *saveGame) {
    int i, dead = 1;

    for (i = 0; i < saveGame->members; i++) {
        if (saveGame->players[i].status != STAT_DEAD) {
            dead = 0;
        }
    }

    return dead;
}

/**
 * Applies a sleep spell to the party.
 */
void playerApplySleepSpell(SaveGame *saveGame) {
    int i;

    for (i = 0; i < saveGame->members; i++) {
        if (saveGame->players[i].status == STAT_GOOD &&
            (rand() % 2) == 0)
            saveGame->players[i].status = STAT_SLEEPING;
    }
}

int playerHeal(SaveGame *saveGame, HealType type, int player) {
    if (player >= saveGame->members)
        return 0;

    switch(type) {

    case HT_NONE:
        return 1;

    case HT_CURE:
        if (saveGame->players[player].status != STAT_POISONED)
            return 0;
        saveGame->players[player].status = STAT_GOOD;
        return 1;

    case HT_HEAL:
        if (saveGame->players[player].status == STAT_DEAD ||
            saveGame->players[player].hp == saveGame->players[player].hpMax)
            return 0;
        saveGame->players[player].hp = saveGame->players[player].hpMax;
        return 1;

    case HT_RESURRECT:
        if (saveGame->players[player].status != STAT_DEAD)
            return 0;
        saveGame->players[player].status = STAT_GOOD;
        saveGame->players[player].hp = STAT_GOOD;
        return 1;
    }

    return 0;
}

void playerReviveParty(SaveGame *saveGame) {
    int i;

    for (i = 0; i < saveGame->members; i++) {
        saveGame->players[i].status = STAT_GOOD;
        saveGame->players[i].hp = saveGame->players[i].hpMax;
    }

    saveGame->food = 20099;
    saveGame->gold = 200;
    saveGame->transport = AVATAR_TILE;
}

/**
 * Returns the maximum number of items of a given price the player can
 * afford.
 */
int playerCanAfford(SaveGame *saveGame, int price) {
    return saveGame->gold / price;
}

/**
 * Attempt to purchase a given quantity of an item for a specified
 * price.  INV_NONE can be used to indicate a service is being
 * purchased instead of an item.  If successful, the inventory will be
 * updated and 1 returned.  Zero is returned on failure.
 */
int playerPurchase(SaveGame *saveGame, InventoryItem item, int type, int quantity, int price) {

    if (price > saveGame->gold)
        return 0;
        
    saveGame->gold -= price;

    switch (item) {
    case INV_NONE:
        /* nothing */
        break;
    case INV_WEAPON:
        saveGame->weapons[type] += quantity;
        break;
    case INV_ARMOR:
        saveGame->armor[type] += quantity;
        break;
    case INV_FOOD:
        saveGame->food += quantity * 100;
        break;
    case INV_REAGENT:
        saveGame->reagents[type] += quantity;
        break;
    case INV_GUILDITEM:
        if (type == 0)
            saveGame->torches += quantity;
        else if (type == 1)
            saveGame->gems += quantity;
        else if (type == 2)
            saveGame->keys += quantity;
        else if (type == 3)
            saveGame->sextants += quantity;
        break;
    case INV_HORSE:
        saveGame->transport = tileGetHorseBase();
        break;
    }

    return 1;
}

/**
 * Attempt to sell a given quantity of an item for a specified price.
 * If successful, the inventory will be updated and 1 returned.  Zero
 * is returned on failure.
 */
int playerSell(SaveGame *saveGame, InventoryItem item, int type, int quantity, int price) {

    switch (item) {
    case INV_NONE:
        assert(0);              /* shouldn't happen */
        break;
    case INV_WEAPON:
        if (saveGame->weapons[type] < quantity)
            return 0;
        saveGame->weapons[type] -= quantity;
        break;
    case INV_ARMOR:
        if (saveGame->armor[type] < quantity)
            return 0;
        saveGame->armor[type] -= quantity;
        break;
    default:
        return 0;
    }

    saveGame->gold += price;

    return 1;
}

/**
 * Determine whether a players attack hits or not.
 */
int playerAttackHit(const SaveGamePlayerRecord *player) {
    if (player->dex >= 40)
        return 1;

    if ((player->dex + 128) >= (rand() & 0xff))
        return 1;
    else
        return 0;
}

/**
 * Calculate damage for an attack.
 */
int playerGetDamage(const SaveGamePlayerRecord *player) {
    static int weaponDamage[] = {
	8, 16,                  /* hands, staff */
	24, 32,                 /* dagger, sling */
	40, 48,                 /* mace, axe */
	64, 40,                 /* sword, bow */
	56, 64,                 /* crossbow, oil */
	96, 96,                 /* halberd, magic axe */
	128, 80,                /* magic sword, magic bow */
	160, 255,               /* magic wand, mystic sword */
    };
    int maxDamage;

    maxDamage = weaponDamage[player->weapon];
    maxDamage += player->str;
    if (maxDamage > 255)
        maxDamage = 255;
    
    return rand() % maxDamage;
}

/**
 * Determine whether a player is hit by a melee attack.
 */
int playerIsHitByAttack(const SaveGamePlayerRecord *player) {
    static int armorVal[] = {
        96, 128,                /* skin, cloth */
        144, 160,               /* leather, chain */
        176, 192,               /* plate, magic chain */
        208, 248                /* magic plate, mystic robes */
    };

    return (rand() % 256) > armorVal[player->armor];
}
