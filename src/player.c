/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "player.h"

/**
 * Determine the highest level a character could have with the number
 * of experience points he has.
 */
int playerGetLevel(const SaveGamePlayerRecord *player) {
    int level = 1;
    int next = 100;

    while (player->xp >= next && level < 8) {
        level++;
        next *= 2;
    }

    return level;
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

    default:
        assert(0);              /* shouldn't happen */
    }
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

    default:
        assert(0);              /* shoudn't happen */
    }
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

    assert(0);
    return 0;
}
