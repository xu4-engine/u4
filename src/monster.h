/*
 * $Id$
 */

#ifndef MONSTER_H
#define MONSTER_H

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
    MATTR_STEALFOOD = 0x01,
    MATTR_STEALGOLD = 0x02,
    MATTR_CASTS_SLEEP = 0x04,
    MATTR_UNDEAD    = 0x08,
    MATTR_GOOD      = 0x10,
    MATTR_WATER     = 0x20,
    MATTR_STATIONARY = 0x40,
    MATTR_NONATTACKABLE = 0x80,
    MATTR_NEGATE = 0x100
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
    unsigned char tile;
    unsigned char leader;
    const char *name;
    unsigned short level;
    MonsterAttrib mattr;
} Monster;

const Monster *monsterForTile(unsigned char tile);
int monsterIsEvil(const Monster *monster);
int monsterGetXp(const Monster *monster);
int monsterCastSleep(const Monster *monster);
int monsterGetDamage(const Monster *monster);
const Monster *monsterRandomForTile(unsigned char tile);
int monsterGetInitialHp(const Monster *monster);
MonsterStatus monsterGetStatus(const Monster *monster, int hp);

#endif
