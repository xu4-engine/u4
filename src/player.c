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

int playerCanWear(const SaveGamePlayerRecord *player, ArmorType armor) {
    return 1;
}

int playerCanReady(const SaveGamePlayerRecord *player, WeaponType weapon) {
    return 1;
}
