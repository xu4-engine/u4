/**
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "monster.h"

/* FIXME: should monsterSpecialAction() and monsterSpecialEffect() be placed elsewhere
   to make monster.c as independent as possible? */

#include "context.h"
#include "error.h"
#include "game.h"	/* required by monsterSpecial functions */
#include "player.h"	/* required by monsterSpecial functions */
#include "savegame.h"
#include "ttype.h"
#include "u4file.h"

int monsterInfoLoaded = 0;
int numMonsters = 0;
Monster monsters[MAX_MONSTERS];

/**
 * Load monster information from monsters.xml
 */

void monsterLoadInfoFromXml() {
    char *fname;
    xmlDocPtr doc;
    xmlNodePtr root, node;
    int monster, i;
    static const struct {
        const char *name;
        unsigned int mask;
    } booleanAttributes[] = {
        { "undead", MATTR_UNDEAD },
        { "good", MATTR_GOOD },
        { "swims", MATTR_WATER },
        { "sails", MATTR_WATER },
        { "stationary", MATTR_STATIONARY },
        { "cantattack", MATTR_NONATTACKABLE },
        { "teleports", MATTR_TELEPORT },
        { "camouflage", MATTR_CAMOUFLAGE }, 
        { "wontattack", MATTR_NOATTACK },
        { "flies", MATTR_FLIES },
        { "ambushes", MATTR_AMBUSHES }
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

    if (!monsterInfoLoaded)
        monsterInfoLoaded = 1;
    else return;

    fname = u4find_conf("monsters.xml");
    if (!fname)
        errorFatal("unable to open file monsters.xml");
    doc = xmlParseFile(fname);
    if (!doc)
        errorFatal("error parsing monsters.xml");

    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "monsters") != 0)
        errorFatal("malformed monsters.xml");

    monster = 0;
    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node) ||
            xmlStrcmp(node->name, (const xmlChar *) "monster") != 0)
            continue;

        monsters[monster].name = (char *)xmlGetProp(node, (const xmlChar *)"name");
        monsters[monster].id = (unsigned short)atoi((char *)xmlGetProp(node, (const xmlChar *)"id"));
        
        /* Get the leader if it's been included, otherwise the leader is itself */
        if (xmlGetProp(node, (const xmlChar *)"leader") != NULL)
            monsters[monster].leader = (unsigned char)atoi((char *)xmlGetProp(node, (const xmlChar *)"leader"));
        else monsters[monster].leader = monsters[monster].id;

        monsters[monster].level = (unsigned short)atoi((char *)xmlGetProp(node, (const xmlChar *)"level"));
        monsters[monster].ranged = (xmlStrcmp(xmlGetProp(node, (const xmlChar *)"ranged"), 
            (const xmlChar *) "true") == 0);
        monsters[monster].tile = (unsigned char)atoi((char *)xmlGetProp(node, (const xmlChar *)"tile"));
        monsters[monster].frames = 1;

        monsters[monster].rangedhittile = HITFLASH_TILE;
        monsters[monster].rangedmisstile = MISSFLASH_TILE;

        monsters[monster].mattr = 0;
        monsters[monster].slowedType = SLOWED_BY_TILE;
        monsters[monster].basehp = 0;
        monsters[monster].encounterSize = 0;        

        /* get the encounter size */
        if (xmlGetProp(node, (const xmlChar *)"encounterSize") != NULL) {
            monsters[monster].encounterSize = 
                (unsigned char)atoi((char *)xmlGetProp(node, (const xmlChar *)"encounterSize"));             
        }

        /* get the base hp */
        if (xmlGetProp(node, (const xmlChar *)"basehp") != NULL) {
            monsters[monster].basehp =
                (unsigned char)atoi((char *)xmlGetProp(node, (const xmlChar *)"basehp"));
        }

        /* get ranged hit tile */
        for (i = 0; i < sizeof(tiles) / sizeof(tiles[0]); i++) {
            if (xmlStrcmp(xmlGetProp(node, (const xmlChar *)"rangedhittile"), 
                          (const xmlChar *)tiles[i].name) == 0) {
                monsters[monster].rangedhittile = tiles[i].tile;
            }
        }

        /* get ranged miss tile */
        for (i = 0; i < sizeof(tiles) / sizeof(tiles[0]); i++) {
            if (xmlStrcmp(xmlGetProp(node, (const xmlChar *)"rangedmisstile"), 
                          (const xmlChar *)tiles[i].name) == 0) {
                monsters[monster].rangedmisstile = tiles[i].tile;
            }
        }

        /* get the number of frames for animation */
        if (xmlGetProp(node, (const xmlChar *)"frames") != NULL) {
            monsters[monster].frames =
                (unsigned char)atoi((char *)xmlGetProp(node, (const xmlChar *)"frames"));
        }

        /* Load monster attributes */
        for (i = 0; i < sizeof(booleanAttributes) / sizeof(booleanAttributes[0]); i++) {
            if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) booleanAttributes[i].name), 
                          (const xmlChar *) "true") == 0) {
                monsters[monster].mattr |= booleanAttributes[i].mask;
            }
        }

        /* steals="" */
        for (i = 0; i < sizeof(steals) / sizeof(steals[0]); i++) {
            if (xmlStrcmp(xmlGetProp(node, (const xmlChar *)"steals"), 
                          (const xmlChar *)steals[i].name) == 0) {
                monsters[monster].mattr |= steals[i].mask;
            }
        }

        /* casts="" */
        for (i = 0; i < sizeof(casts) / sizeof(casts[0]); i++) {
            if (xmlStrcmp(xmlGetProp(node, (const xmlChar *)"casts"), 
                          (const xmlChar *)casts[i].name) == 0) {
                monsters[monster].mattr |= casts[i].mask;
            }
        }

        /* movement="" */
        for (i = 0; i < sizeof(movement) / sizeof(movement[0]); i++) {
            if (xmlStrcmp(xmlGetProp(node, (const xmlChar *)"movement"), 
                          (const xmlChar *)movement[i].name) == 0) {
                monsters[monster].mattr |= movement[i].mask;
            }
        }

        /* Figure out which 'slowed' function to use */
        if (xmlStrcmp(xmlGetProp(node, (const xmlChar *)"sails"), (const xmlChar *)"true") == 0)
            monsters[monster].slowedType = SLOWED_BY_WIND;
        else if (xmlStrcmp(xmlGetProp(node, (const xmlChar *)"flies"), (const xmlChar *)"true") == 0)
            monsters[monster].slowedType = SLOWED_BY_NOTHING;
            
        monster++;
        numMonsters++;
    }

    xmlFreeDoc(doc);
}

const Monster *monsterForTile(unsigned char tile) {
    int i;
    monsterLoadInfoFromXml();

    for (i = 0; i < numMonsters; i++) {        
        if (tile >= monsters[i].tile && tile < monsters[i].tile + monsters[i].frames)
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

int monsterIsAquatic(const Monster *monster) {
    return (monster->mattr & MATTR_WATER) ? 1 : 0;
}

int monsterFlies(const Monster *monster) {
    return (monster->mattr & MATTR_FLIES) ? 1 : 0;
}

int monsterTeleports(const Monster *monster) {
    return (monster->mattr & MATTR_TELEPORT) ? 1 : 0;
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

int monsterAmbushes(const Monster *monster) {
    return (monster->mattr & MATTR_AMBUSHES) ? 1 : 0;
}

int monsterGetXp(const Monster *monster) {
    return (monster->level == 16) ? 16 : monster->level + 1;    
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
    int era;    
    
    if (tileIsSailable(tile)) {        
        return monsterById((rand() % 7) + PIRATE_ID);        
    }
    else if (tileIsSwimable(tile)) {        
        return monsterById((rand() % 6) + NIXIE_ID);
    }

    if (!tileIsMonsterWalkable(tile))
        return 0;

    if (c->saveGame->moves > 100000)
        era = 0x0f;
    else if (c->saveGame->moves > 20000)
        era = 0x07;
    else
        era = 0x03;
    
    return monsterById((era & rand() & rand()) + ORC_ID);
}

int monsterGetInitialHp(const Monster *monster) {
    int basehp, hp;

    basehp = (monster->basehp > 0) ? 
        monster->basehp :
        monster->level == 16 ? 255 : (monster->level << 4);

    hp = (rand() % basehp) + (basehp / 2);
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

int monsterSpecialAction(Object *obj) {
    int broadsidesDirs, dx, dy, mapdist, dirx, diry;
    const Monster *m = NULL;
    CoordActionInfo *info;
    int retval = 0;    

    if (obj->objType == OBJECT_MONSTER) {

        m = obj->monster;

        broadsidesDirs = DIR_REMOVE_FROM_MASK(tileGetDirection(obj->tile), MASK_DIR_ALL);
        broadsidesDirs = DIR_REMOVE_FROM_MASK(dirReverse(tileGetDirection(obj->tile)), broadsidesDirs);

        dx = c->location->x - obj->x;
        dy = c->location->y - obj->y;
        mapdist = mapDistance(c->location->x, c->location->y, obj->x, obj->y);

        dirx = diry = DIR_NONE;
        
        /* Find out if the avatar is east or west of the object */
        if (dx < 0) {
            dx *= -1;
            dirx = DIR_WEST;
        } else if (dx > 0)
            dirx = DIR_EAST;

        /* Find out if the avatar is north or south of the object */
        if (dy < 0) {            
            dy *= -1;
            diry = DIR_NORTH;
        } else if (dy > 0)
            diry = DIR_SOUTH;

        switch(m->id) {
        case PIRATE_ID:
            /** 
             * Fire cannon: Pirates only fire broadsides and only when they can hit you :)
             */
            retval = 1;

            info = (CoordActionInfo *) malloc(sizeof(CoordActionInfo));
            info->handleAtCoord = &fireAtCoord;
            info->origin_x = obj->x;
            info->origin_y = obj->y;
            info->prev_x = info->prev_y = -1;
            info->range = 3;
            info->validDirections = broadsidesDirs;
            info->player = -1;
            info->blockedPredicate = NULL;
            info->blockBefore = 1; 
            info->firstValidDistance = 1;
            
            if ((dy == 0) && (dx <= 3) && DIR_IN_MASK(dirx, broadsidesDirs)) {
                /* Fire cannon in 'dirx' direction */
                info->dir = MASK_DIR(dirx);
                gameDirectionalAction(info);
            }
            else if ((dx == 0) && (dy <= 3) && DIR_IN_MASK(diry, broadsidesDirs)) {
                /* Fire cannon in 'diry' direction */
                info->dir = MASK_DIR(diry);
                gameDirectionalAction(info);
            }
            else retval = 0;

            free(info);
            break;
        
        case SEA_SERPENT_ID: /* ranged */
        case HYDRA_ID: /* ranged */
        case DRAGON_ID: /* ranged */

            retval = 1;
            
            info = (CoordActionInfo *) malloc(sizeof(CoordActionInfo));
            info->handleAtCoord = &monsterRangeAttack;
            info->origin_x = obj->x;
            info->origin_y = obj->y;
            info->prev_x = info->prev_y = -1;
            info->range = 3;
            info->validDirections = MASK_DIR_ALL;
            info->player = -1;
            info->blockedPredicate = NULL;
            info->blockBefore = 1;
            info->firstValidDistance = 1;

            /* A 50/50 chance they try to range attack when you're close enough */
            if (mapdist <= 3 && (rand() % 2 == 0)) {
                info->dir = (diry ? MASK_DIR(diry) : 0) | (dirx ? MASK_DIR(dirx) : 0);
                gameDirectionalAction(info);
            }
            else retval = 0;

            free(info);
            break;
    
            /* FIXME: add ranged monster's behavior here */

            /* Other ranged monsters here too, whoever you are! */

        default: break;
        }
    }

    return retval;
}

void monsterSpecialEffect(Object *obj) {
    Object *o;
    const Monster *m = NULL;        

    if (obj->objType == OBJECT_MONSTER) {
        m = obj->monster;
        switch(m->id) {        
        
        case STORM_ID:
            {
                if (obj->x == c->location->x &&
                    obj->y == c->location->y &&
                    obj->z == c->location->z) {

                    if (tileIsShip(c->saveGame->transport)) {
                        /* FIXME: Check actual damage from u4dos
                           Screen should shake here */
                        gameDamageShip(10, 30);                        
                    }
                    else {
                        /* FIXME: formula for twister damage is guesstimated from u4dos */
                        gameDamageParty(0, 75);                        
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
        
        case WHIRLPOOL_ID:        
            {
                if (obj->x == c->location->x &&
                    obj->y == c->location->y &&
                    obj->z == c->location->z && tileIsShip(c->saveGame->transport)) {                    
                
                    /* FIXME: Screen should shake here */
                    gameDamageShip(-1, 10);

                    c->location->x = 127;
                    c->location->y = 78;

                    /* Destroy the whirlpool that sent you there */
                    mapRemoveObject(c->location->map, obj);
                    break;
                }
            
                /* See if the whirlpool is on top of any objects and destroy them! */
                for (o = c->location->map->objects; o; o = o->next) {
                    if (o != obj && 
                        o->x == obj->x &&
                        o->y == obj->y &&
                        o->z == obj->z) {                    
                    
                        /* Make sure the object isn't a flying monster or object */
                        if (!tileIsBalloon(o->tile) && ((o->objType != OBJECT_MONSTER) || !monsterFlies(o->monster)))
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
    int i;

    /* make sure monster info has been loaded */
    monsterLoadInfoFromXml();

    for (i = 0; i < numMonsters; i++) {
        if (monsters[i].id == id)
            return &monsters[i];
    }

    return NULL;
}
