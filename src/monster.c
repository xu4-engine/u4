/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "monster.h"
#include "context.h"
#include "savegame.h"
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
    { LORDBRITISH_TILE, UNKNOWN,       "Lord British", 16, 0 },

    { PIRATE_TILE,      UNKNOWN,       "Pirate Ship",  16, MATTR_WATER },
    { NIXIE_TILE,       SEAHORSE_TILE, "Nixie",        4,  MATTR_WATER },
    { GIANT_SQUID_TILE, SEA_SERPENT_TILE, "Giant Squid",  6, MATTR_WATER },
    { SEA_SERPENT_TILE, GIANT_SQUID_TILE, "Sea Serpent",  8, MATTR_WATER },
    { SEAHORSE_TILE,    NIXIE_TILE,    "Seahorse",     6,  MATTR_WATER | MATTR_GOOD },
    { WHIRLPOOL_TILE,   WHIRLPOOL_TILE, "Whirlpool",   16, MATTR_WATER },
    { STORM_TILE,       STORM_TILE,    "Storm",        16, MATTR_WATER },
    { RAT_TILE,         SKELETON_TILE, "Rat",          3,  MATTR_GOOD },
    { BAT_TILE,         LAVA_LIZARD_TILE, "Bat",       3,  MATTR_GOOD },
    { GIANT_SPIDER_TILE, RAT_TILE,     "Giant Spider", 4,  MATTR_GOOD },
    { GHOST_TILE,       LICH_TILE,     "Ghost",        5,  MATTR_UNDEAD },
    { SLIME_TILE,       SLIME_TILE,    "Slime",        3,  0 },
    { TROLL_TILE,       ETTIN_TILE,    "Troll",        6,  0 },
    { GREMLIN_TILE,     GREMLIN_TILE,  "Gremlin",      3,  MATTR_STEALFOOD },
    { MIMIC_TILE,       MIMIC_TILE,    "Mimic",        12, 0 },
    { REAPER_TILE,      REAPER_TILE,   "Reaper",       16, MATTR_CASTS_SLEEP },
    { INSECT_SWARM_TILE, RAT_TILE,     "Insect Swarm", 3,  MATTR_GOOD },
    { GAZER_TILE,       PHANTOM_TILE,  "Gazer",        15, 0 },
    { PHANTOM_TILE,     GHOST_TILE,    "Phantom",      8,  MATTR_UNDEAD },
    { ORC_TILE,         TROLL_TILE,    "Orc",          5,  0 },
    { SKELETON_TILE,    EVILMAGE_TILE, "Skeleton",     3,  MATTR_UNDEAD },
    { ROGUE_TILE,       ROGUE_TILE,    "Rogue",        5,  MATTR_STEALGOLD },
    { PYTHON_TILE,      RAT_TILE,      "Python",       3,  MATTR_GOOD },
    { ETTIN_TILE,       DAEMON_TILE,   "Ettin",        7,  0 },
    { HEADLESS_TILE,    GAZER_TILE,    "Headless",     4,  0 },
    { CYCLOPS_TILE,     ZORN_TILE,     "Cyclops",      8,  0 },
    { WISP_TILE,        PHANTOM_TILE,  "Wisp",         4,  0 },
    { EVILMAGE_TILE,    DAEMON_TILE,   "Mage",         11, 0 },
    { LICH_TILE,        DAEMON_TILE,   "Lich",         12, MATTR_UNDEAD },
    { LAVA_LIZARD_TILE, HYDRA_TILE,    "Lava Lizard",  5,  0 },
    { ZORN_TILE,        GAZER_TILE,    "Zorn",         15, 0 },
    { DAEMON_TILE,      BALRON_TILE,   "Daemon",       7,  0 },
    { HYDRA_TILE,       DRAGON_TILE,   "Hydra",        13, 0 },
    { DRAGON_TILE,      BALRON_TILE,   "Dragon",       14, 0 },
    { BALRON_TILE,      BALRON_TILE,   "Balron",       16, MATTR_CASTS_SLEEP }
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
            if (tileIsPirateShip(tile))
                n = 4;
            else
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

unsigned char monsterRandomForTile(unsigned char tile) {
    unsigned char monster;
    int era;
    
    if (tileIsSailable(tile)) {
        monster = ((rand() % 8) << 1) + PIRATE_TILE;
        monster = monsterForTile(monster)->tile;
        return monster;
    }

    if (!tileIsWalkable(tile))
        return 0;

    if (c->saveGame->moves > 100000)
        era = 0x0f;
    else if (c->saveGame->moves > 20000)
        era = 0x07;
    else
        era = 0x03;

    monster = ((era & rand() & rand()) << 2) + ORC_TILE;

    return monster;
}

int monsterGetInitialHp(const Monster *monster) {
    int basehp, hp;

    basehp = monster->level == 16 ? 255 : (monster->level << 4);
    hp = (basehp % rand()) + (basehp / 2);

    return hp;
}

MonsterStatus monsterGetStatus(const Monster *monster, int hp) {
    int basehp, heavy_threshold, light_threshold, crit_threshold;

    basehp = monster->level == 16 ? 255 : (monster->level << 4);
    crit_threshold = basehp / 4;
    heavy_threshold = basehp / 2;
    light_threshold = crit_threshold + heavy_threshold;

    if (hp <= 0)
        return MSTAT_DEAD;
    else if (hp < 24)
        return MSTAT_FLEEING;
    else if (hp < crit_threshold)
        return MSTAT_CRITICAL;
    else if (hp < heavy_threshold)
        return MSTAT_HEAVILYWOUNDED;
    else if (hp < light_threshold)
        return MSTAT_LIGHTLYWOUNDED;
    else
        return MSTAT_BARELYWOUNDED;
}
