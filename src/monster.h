/*
 * $Id$
 */

#ifndef MONSTER_H
#define MONSTER_H

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
#define MAGE_TILE 224
#define LICH_TILE 228
#define LAVA_LIZARD_TILE 232
#define ZORN_TILE 236
#define DAEMON_TILE 240
#define HYDRA_TILE 244
#define DRAGON_TILE 248
#define BALRON_TILE 252

typedef struct _Monster {
    unsigned char tile;
    unsigned char leader;
    const char *name;
    unsigned short xp;
} Monster;

const Monster *monsterForTile(unsigned char tile);
int monsterIsEvil(const Monster *monster);

#endif
