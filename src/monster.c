/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "monster.h"
#include "context.h"
#include "ttype.h"

#define UNKNOWN 0

static const Monster monsters[] = {
    { HORSE1_TILE,      UNKNOWN,       "Horse",        9,  0 },
    { HORSE2_TILE,      UNKNOWN,       "Horse",        9,  0 },

    { MAGE_TILE,        UNKNOWN,       "Mage",         8,  0 },
    { BARD_TILE,        UNKNOWN,       "Bard",         9,  0 },
    { FIGHTER_TILE,     UNKNOWN,       "Fighter",      7,  0 },
    { DRUID_TILE,       UNKNOWN,       "Druid",        10, 0 },
    { TINKER_TILE,      UNKNOWN,       "Tinker",       9,  0 },
    { PALADIN_TILE,     UNKNOWN,       "Paladin",      4,  0 },
    { RANGER_TILE,      UNKNOWN,       "Ranger",       3,  0 },
    { SHEPHERD_TILE,    UNKNOWN,       "Shepherd",     9,  0 },

    { GUARD_TILE,       UNKNOWN,       "Guard",        13, 0 },
    { VILLAGER_TILE,    UNKNOWN,       "Villager",     13, 0 },
    { SINGINGBARD_TILE, UNKNOWN,       "Bard",         9,  0 },
    { JESTER_TILE,      UNKNOWN,       "Jester",       9,  0 },
    { BEGGAR_TILE,      UNKNOWN,       "Beggar",       13, 0 },
    { CHILD_TILE,       UNKNOWN,       "Child",        10, 0 },
    { BULL_TILE,        UNKNOWN,       "Bull",         11, 0 },
    { LORDBRITISH_TILE, UNKNOWN,       "Lord British", UNKNOWN, 0 },

    { PIRATE_TILE,      UNKNOWN,       "Pirate Ship",  UNKNOWN, 0 },
    { NIXIE_TILE,       SEAHORSE_TILE, "Nixie",        5,  0 },
    { GIANT_SQUID_TILE, UNKNOWN,       "Giant Squid",  9,  0 },
    { SEA_SERPENT_TILE, UNKNOWN,       "Sea Serpent",  9,  0 },
    { SEAHORSE_TILE,    UNKNOWN,       "Seahorse",     7,  MATTR_GOOD },
    { WHIRLPOOL_TILE,   UNKNOWN,       "Whirlpool",    UNKNOWN, 0 },
    { STORM_TILE,       UNKNOWN,       "Storm",        UNKNOWN, 0 },
    { RAT_TILE,         UNKNOWN,       "Rat",          3,  MATTR_GOOD },
    { BAT_TILE,         UNKNOWN,       "Bat",          3,  MATTR_GOOD },
    { GIANT_SPIDER_TILE, UNKNOWN,      "Giant Spider", 4,  MATTR_GOOD },
    { GHOST_TILE,       UNKNOWN,       "Ghost",        5,  MATTR_UNDEAD },
    { SLIME_TILE,       UNKNOWN,       "Slime",        3,  0 },
    { TROLL_TILE,       ETTIN_TILE,    "Troll",        6,  0 },
    { GREMLIN_TILE,     UNKNOWN,       "Gremlin",      3,  MATTR_STEALFOOD },
    { MIMIC_TILE,       UNKNOWN,       "Mimic",        12, 0 },
    { REAPER_TILE,      UNKNOWN,       "Reaper",       16, MATTR_CASTS_SLEEP },
    { INSECT_SWARM_TILE, UNKNOWN,      "Insect Swarm", 3,  MATTR_GOOD },
    { GAZER_TILE,       UNKNOWN,       "Gazer",        15, 0 },
    { PHANTOM_TILE,     UNKNOWN,       "Phantom",      8,  MATTR_UNDEAD },
    { ORC_TILE,         TROLL_TILE,    "Orc",          5,  0 },
    { SKELETON_TILE,    EVILMAGE_TILE, "Skeleton",     3,  MATTR_UNDEAD },
    { ROGUE_TILE,       UNKNOWN,       "Rogue",        5,  MATTR_STEALGOLD },
    { PYTHON_TILE,      UNKNOWN,       "Python",       3,  MATTR_GOOD },
    { ETTIN_TILE,       UNKNOWN,       "Ettin",        7,  0 },
    { HEADLESS_TILE,    GAZER_TILE,    "Headless",     4,  0 },
    { CYCLOPS_TILE,     UNKNOWN,       "Cyclops",      8,  0 },
    { WISP_TILE,        UNKNOWN,       "Wisp",         4,  0 },
    { EVILMAGE_TILE,    UNKNOWN,       "Mage",         11, 0 },
    { LICH_TILE,        UNKNOWN,       "Lich",         12, MATTR_UNDEAD },
    { LAVA_LIZARD_TILE, UNKNOWN,       "Lava Lizard",  5,  0 },
    { ZORN_TILE,        UNKNOWN,       "Zorn",         15, 0 },
    { DAEMON_TILE,      UNKNOWN,       "Daemon",       7,  0 },
    { HYDRA_TILE,       UNKNOWN,       "Hydra",        13, 0 },
    { DRAGON_TILE,      UNKNOWN,       "Dragon",       14, 0 },
    { BALRON_TILE,      UNKNOWN,       "Balron",       16, MATTR_CASTS_SLEEP }
};

#define N_MONSTERS (sizeof(monsters) / sizeof(monsters[0]))

const Monster *monsterForTile(unsigned char tile) {
    int i, n;

    for (i = 0; i < N_MONSTERS; i++) {
        switch (tileGetAnimationStyle(monsters[i].tile)) {
        case ANIM_TWOFRAMES:
            n = 2;
            break;
        case ANIM_FOURFRAMES:
            n = 4;
            break;
        default:
            n = 1;
            break;
        }

        if (tile >= monsters[i].tile && tile < monsters[i].tile + n)
            return &(monsters[i]);
    }

    return NULL;
}

int monsterIsEvil(const Monster *monster) {
    return (monster->mattr & MATTR_GOOD) == 0;
}

int monsterIsUndead(const Monster *monster) {
    return (monster->mattr & MATTR_UNDEAD) != 0;
}

int monsterGetXp(const Monster *monster) {
    if (monster->level == 16)
        return 16;
    else
        return monster->level + 1;
}

int monsterGetDamage(const Monster *monster) {
    int damage, val, x;
    val = (monster->level << 4);
    if (val > 255)
        val = 255;
    x = (rand() % (val >> 2));
    damage = (x >> 4) + ((x >> 2) & 0xfc);
    damage += x % 10;
    return damage;
}

int monsterCastSleep(const Monster *monster) {
    return
        (monster->mattr & MATTR_CASTS_SLEEP) &&
        (c->aura != AURA_NEGATE) &&
        (rand() % 4) == 0;
}
