/**
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <libxml/xmlmemory.h>

#include "monster.h"

/* FIXME: should monsterSpecialAction() and monsterSpecialEffect() be placed elsewhere
   to make monster.c as independent as possible? */

#include "context.h"
#include "debug.h"
#include "error.h"
#include "event.h"
#include "game.h"	/* required by monsterSpecial functions */
#include "location.h"
#include "player.h"	/* required by monsterSpecial functions */
#include "savegame.h"
#include "settings.h"
#include "tile.h"
#include "utils.h"
#include "xml.h"

int monsterInfoLoaded = 0;
unsigned int numMonsters = 0;
Monster monsters[MAX_MONSTERS];

/**
 * Load monster information from monsters.xml
 */

void monsterLoadInfoFromXml() {
    xmlDocPtr doc;
    xmlNodePtr root, node;
    unsigned int i;
    int monster;
    static const struct {
        const char *name;
        unsigned int mask;
    } booleanAttributes[] = {
        { "undead", MATTR_UNDEAD },
        { "good", MATTR_GOOD },
        { "swims", MATTR_WATER },
        { "sails", MATTR_WATER },       
        { "cantattack", MATTR_NONATTACKABLE },       
        { "camouflage", MATTR_CAMOUFLAGE }, 
        { "wontattack", MATTR_NOATTACK },       
        { "ambushes", MATTR_AMBUSHES },
        { "incorporeal", MATTR_INCORPOREAL },
        { "nochest", MATTR_NOCHEST },
        { "divides", MATTR_DIVIDES }
    };    
    
    /* steals="" */
    static const struct {
        const char *name;
        unsigned int mask;
    } steals[] = {
        { "food", MATTR_STEALFOOD },
        { "gold", MATTR_STEALGOLD }
    };

    /* casts="" */
    static const struct {
        const char *name;
        unsigned int mask;
    } casts[] = {
        { "sleep", MATTR_CASTS_SLEEP },
        { "negate", MATTR_NEGATE }
    };

    /* movement="" */
    static const struct {
        const char *name;
        unsigned int mask;
    } movement[] = {
        { "none", MATTR_STATIONARY },
        { "wanders", MATTR_WANDERS }
    };

    /* boolean attributes that affect movement */
    static const struct {
        const char *name;
        unsigned int mask;
    } movementBoolean[] = {
        { "swims", MATTR_SWIMS },
        { "sails", MATTR_SAILS },
        { "flies", MATTR_FLIES },
        { "teleports", MATTR_TELEPORT },
        { "canMoveOntoMonsters", MATTR_CANMOVEMONSTERS },
        { "canMoveOntoAvatar", MATTR_CANMOVEAVATAR },
        { "canMoveOn", MATTR_CANMOVEON }
    };

    /* hit and miss tiles */
    static const struct {
        const char *name;
        unsigned int tile;
    } tiles[] = {
        { "fire", FIREFIELD_TILE },
        { "poison", POISONFIELD_TILE },
        { "lightning", LIGHTNINGFIELD_TILE },
        { "magic", MAGICFLASH_TILE },
        { "lava", LAVA_TILE },
        { "fireblast", HITFLASH_TILE },
        { "boulder", BOULDER_TILE },
        { "sleep", SLEEPFIELD_TILE }
    };

    static const struct {
        const char *name;
        TileEffect effect;
    } effects[] = {
        { "fire", (TileEffect)(EFFECT_FIRE | EFFECT_LAVA) },
        { "poison", (TileEffect)(EFFECT_POISON | EFFECT_POISONFIELD) },
        { "sleep", EFFECT_SLEEP }
    };

    if (!monsterInfoLoaded)
        monsterInfoLoaded = 1;
    else return;

    doc = xmlParse("monsters.xml");
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "monsters") != 0)
        errorFatal("malformed monsters.xml");

    monster = 0;
    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node) ||
            xmlStrcmp(node->name, (const xmlChar *) "monster") != 0)
            continue;

        monsters[monster].name = xmlGetPropAsStr(node, "name");
        monsters[monster].id = (unsigned short)xmlGetPropAsInt(node, "id");
        
        /* Get the leader if it's been included, otherwise the leader is itself */
        if (xmlPropExists(node, "leader"))        
            monsters[monster].leader = (unsigned char)xmlGetPropAsInt(node, "leader");
        else monsters[monster].leader = monsters[monster].id;
        
        monsters[monster].xp = (unsigned short)xmlGetPropAsInt(node, "exp");
        monsters[monster].ranged = xmlGetPropAsBool(node, "ranged");
        monsters[monster].tile = (MapTile)xmlGetPropAsInt(node, "tile");
        monsters[monster].frames = 1;
        monsters[monster].camouflageTile = 0;

        monsters[monster].worldrangedtile = 0;
        monsters[monster].rangedhittile = HITFLASH_TILE;
        monsters[monster].rangedmisstile = MISSFLASH_TILE;
        monsters[monster].leavestile = 0;

        monsters[monster].mattr = (MonsterAttrib)0;
        monsters[monster].movementAttr = (MonsterMovementAttrib)0;
        monsters[monster].slowedType = SLOWED_BY_TILE;
        monsters[monster].basehp = 0;
        monsters[monster].encounterSize = 0;
        monsters[monster].resists = 0;

        /* get the encounter size */
        if (xmlPropExists(node, "encounterSize")) {
            monsters[monster].encounterSize = 
                (unsigned char)xmlGetPropAsInt(node, "encounterSize");
        }

        /* get the base hp */
        if (xmlPropExists(node, "basehp")) {
            monsters[monster].basehp =
                (unsigned short)xmlGetPropAsInt(node, "basehp");
            /* adjust basehp according to battle difficulty setting */
            monsters[monster].basehp <<= (settings.battleDiff - 1);
        }

        /* get the camouflaged tile */
        if (xmlPropExists(node, "camouflageTile")) {        
            monsters[monster].camouflageTile =
                (MapTile)xmlGetPropAsInt(node, "camouflageTile");
        }

        /* get the ranged tile for world map attacks */
        for (i = 0; i < sizeof(tiles) / sizeof(tiles[0]); i++) {
            if (xmlPropCmp(node, "worldrangedtile", tiles[i].name) == 0) {
                monsters[monster].worldrangedtile = tiles[i].tile;
            }
        }

        /* get ranged hit tile */
        for (i = 0; i < sizeof(tiles) / sizeof(tiles[0]); i++) {
            if (xmlPropCmp(node, "rangedhittile", tiles[i].name) == 0) {
                monsters[monster].rangedhittile = tiles[i].tile;
            }
            else if (xmlPropCmp(node, "rangedhittile", "random") == 0)
                monsters[monster].mattr = (MonsterAttrib)(monsters[monster].mattr | MATTR_RANDOMRANGED);
        }

        /* get ranged miss tile */
        for (i = 0; i < sizeof(tiles) / sizeof(tiles[0]); i++) {
            if (xmlPropCmp(node, "rangedmisstile", tiles[i].name) == 0) {
                monsters[monster].rangedmisstile = tiles[i].tile;
            }
            else if (xmlPropCmp(node, "rangedhittile", "random") == 0)
                monsters[monster].mattr = (MonsterAttrib)(monsters[monster].mattr | MATTR_RANDOMRANGED);
        }

        /* find out if the monster leaves a tile behind on ranged attacks */
        monsters[monster].leavestile = xmlGetPropAsBool(node, "leavestile");

        /* get effects that this monster is immune to */
        for (i = 0; i < sizeof(effects) / sizeof(effects[0]); i++) {
            if (xmlPropCmp(node, "resists", effects[i].name) == 0) {
                monsters[monster].resists = effects[i].effect;
            }
        }

        /* get the number of frames for animation */
        if (xmlPropExists(node, "frames")) {
            monsters[monster].frames =
                (unsigned char)xmlGetPropAsInt(node, "frames");
        }
        else monsters[monster].frames = 2;

        /* Load monster attributes */
        for (i = 0; i < sizeof(booleanAttributes) / sizeof(booleanAttributes[0]); i++) {
            if (xmlGetPropAsBool(node, booleanAttributes[i].name)) {
                monsters[monster].mattr = (MonsterAttrib)(monsters[monster].mattr | booleanAttributes[i].mask);
            }
        }
        
        /* Load boolean attributes that affect movement */
        for (i = 0; i < sizeof(movementBoolean) / sizeof(movementBoolean[0]); i++) {
            if (xmlGetPropAsBool(node, movementBoolean[i].name)) {
                monsters[monster].movementAttr = (MonsterMovementAttrib)(monsters[monster].movementAttr | movementBoolean[i].mask);
            }
        }

        /* steals="" */
        for (i = 0; i < sizeof(steals) / sizeof(steals[0]); i++) {
            if (xmlPropCmp(node, "steals", steals[i].name) == 0) {
                monsters[monster].mattr = (MonsterAttrib)(monsters[monster].mattr | steals[i].mask);
            }
        }

        /* casts="" */
        for (i = 0; i < sizeof(casts) / sizeof(casts[0]); i++) {
            if (xmlPropCmp(node, "casts", casts[i].name) == 0) {
                monsters[monster].mattr = (MonsterAttrib)(monsters[monster].mattr | casts[i].mask);
            }
        }

        /* movement="" */
        for (i = 0; i < sizeof(movement) / sizeof(movement[0]); i++) {
            if (xmlPropCmp(node, "movement", movement[i].name) == 0) {
                monsters[monster].movementAttr = (MonsterMovementAttrib)(monsters[monster].movementAttr | movement[i].mask);
            }
        }

        /* Figure out which 'slowed' function to use */
        if (monsterSails(&monsters[monster]))        
            /* sailing monsters (pirate ships) */
            monsters[monster].slowedType = SLOWED_BY_WIND;
        else if (monsterFlies(&monsters[monster]) || monsterIsIncorporeal(&monsters[monster]))
            /* flying monsters (dragons, bats, etc.) and incorporeal monsters (ghosts, zorns) */
            monsters[monster].slowedType = SLOWED_BY_NOTHING;
            
        monster++;
        numMonsters++;
    }

    xmlFreeDoc(doc);
}

const Monster *monsterForTile(MapTile tile) {
    unsigned int i;
    monsterLoadInfoFromXml();

    for (i = 0; i < numMonsters; i++) {
        if ((tile >= monsters[i].tile) && (tile < monsters[i].tile + monsters[i].frames))
            return &(monsters[i]);
    }

    return NULL;
}

int monsterIsGood(const Monster *monster) {
    return (monster->mattr & MATTR_GOOD) ? 1 : 0;
}

int monsterIsEvil(const Monster *monster) {
    return !monsterIsGood(monster);
}

int monsterIsUndead(const Monster *monster) {
    return (monster->mattr & MATTR_UNDEAD) ? 1 : 0;
}

int monsterLeavesChest(const Monster *monster) {
    if (monsterIsAquatic(monster))
        return 0;
    else return (monster->mattr & MATTR_NOCHEST) ? 0 : 1;
}

int monsterIsAquatic(const Monster *monster) {
    return (monster->mattr & MATTR_WATER) ? 1 : 0;
}

int monsterWanders(const Monster *monster) {
    return (monster->movementAttr & MATTR_WANDERS) ? 1 : 0;
}

int monsterIsStationary(const Monster *monster) {
    return (monster->movementAttr & MATTR_STATIONARY) ? 1 : 0;
}

int monsterFlies(const Monster *monster) {
    return (monster->movementAttr & MATTR_FLIES) ? 1 : 0;
}

int monsterTeleports(const Monster *monster) {
    return (monster->movementAttr & MATTR_TELEPORT) ? 1 : 0;
}

int monsterSwims(const Monster *monster) {
    return (monster->movementAttr & MATTR_SWIMS) ? 1 : 0;
}

int monsterSails(const Monster *monster) {
    return (monster->movementAttr & MATTR_SAILS) ? 1 : 0;
}

int monsterWalks(const Monster *monster) {
    return (monsterFlies(monster) |
            monsterSwims(monster) |
            monsterSails(monster)) ? 0 : 1;
}

int monsterDivides(const Monster *monster) {
    return (monster->mattr & MATTR_DIVIDES) ? 1 : 0;
}

int monsterCanMoveOntoMonsters(const Monster *monster) {
    return (monster->movementAttr & MATTR_CANMOVEMONSTERS) ? 1 : 0;
}

int monsterCanMoveOntoAvatar(const Monster *monster) {
    return (monster->movementAttr & MATTR_CANMOVEAVATAR) ? 1 : 0;
}

int monsterCanMoveOnto(const Monster *monster) {
    return (monster->movementAttr & MATTR_CANMOVEON) ? 1 : 0;
}

int monsterIsAttackable(const Monster *monster) {
    return (monster->mattr & MATTR_NONATTACKABLE) ? 0 : 1;
}

int monsterWillAttack(const Monster *monster) {
    return (monster->mattr & MATTR_NOATTACK) ? 0 : 1;
}

int monsterStealsGold(const Monster *monster) {
    return (monster->mattr & MATTR_STEALGOLD) ? 1 : 0;
}

int monsterStealsFood(const Monster *monster) {
    return (monster->mattr & MATTR_STEALFOOD) ? 1 : 0;
}

int monsterNegates(const Monster *monster) {
    return (monster->mattr & MATTR_NEGATE) ? 1 : 0;
}

int monsterCamouflages(const Monster *monster) {
    return (monster->mattr & MATTR_CAMOUFLAGE) ? 1 : 0;
}

int monsterAmbushes(const Monster *monster) {
    return (monster->mattr & MATTR_AMBUSHES) ? 1 : 0;
}

int monsterIsIncorporeal(const Monster *monster) {
    return (monster->mattr & MATTR_INCORPOREAL) ? 1 : 0;
}

int monsterHasRandomRangedAttack(const Monster *monster) {
    return (monster->mattr & MATTR_RANDOMRANGED) ? 1 : 0;
}

int monsterLeavesTile(const Monster *monster) {
    return (monster->leavestile);
}

int monsterGetDamage(const Monster *monster) {
    int damage, val, x;
    val = monster->basehp;    
    x = xu4_random(val >> 2);
    damage = (x >> 4) + ((x >> 2) & 0xfc);
    damage += x % 10;
    return damage;
}

int monsterGetCamouflageTile(const Monster *monster) {
    return monster->camouflageTile;
}

void monsterSetRandomRangedWeapon(Monster *monster) {
    monster->rangedhittile = monster->rangedmisstile = xu4_random(4) + POISONFIELD_TILE;
}

int monsterCastSleep(const Monster *monster) {
    return
        (monster->mattr & MATTR_CASTS_SLEEP) &&
        (c->aura != AURA_NEGATE) &&
        xu4_random(4) == 0;
}

const Monster *monsterRandomForTile(MapTile tile) {
    int era;
    MapTile randTile;    

    if (tileIsSailable(tile)) {
        randTile = (xu4_random(8) << 1) + monsterById(PIRATE_ID)->tile;
        return monsterForTile(randTile);        
    }
    else if (tileIsSwimable(tile)) {
        randTile = (xu4_random(5) << 1) + monsterById(NIXIE_ID)->tile;
        return monsterForTile(randTile);
    }

    if (!tileIsMonsterWalkable(tile))
        return 0;

    //if (c->saveGame->moves > 100000) /* what's 100,000 moves all about? */
    if (c->saveGame->moves > 30000)
        era = 0x0f;
    else if (c->saveGame->moves > 20000)
        era = 0x07;
    else
        era = 0x03;

    randTile = ((era & xu4_random(0x10) & xu4_random(0x10)) << 2) + monsterById(ORC_ID)->tile;
    return monsterForTile(randTile);
}

const Monster *monsterForDungeon(int dngLevel) {
    int era;
    const Monster *monsters[] =
    {
        monsterById(RAT_ID),
        monsterById(BAT_ID),
        monsterById(GIANT_SPIDER_ID),
        monsterById(GHOST_ID),
        monsterById(SLIME_ID),
        monsterById(TROLL_ID),
        monsterById(GREMLIN_ID),
        monsterById(MIMIC_ID),
        monsterById(REAPER_ID),
        monsterById(INSECT_SWARM_ID),
        monsterById(GAZER_ID),
        monsterById(PHANTOM_ID),
        monsterById(ORC_ID),
        monsterById(SKELETON_ID),
        monsterById(ROGUE_ID)
    };

    era = 9 + dngLevel;
    return monsters[era & xu4_random(0x100) & xu4_random(0x100)];
}

int monsterGetInitialHp(const Monster *monster) {
    int basehp, hp;
    
    basehp = monster->basehp;

    hp = xu4_random(basehp) | (basehp / 2);
    
    /* make sure the monster doesn't flee initially */
    if (hp < 24) hp = 24;

    return hp;
}

MonsterStatus monsterGetStatus(const Monster *monster, int hp) {
    int basehp, heavy_threshold, light_threshold, crit_threshold;

    basehp = monster->basehp;
    crit_threshold = basehp >> 2;  /* (basehp / 4) */
    heavy_threshold = basehp >> 1; /* (basehp / 2) */
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

const Monster *monsterGetAmbushingMonster(void) {
    unsigned int i,
        numAmbushingMonsters = 0,
        randMonster;    

    /* first, find out how many monsters exist that might ambush you */
    for (i = 0; i < numMonsters; i++) {
        if (monsterAmbushes(&monsters[i]))
            numAmbushingMonsters++;
    }
    
    if (numAmbushingMonsters > 0) {
        /* now, randomely select one of them */
        randMonster = xu4_random(numAmbushingMonsters);
        numAmbushingMonsters = 0;

        /* now, find the one we selected */
        for (i = 0; i < numMonsters; i++) {
            if (monsterAmbushes(&monsters[i])) {
                /* found the monster - return it! */
                if (numAmbushingMonsters == randMonster)
                    return &monsters[i];
                /* move on to the next monster */
                else numAmbushingMonsters++;
            }
        }
    }

    ASSERT(0, "failed to find an ambushing monster");
    return NULL;
}

int monsterSpecialAction(Object *obj) {
    int broadsidesDirs, dx, dy, mapdist;
    const Monster *m = NULL;
    CoordActionInfo *info;
    int retval = 0;    

    if (obj->getType() == OBJECT_MONSTER) {
        MapCoords o_coords = obj->getCoords();

        m = obj->monster;

        broadsidesDirs = dirGetBroadsidesDirs(tileGetDirection(obj->getTile()));        
        
        dx = abs(c->location->coords.x - o_coords.x);
        dy = abs(c->location->coords.y - o_coords.y);
        mapdist = c->location->coords.distance(o_coords, c->location->map);

        /* setup info for monster action */
        info = new CoordActionInfo;        
        info->handleAtCoord = &monsterRangeAttack; /* standard action */
        info->origin = obj->getCoords();
        info->prev = MapCoords(-1, -1);
        info->range = 3;
        info->validDirections = MASK_DIR_ALL;
        info->player = -1;
        info->blockedPredicate = NULL;
        info->blockBefore = 1; 
        info->firstValidDistance = 1;

        /* find out which direction the avatar is in relation to the monster */
        info->dir = o_coords.getRelativeDirection(c->location->coords, c->location->map);
       
        switch(m->id) {
        
        case LAVA_LIZARD_ID:
        case SEA_SERPENT_ID:
        case HYDRA_ID:
        case DRAGON_ID:

            retval = 1;            
            
            /* A 50/50 chance they try to range attack when you're close enough */
            if (mapdist <= 3 && xu4_random(2) == 0)                
                gameDirectionalAction(info);            
            else retval = 0;
            
            break;

        case PIRATE_ID:
            
            /* Fire cannon: Pirates only fire broadsides and only when they can hit you :) */
            retval = 1;

            info->handleAtCoord = &fireAtCoord;
            info->validDirections = broadsidesDirs;            
            
            if ((((dx == 0) && (dy <= 3)) ||        /* avatar is close enough and on the same column, OR */
                 ((dy == 0) && (dx <= 3))) &&       /* avatar is close enough and on the same row, AND */
                 ((broadsidesDirs & info->dir) > 0))/* pirate ship is firing broadsides */
                 gameDirectionalAction(info);       /* *** FIRE! *** */
            else
                retval = 0;            
            
            break;       

        default: break;
        }

        delete info;
    }
    
    return retval;
}

void monsterSpecialEffect(Object *obj) {
    Object *o;
    const Monster *m = NULL;        

    if (obj->getType() == OBJECT_MONSTER) {
        m = obj->monster;
        switch(m->id) {        
        
        case STORM_ID:
            {
                if (obj->getCoords() == c->location->coords) {

                    /* damage the ship */
                    if (c->transportContext == TRANSPORT_SHIP) {
                        /* FIXME: Check actual damage from u4dos */                           
                        gameDamageShip(10, 30);                        
                    }
                    /* anything else but balloon damages the party */
                    else if (c->transportContext != TRANSPORT_BALLOON) {
                        /* FIXME: formula for twister damage is guesstimated from u4dos */
                        gameDamageParty(0, 75);
                    }
                    break;
                }

                /* See if the storm is on top of any objects and destroy them! */
                for (o = c->location->map->objects; o; o = o->next) {                
                    if (o != obj && 
                        o->getCoords() == obj->getCoords()) {                        
                        /* Converged with an object, destroy the object! */
                        mapRemoveObject(c->location->map, o);
                        break;
                    }
                }
            }      
            break;
        
        case WHIRLPOOL_ID:        
            {
                if (obj->getCoords() == c->location->coords && (c->transportContext == TRANSPORT_SHIP)) {                    
                                    
                    /* Deal 10 damage to the ship */
                    gameDamageShip(-1, 10);

                    /* Send the party to Locke Lake */
                    c->location->coords = MapCoords(127, 78);                    

                    /* Destroy the whirlpool that sent you there */
                    mapRemoveObject(c->location->map, obj);
                    break;
                }
            
                /* See if the whirlpool is on top of any objects and destroy them! */
                for (o = c->location->map->objects; o; o = o->next) {
                    if (o != obj && 
                        o->getCoords() == obj->getCoords()) {
                    
                        /* Make sure the object isn't a flying monster or object */
                        if (!tileIsBalloon(o->getTile()) && ((o->getType() != OBJECT_MONSTER) || !monsterFlies(o->monster)))
                            /* Destroy the object it met with */
                            mapRemoveObject(c->location->map, o);
                        break;
                    }
                }            
            }

        default: break;
        }
    }
}

/**
 * Returns a pointer to the monster with the corresponding 'id'
 */

const Monster *monsterById(unsigned short id) {
    unsigned int i;

    /* make sure monster info has been loaded */
    monsterLoadInfoFromXml();

    for (i = 0; i < numMonsters; i++) {
        if (monsters[i].id == id)
            return &monsters[i];
    }

    return NULL;
}
