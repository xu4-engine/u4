/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "monster.h"

#define UNKNOWN 0

/*
    Bard         9
    Beggar      13
    Bull        11
    Child       10
    Druid       10
    Fighter      7
    Guard       13
    Horse        9
    Jester       9
    Mage         8
    Merchant     9
    Paladin      4
    Ranger       3
    Shepherd     9
    Tinker       9
    Villager    13
*/


static const Monster monsters[] = {
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
    { SKELETON_TILE,    MAGE_TILE,     "Skeleton",     4 },
    { ROGUE_TILE,       UNKNOWN,       "Rogue",        6 },
    { PYTHON_TILE,      UNKNOWN,       "Python",       4 },
    { ETTIN_TILE,       UNKNOWN,       "Ettin",        8 },
    { HEADLESS_TILE,    GAZER_TILE,    "Headless",     5 },
    { CYCLOPS_TILE,     UNKNOWN,       "Cyclops",      9 },
    { WISP_TILE,        UNKNOWN,       "Wisp",         5 },
    { MAGE_TILE,        UNKNOWN,       "Mage",         12 },
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
    int i;

    for (i = 0; i < N_MONSTERS - 1; i++) {
        if (tile >= monsters[i].tile && tile < monsters[i+1].tile)
            return &(monsters[i]);
    }
    if (tile >= monsters[N_MONSTERS - 1].tile)
        return &(monsters[N_MONSTERS - 1]);

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
