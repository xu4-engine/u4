/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "monster.h"
#include "ttype.h"

#define UNKNOWN 0

static const Monster monsters[] = {
    { HORSE1_TILE,      UNKNOWN,       "Horse",        9 },
    { HORSE2_TILE,      UNKNOWN,       "Horse",        9 },

    { MAGE_TILE,        UNKNOWN,       "Mage",         8 },
    { BARD_TILE,        UNKNOWN,       "Bard",         9 },
    { FIGHTER_TILE,     UNKNOWN,       "Fighter",      7 },
    { DRUID_TILE,       UNKNOWN,       "Druid",        10 },
    { TINKER_TILE,      UNKNOWN,       "Tinker",       9 },
    { PALADIN_TILE,     UNKNOWN,       "Paladin",      4 },
    { RANGER_TILE,      UNKNOWN,       "Ranger",       3 },
    { SHEPHERD_TILE,    UNKNOWN,       "Shepherd",     9 },

    { GUARD_TILE,       UNKNOWN,       "Guard",        13 },
    { VILLAGER_TILE,    UNKNOWN,       "Villager",     13 },
    { SINGINGBARD_TILE, UNKNOWN,       "Bard",         9 },
    { JESTER_TILE,      UNKNOWN,       "Jester",       9 },
    { BEGGAR_TILE,      UNKNOWN,       "Beggar",       13 },
    { CHILD_TILE,       UNKNOWN,       "Child",        10 },
    { BULL_TILE,        UNKNOWN,       "Bull",         11 },
    { LORDBRITISH_TILE, UNKNOWN,       "Lord British", UNKNOWN },

    { PIRATE_TILE,      UNKNOWN,       "Pirate Ship",  UNKNOWN },
    { NIXIE_TILE,       SEAHORSE_TILE, "Nixie",        5 },
    { GIANT_SQUID_TILE, UNKNOWN,       "Giant Squid",  9 },
    { SEA_SERPENT_TILE, UNKNOWN,       "Sea Serpent",  9 },
    { SEAHORSE_TILE,    UNKNOWN,       "Seahorse",     7 },
    { WHIRLPOOL_TILE,   UNKNOWN,       "Whirlpool",    UNKNOWN },
    { STORM_TILE,       UNKNOWN,       "Storm",        UNKNOWN },
    { RAT_TILE,         UNKNOWN,       "Rat",          4 },
    { BAT_TILE,         UNKNOWN,       "Bat",          4 },
    { GIANT_SPIDER_TILE, UNKNOWN,      "Giant Spider", 5 },
    { GHOST_TILE,       UNKNOWN,       "Ghost",        6 },
    { SLIME_TILE,       UNKNOWN,       "Slime",        4 },
    { TROLL_TILE,       ETTIN_TILE,    "Troll",        7 },
    { GREMLIN_TILE,     UNKNOWN,       "Gremlin",      4 },
    { MIMIC_TILE,       UNKNOWN,       "Mimic",        13 },
    { REAPER_TILE,      UNKNOWN,       "Reaper",       16 },
    { INSECT_SWARM_TILE, UNKNOWN,      "Insect Swarm", 4 },
    { GAZER_TILE,       UNKNOWN,       "Gazer",        16 },
    { PHANTOM_TILE,     UNKNOWN,       "Phantom",      9 },
    { ORC_TILE,         TROLL_TILE,    "Orc",          6 },
    { SKELETON_TILE,    EVILMAGE_TILE, "Skeleton",     4 },
    { ROGUE_TILE,       UNKNOWN,       "Rogue",        6 },
    { PYTHON_TILE,      UNKNOWN,       "Python",       4 },
    { ETTIN_TILE,       UNKNOWN,       "Ettin",        8 },
    { HEADLESS_TILE,    GAZER_TILE,    "Headless",     5 },
    { CYCLOPS_TILE,     UNKNOWN,       "Cyclops",      9 },
    { WISP_TILE,        UNKNOWN,       "Wisp",         5 },
    { EVILMAGE_TILE,    UNKNOWN,       "Mage",         12 },
    { LICH_TILE,        UNKNOWN,       "Lich",         13 },
    { LAVA_LIZARD_TILE, UNKNOWN,       "Lava Lizard",  7 },
    { ZORN_TILE,        UNKNOWN,       "Zorn",         16 },
    { DAEMON_TILE,      UNKNOWN,       "Daemon",       8 },
    { HYDRA_TILE,       UNKNOWN,       "Hydra",        14 },
    { DRAGON_TILE,      UNKNOWN,       "Dragon",       15 },
    { BALRON_TILE,      UNKNOWN,       "Balron",       16 }
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
    return !(monster->tile == SEAHORSE_TILE ||
             monster->tile == RAT_TILE ||
             monster->tile == BAT_TILE ||
             monster->tile == GIANT_SPIDER_TILE ||
             monster->tile == INSECT_SWARM_TILE ||
             monster->tile == PYTHON_TILE);
}
