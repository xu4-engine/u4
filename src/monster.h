/**
 * $Id$
 */

#ifndef MONSTER_H
#define MONSTER_H

#include "object.h"
#include "movement.h"

#define MAX_MONSTERS 128

/* Monsters on world map */

#define MAX_MONSTERS_ON_MAP 4
#define MAX_MONSTER_DISTANCE 30

/* Monster ids */

#define HORSE1_ID       0
#define HORSE2_ID       1

#define MAGE_ID         2
#define BARD_ID         3
#define FIGHTER_ID      4
#define DRUID_ID        5
#define TINKER_ID       6
#define PALADIN_ID      7
#define RANGER_ID       8
#define SHEPHERD_ID     9

#define GUARD_ID        10
#define VILLAGER_ID     11
#define SINGINGBARD_ID  12
#define JESTER_ID       13
#define BEGGAR_ID       14
#define CHILD_ID        15
#define BULL_ID         16
#define LORDBRITISH_ID  17

#define PIRATE_ID       18
#define NIXIE_ID        19
#define GIANT_SQUID_ID  20
#define SEA_SERPENT_ID  21
#define SEAHORSE_ID     22
#define WHIRLPOOL_ID    23
#define STORM_ID        24
#define RAT_ID          25
#define BAT_ID          26
#define GIANT_SPIDER_ID 27
#define GHOST_ID        28
#define SLIME_ID        29
#define TROLL_ID        30
#define GREMLIN_ID      31
#define MIMIC_ID        32
#define REAPER_ID       33
#define INSECT_SWARM_ID 34
#define GAZER_ID        35
#define PHANTOM_ID      36
#define ORC_ID          37
#define SKELETON_ID     38
#define ROGUE_ID        39
#define PYTHON_ID       40
#define ETTIN_ID        41
#define HEADLESS_ID     42
#define CYCLOPS_ID      43
#define WISP_ID         44
#define EVILMAGE_ID     45
#define LICH_ID         46
#define LAVA_LIZARD_ID  47
#define ZORN_ID         48
#define DAEMON_ID       49
#define HYDRA_ID        50
#define DRAGON_ID       51
#define BALRON_ID       52

/* Monster tiles */
#define HORSE1_TILE 20
#define HORSE2_TILE 21

#define MAGE_TILE 32
#define BARD_TILE 34
#define FIGHTER_TILE 36
#define DRUID_TILE 38
#define TINKER_TILE 40
#define PALADIN_TILE 42
#define RANGER_TILE 44
#define SHEPHERD_TILE 46

#define GUARD_TILE 80
#define VILLAGER_TILE 82
#define SINGINGBARD_TILE 84
#define JESTER_TILE 86
#define BEGGAR_TILE 88
#define CHILD_TILE 90
#define BULL_TILE 92
#define LORDBRITISH_TILE 94

#define PIRATE_TILE 128
#define NIXIE_TILE 132
#define GIANT_SQUID_TILE 134
#define SEA_SERPENT_TILE 136
#define SEAHORSE_TILE 138
#define WHIRLPOOL_TILE 140
#define STORM_TILE 142
#define RAT_TILE 144
#define BAT_TILE 148
#define GIANT_SPIDER_TILE 152
#define GHOST_TILE 156
#define SLIME_TILE 160
#define TROLL_TILE 164
#define GREMLIN_TILE 168
#define MIMIC_TILE 172
#define REAPER_TILE 176
#define INSECT_SWARM_TILE 180
#define GAZER_TILE 184
#define PHANTOM_TILE 188
#define ORC_TILE 192
#define SKELETON_TILE 196
#define ROGUE_TILE 200
#define PYTHON_TILE 204
#define ETTIN_TILE 208
#define HEADLESS_TILE 212
#define CYCLOPS_TILE 216
#define WISP_TILE 220
#define EVILMAGE_TILE 224
#define LICH_TILE 228
#define LAVA_LIZARD_TILE 232
#define ZORN_TILE 236
#define DAEMON_TILE 240
#define HYDRA_TILE 244
#define DRAGON_TILE 248
#define BALRON_TILE 252

typedef enum {
    MATTR_STEALFOOD   = 0x01,
    MATTR_STEALGOLD   = 0x02,
    MATTR_CASTS_SLEEP = 0x04,
    MATTR_UNDEAD      = 0x08,
    MATTR_GOOD        = 0x10,
    MATTR_WATER       = 0x20,
    MATTR_STATIONARY  = 0x40,
    MATTR_NONATTACKABLE = 0x80,
    MATTR_NEGATE      = 0x100,
    MATTR_TELEPORT    = 0x200,
    MATTR_FIRERESISTANT = 0x400,
    MATTR_CAMOUFLAGE  = 0x800,
    MATTR_WANDERS     = 0x1000,
    MATTR_NOATTACK    = 0x2000,
    MATTR_FLIES       = 0x4000
} MonsterAttrib;

typedef enum {
    MSTAT_DEAD,
    MSTAT_FLEEING,
    MSTAT_CRITICAL,
    MSTAT_HEAVILYWOUNDED,
    MSTAT_LIGHTLYWOUNDED,
    MSTAT_BARELYWOUNDED
} MonsterStatus;

typedef struct _Monster {
    const char *name;
    unsigned short id;
    unsigned char tile;
    unsigned char leader;
    unsigned char basehp;
    unsigned short level;
    unsigned char ranged;
    unsigned char rangedhittile;
    unsigned char rangedmisstile;
    MonsterAttrib mattr;
    SlowedType slowedType;
    unsigned char encounterSize;
} Monster;

const Monster *monsterForTile(unsigned char tile);

int monsterIsGood(const Monster *monster);
int monsterIsEvil(const Monster *monster);
int monsterIsUndead(const Monster *monster);
int monsterIsAquatic(const Monster *monster);
int monsterFlies(const Monster *monster);
int monsterTeleports(const Monster *monster);
int monsterIsAttackable(const Monster *monster);
int monsterWillAttack(const Monster *monster);
int monsterStealsGold(const Monster *monster);
int monsterStealsFood(const Monster *monster);
int monsterGetXp(const Monster *monster);
int monsterCastSleep(const Monster *monster);
int monsterGetDamage(const Monster *monster);
const Monster *monsterRandomForTile(unsigned char tile);
int monsterGetInitialHp(const Monster *monster);
MonsterStatus monsterGetStatus(const Monster *monster, int hp);
int monsterSpecialAction(Object *obj);
void monsterSpecialEffect(Object *obj);
const Monster *monsterById(unsigned short id);

#endif
