/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "player.h"
#include "map.h"

/**
 * Applies damage to a player, and changes status to dead if hit
 * points drop to zero or below.
 */
void playerApplyDamage(SaveGamePlayerRecord *player, int damage) {
    int newHp = player->hp;

    if (newHp == 0 || player->status == STAT_DEAD)
        return;

    newHp -= damage;

    if (newHp <= 0) {
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
 * Adjusts the avatar's karma level for the given action.  Returns the
 * number of eighths of avatarhood the player has lost, or zero if
 * none.
 */
int playerAdjustKarma(SaveGame *saveGame, KarmaAction action) {
    int eighths = 0;
    int v, newKarma[VIRT_MAX];

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
        newKarma[VIRT_COMPASSION] += 2;
        newKarma[VIRT_HONOR] += 3;
        break;
    case KA_BRAGGED:
        newKarma[VIRT_HUMILITY] -= 5;
        break;
    case KA_HUMBLE:
        newKarma[VIRT_HUMILITY] += 10;
        break;
    case KA_HAWKWIND:
    case KA_MEDITATION:
        newKarma[VIRT_SPIRITUALITY] += 3;
        break;
    case KA_BAD_MANTRA:
        newKarma[VIRT_SPIRITUALITY] -= 3;
        break;
    case KA_FLED:
        newKarma[VIRT_VALOR] -= 2;
        newKarma[VIRT_SACRIFICE] -= 2;
        break;
    case KA_DONATED_BLOOD:
        newKarma[VIRT_SACRIFICE] += 5;
        break;
    case KA_DIDNT_DONATE_BLOOD:
        newKarma[VIRT_SACRIFICE] -= 5;
        break;
    }

    for (v = 0; v < VIRT_MAX; v++) {
        if (saveGame->karma[v] == 0) {               /* already an avatar */
            if (newKarma[v] < 0) {
                saveGame->karma[v] = newKarma[v] + 100;
                eighths++;
            }
        } else {
            if (newKarma[v] <= 0)
                newKarma[v] = 1;
            if (newKarma[v] >= 99)
                newKarma[v] = 99;
            saveGame->karma[v] = newKarma[v];
        }
    }

    return eighths;
}

int playerAttemptElevation(SaveGame *saveGame, Virtue virtue) {
    if (saveGame->karma[virtue] == 99) {
        saveGame->karma[virtue] = 0;
        return 1;
    } else
        return 0;
}

void playerGetChest(SaveGame *saveGame) {
    saveGame->gold += rand() % 32;
    saveGame->gold += rand() % 32;
    saveGame->gold += rand() % 32;
}

int playerDonate(SaveGame *saveGame, int quantity) {
    if (quantity > saveGame->gold)
        return 0;

    saveGame->gold -= quantity;
    playerAdjustKarma(saveGame, KA_GAVE_TO_BEGGAR);
    return 1;
}

void playerJoin(SaveGame *saveGame, const char *name) {
    int i;
    SaveGamePlayerRecord tmp;

    for (i = saveGame->members; i < 8; i++) {
        if (strcmp(saveGame->players[i].name, name) == 0) {
            tmp = saveGame->players[saveGame->members];
            saveGame->players[saveGame->members] = saveGame->players[i];
            saveGame->players[i] = tmp;
            saveGame->members++;
            break;
        }
    }
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

