/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "monster.h"

/* FIXME: should monsterSpecialAction() and monsterSpecialEffect() be placed elsewhere
   to make monster.c as independent as possible? */

#include "game.h"	/* required by monsterSpecial functions */
#include "player.h"	/* required by monsterSpecial functions */
#include "context.h"
#include "savegame.h"
#include "ttype.h"

#define UNKNOWN 0

static const Monster monsters[] = {
    { HORSE1_TILE,      UNKNOWN,       "Horse",        9,  0, MATTR_GOOD | MATTR_NOATTACK },
    { HORSE2_TILE,      UNKNOWN,       "Horse",        9,  0, MATTR_GOOD | MATTR_NOATTACK },
 
    { MAGE_TILE,        UNKNOWN,       "Mage",         8,  MAGICFLASH_TILE, MATTR_GOOD | MATTR_NOATTACK },
    { BARD_TILE,        UNKNOWN,       "Bard",         9,  0, MATTR_GOOD | MATTR_NOATTACK },
    { FIGHTER_TILE,     UNKNOWN,       "Fighter",      7,  0, MATTR_GOOD | MATTR_NOATTACK },
    { DRUID_TILE,       UNKNOWN,       "Druid",        10, 0, MATTR_GOOD | MATTR_NOATTACK },
    { TINKER_TILE,      UNKNOWN,       "Tinker",       9,  0, MATTR_GOOD | MATTR_NOATTACK },
    { PALADIN_TILE,     UNKNOWN,       "Paladin",      4,  0, MATTR_GOOD | MATTR_NOATTACK },
    { RANGER_TILE,      UNKNOWN,       "Ranger",       3,  0, MATTR_GOOD | MATTR_NOATTACK },
    { SHEPHERD_TILE,    UNKNOWN,       "Shepherd",     9,  0, MATTR_GOOD | MATTR_NOATTACK },

    { GUARD_TILE,       UNKNOWN,       "Guard",        13, 0, MATTR_GOOD | MATTR_NOATTACK },
    { VILLAGER_TILE,    UNKNOWN,       "Villager",     13, 0, MATTR_GOOD | MATTR_NOATTACK },
    { SINGINGBARD_TILE, UNKNOWN,       "Bard",         9,  0, MATTR_GOOD | MATTR_NOATTACK },
    { JESTER_TILE,      UNKNOWN,       "Jester",       9,  0, MATTR_GOOD | MATTR_NOATTACK },
    { BEGGAR_TILE,      UNKNOWN,       "Beggar",       13, 0, MATTR_GOOD | MATTR_NOATTACK },
    { CHILD_TILE,       UNKNOWN,       "Child",        10, 0, MATTR_GOOD | MATTR_NOATTACK },
    { BULL_TILE,        UNKNOWN,       "Bull",         11, 0, MATTR_GOOD },
    { LORDBRITISH_TILE, UNKNOWN,       "Lord British", 16, MAGICFLASH_TILE, MATTR_GOOD | MATTR_NOATTACK },

    { PIRATE_TILE,      UNKNOWN,       "Pirate Ship",  16, 0,      MATTR_WATER },
    { NIXIE_TILE,       SEAHORSE_TILE, "Nixie",        4,  MISSFLASH_TILE, MATTR_WATER },
    { GIANT_SQUID_TILE, SEA_SERPENT_TILE, "Giant Squid",  6, LIGHTNINGFIELD_TILE, MATTR_WATER },
    { SEA_SERPENT_TILE, GIANT_SQUID_TILE, "Sea Serpent",  8, FIREFIELD_TILE, MATTR_WATER },
    { SEAHORSE_TILE,    NIXIE_TILE,    "Seahorse",     6,  MAGICFLASH_TILE, MATTR_WATER | MATTR_GOOD },
    { WHIRLPOOL_TILE,   WHIRLPOOL_TILE, "Whirlpool",   16, 0,      MATTR_WATER | MATTR_NONATTACKABLE | MATTR_WANDERS | MATTR_NOATTACK },
    { STORM_TILE,       STORM_TILE,    "Storm",        16, 0,      MATTR_FLIES | MATTR_NONATTACKABLE | MATTR_WANDERS | MATTR_NOATTACK },
    { RAT_TILE,         SKELETON_TILE, "Rat",          3,  0,      MATTR_GOOD | MATTR_WANDERS },
    { BAT_TILE,         LAVA_LIZARD_TILE, "Bat",       3,  0,      MATTR_FLIES | MATTR_GOOD | MATTR_WANDERS },
    { GIANT_SPIDER_TILE, RAT_TILE,     "Giant Spider", 4,  POISONFIELD_TILE, MATTR_GOOD | MATTR_WANDERS },
    { GHOST_TILE,       LICH_TILE,     "Ghost",        5,  0,      MATTR_UNDEAD },
    { SLIME_TILE,       SLIME_TILE,    "Slime",        3,  0,      0 },
    { TROLL_TILE,       ETTIN_TILE,    "Troll",        6,  MISSFLASH_TILE, 0 },
    { GREMLIN_TILE,     GREMLIN_TILE,  "Gremlin",      3,  0,      MATTR_STEALFOOD },
    { MIMIC_TILE,       MIMIC_TILE,    "Mimic",        12, POISONFIELD_TILE, MATTR_STATIONARY | MATTR_CAMOUFLAGE },
    { REAPER_TILE,      REAPER_TILE,   "Reaper",       16, 0xFF,   MATTR_CASTS_SLEEP | MATTR_STATIONARY },
    { INSECT_SWARM_TILE, RAT_TILE,     "Insect Swarm", 3,  0,      MATTR_GOOD | MATTR_WANDERS },
    { GAZER_TILE,       PHANTOM_TILE,  "Gazer",        15, SLEEPFIELD_TILE, 0 },
    { PHANTOM_TILE,     GHOST_TILE,    "Phantom",      8,  0,      MATTR_UNDEAD },
    { ORC_TILE,         TROLL_TILE,    "Orc",          5,  0,      0 },
    { SKELETON_TILE,    EVILMAGE_TILE, "Skeleton",     3,  0,      MATTR_UNDEAD },
    { ROGUE_TILE,       ROGUE_TILE,    "Rogue",        5,  0,      MATTR_STEALGOLD },
    { PYTHON_TILE,      RAT_TILE,      "Python",       3,  POISONFIELD_TILE, MATTR_GOOD | MATTR_WANDERS },
    { ETTIN_TILE,       DAEMON_TILE,   "Ettin",        7,  BOULDER_TILE, 0 },
    { HEADLESS_TILE,    GAZER_TILE,    "Headless",     4,  0,      0 },
    { CYCLOPS_TILE,     ZORN_TILE,     "Cyclops",      8,  BOULDER_TILE, 0 },
    { WISP_TILE,        PHANTOM_TILE,  "Wisp",         4,  0,      MATTR_TELEPORT },
    { EVILMAGE_TILE,    DAEMON_TILE,   "Mage",         11, MAGICFLASH_TILE, 0 },
    { LICH_TILE,        DAEMON_TILE,   "Lich",         12, MAGICFLASH_TILE, MATTR_UNDEAD },
    { LAVA_LIZARD_TILE, HYDRA_TILE,    "Lava Lizard",  5,  LAVA_TILE, MATTR_FIRERESISTANT },
    { ZORN_TILE,        GAZER_TILE,    "Zorn",         15, 0,      MATTR_NEGATE },
    { DAEMON_TILE,      BALRON_TILE,   "Daemon",       7,  MAGICFLASH_TILE, MATTR_FIRERESISTANT },
    { HYDRA_TILE,       DRAGON_TILE,   "Hydra",        13, FIREFIELD_TILE, MATTR_FIRERESISTANT },
    { DRAGON_TILE,      BALRON_TILE,   "Dragon",       14, FIREFIELD_TILE, MATTR_FLIES | MATTR_FIRERESISTANT },
    { BALRON_TILE,      BALRON_TILE,   "Balron",       16, 0xFF,   MATTR_CASTS_SLEEP | MATTR_FIRERESISTANT }
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

const Monster *monsterRandomForTile(unsigned char tile) {
    unsigned char mtile;
    int era;
    
    if (tileIsSailable(tile)) {
        mtile = ((rand() % 8) << 1) + PIRATE_TILE;
        return monsterForTile(mtile);
    }
    else if (tileIsSwimable(tile)) {
        mtile = ((rand() % 7) << 1) + NIXIE_TILE;        
        return monsterForTile(mtile);
    }

    if (!tileIsMonsterWalkable(tile))
        return 0;

    if (c->saveGame->moves > 100000)
        era = 0x0f;
    else if (c->saveGame->moves > 20000)
        era = 0x07;
    else
        era = 0x03;

    mtile = ((era & rand() & rand()) << 2) + ORC_TILE;    

    return monsterForTile(mtile);
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

int monsterSpecialAction(const Monster *monster) {
    switch(monster->tile) {
    case PIRATE_TILE:
        /* fire cannon */
        break;

    case GIANT_SQUID_TILE: /* ranged */
    case SEA_SERPENT_TILE: /* ranged */
    case DRAGON_TILE: /* ranged */
    
        /* FIXME: add ranged monster's behavior here */

        /* Other ranged monsters here too, whoever you are! */

    default: break;
    }

    return 0;
}

void monsterSpecialEffect(Object *obj) {
    Object *o;
    Monster *m;

    switch(obj->tile) {
    case STORM_TILE:
    case STORM_TILE+1:
        {
            if (obj->x == c->location->x &&
                    obj->y == c->location->y &&
                    obj->z == c->location->z) {

                if (tileIsShip(c->saveGame->transport)) {
                    /* FIXME: Check actual damage from u4dos
                       Screen should shake here */
                    c->saveGame->shiphull -= (11 + rand()%20);
                    if (c->saveGame->shiphull > 99)
                    {
                        c->saveGame->shiphull = 0;
                        gameCheckHullIntegrity();
                    }
                }
                else {
                    /* FIXME: formula for twister damage is guesstimated from u4dos */
                    int i;

                    for (i = 0; i < c->saveGame->members; i++)
                        playerApplyDamage(&c->saveGame->players[i], rand() % 75);
                }
                break;
            }

            /* See if the storm is on top of any objects and destroy them! */
            for (o = c->location->map->objects; o; o = o->next) {                
                if (o != obj && 
                    o->x == obj->x &&
                    o->y == obj->y &&
                    o->z == obj->z) {
                    /* Converged with an object, destroy the object! */
                    mapRemoveObject(c->location->map, o);
                    break;
                }
            }
        }      
        break;

    case WHIRLPOOL_TILE:
    case WHIRLPOOL_TILE+1:
        {
            if (obj->x == c->location->x &&
                obj->y == c->location->y &&
                obj->z == c->location->z && tileIsShip(c->saveGame->transport)) {
                
                /* FIXME: Screen should shake here */
                c->saveGame->shiphull -= 10;
                gameCheckHullIntegrity();

                c->location->x = 127;
                c->location->y = 78;

                mapRemoveObject(c->location->map, obj);
                break;
            }
            
            /* See if the whirlpool is on top of any objects and destroy them! */
            for (o = c->location->map->objects; o; o = o->next) {
                if (o != obj && 
                    o->x == obj->x &&
                    o->y == obj->y &&
                    o->z == obj->z) {                    

                    m = monsterForTile(o->tile);
                    
                    /* Make sure the object isn't a flying monster or object */
                    if (!tileIsBalloon(o->tile) && (!m || !(m->mattr & MATTR_FLIES)))
                        mapRemoveObject(c->location->map, o); /* Destroy the object it met with */
                    break;
                }
            }            
        }

    default: break;
    }
}
