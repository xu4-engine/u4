/**
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <libxml/xmlmemory.h>

#include "monster.h"

#include "context.h"
#include "debug.h"
#include "error.h"
#include "event.h"
#include "game.h"	/* required by specialAction and specialEffect functions */
#include "location.h"
#include "player.h"	/* required by specialAction and specialEffect functions */
#include "savegame.h"
#include "settings.h"
#include "tile.h"
#include "utils.h"
#include "xml.h"

MonsterMgr monsters;

/**
 * Monster class implementation
 */ 
Monster::Monster(MapTile tile) : Object(OBJECT_MONSTER) {
    const Monster *m = monsters.getByTile(tile);
    if (m)
        *this = *m;
}

bool Monster::isGood() const        { return (mattr & MATTR_GOOD) ? true : false; }
bool Monster::isEvil() const        { return !isGood(); }
bool Monster::isUndead() const      { return (mattr & MATTR_UNDEAD) ? true : false; }

bool Monster::leavesChest() const {
    if (isAquatic())
        return true;
    else return (mattr & MATTR_NOCHEST) ? false : true;
}

bool Monster::isAquatic() const     { return (mattr & MATTR_WATER) ? true : false; }
bool Monster::wanders() const       { return (movementAttr & MATTR_WANDERS) ? true : false; }
bool Monster::isStationary() const  { return (movementAttr & MATTR_STATIONARY) ? true : false; }
bool Monster::flies() const         { return (movementAttr & MATTR_FLIES) ? true : false; }
bool Monster::teleports() const     { return (movementAttr & MATTR_TELEPORT) ? true : false; }
bool Monster::swims() const         { return (movementAttr & MATTR_SWIMS) ? true : false; }
bool Monster::sails() const         { return (movementAttr & MATTR_SAILS) ? true : false; }

bool Monster::walks() const {
    return (flies() || swims() || sails()) ? false : true;
}

bool Monster::divides() const       { return (mattr & MATTR_DIVIDES) ? true : false; }
bool Monster::canMoveOntoMonsters() const { return (movementAttr & MATTR_CANMOVEMONSTERS) ? true : false; }
bool Monster::canMoveOntoPlayer() const   { return (movementAttr & MATTR_CANMOVEAVATAR) ? true : false; }
bool Monster::canMoveOnto() const         { return (movementAttr & MATTR_CANMOVEON) ? true : false; }
bool Monster::isAttackable() const  { return (mattr & MATTR_NONATTACKABLE) ? true : false; }
bool Monster::willAttack() const    { return (mattr & MATTR_NOATTACK) ? false : true; }
bool Monster::stealsGold() const    { return (mattr & MATTR_STEALGOLD) ? true : false; }
bool Monster::stealsFood() const    { return (mattr & MATTR_STEALFOOD) ? true : false; }
bool Monster::negates() const       { return (mattr & MATTR_NEGATE) ? true : false; }
bool Monster::camouflages() const   { return (mattr & MATTR_CAMOUFLAGE) ? true : false; }
bool Monster::ambushes() const      { return (mattr & MATTR_AMBUSHES) ? true : false; }
bool Monster::isIncorporeal() const { return (mattr & MATTR_INCORPOREAL) ? true : false; }
bool Monster::hasRandomRanged() const { return (mattr & MATTR_RANDOMRANGED) ? true : false; }
bool Monster::leavesTile() const    { return leavestile; }
bool Monster::castsSleep() const { return (mattr & MATTR_CASTS_SLEEP) ? true : false; }
MapTile Monster::getCamouflageTile() const { return camouflageTile; }

int  Monster::getDamage() const {
    int damage, val, x;
    val = basehp;    
    x = xu4_random(val >> 2);
    damage = (x >> 4) + ((x >> 2) & 0xfc);
    damage += x % 10;
    return damage;
}

int Monster::setInitialHp(int points) {
    if (points < 0)
        hp = xu4_random(basehp) | (basehp / 2);
    else
        hp = points;
    
    /* make sure the monster doesn't flee initially */
    if (hp < 24) hp = 24;

    return hp;
}

void Monster::setRandomRanged() {
    rangedhittile = rangedmisstile = xu4_random(4) + POISONFIELD_TILE;
}

MonsterStatus Monster::getStatus() const {
    int heavy_threshold, light_threshold, crit_threshold;
    
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

/**
 * Performs a special action for the monster
 * Returns true if the action takes up the monsters
 * whole turn (i.e. it cant move afterwords)
 */ 
bool Monster::specialAction() {
    int broadsidesDirs, dx, dy, mapdist;    
    CoordActionInfo *info;
    bool retval = false;        
    broadsidesDirs = dirGetBroadsidesDirs(tileGetDirection(tile));

    dx = abs(c->location->coords.x - coords.x);
    dy = abs(c->location->coords.y - coords.y);
    mapdist = c->location->coords.distance(coords, c->location->map);

    /* setup info for monster action */
    info = new CoordActionInfo;        
    info->handleAtCoord = &monsterRangeAttack; /* standard action */
    info->origin = coords;
    info->prev = MapCoords(-1, -1);
    info->range = 3;
    info->validDirections = MASK_DIR_ALL;
    info->player = -1;
    info->blockedPredicate = NULL;
    info->blockBefore = 1; 
    info->firstValidDistance = 1;

    /* find out which direction the avatar is in relation to the monster */
    MapCoords mapcoords(coords);
    info->dir = mapcoords.getRelativeDirection(c->location->coords, c->location->map);
   
    switch(id) {
    
    case LAVA_LIZARD_ID:
    case SEA_SERPENT_ID:
    case HYDRA_ID:
    case DRAGON_ID:       
        
        /* A 50/50 chance they try to range attack when you're close enough */
        if (mapdist <= 3 && xu4_random(2) == 0)                
            gameDirectionalAction(info);       
        
        break;

    case PIRATE_ID:
        
        /* Fire cannon: Pirates only fire broadsides and only when they can hit you :) */
        retval = true;

        info->handleAtCoord = &fireAtCoord;
        info->validDirections = broadsidesDirs;            
        
        if ((((dx == 0) && (dy <= 3)) ||        /* avatar is close enough and on the same column, OR */
             ((dy == 0) && (dx <= 3))) &&       /* avatar is close enough and on the same row, AND */
             ((broadsidesDirs & info->dir) > 0))/* pirate ship is firing broadsides */
             gameDirectionalAction(info);       /* *** FIRE! *** */
        else
            retval = false;
        
        break;

    default: break;
    }

    delete info;    
    
    return retval;
}

/**
 * Performs a special effect for the monster
 * Returns true if something special happened,
 * or false if nothing happened
 */ 
bool Monster::specialEffect() {
    Object *obj;    
    bool retval = false;
    
    switch(id) {
    
    case STORM_ID:
        {
            ObjectList::iterator i;

            if (coords == c->location->coords) {

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
                return true;
            }

            /* See if the storm is on top of any objects and destroy them! */
            for (i = c->location->map->objects.begin();
                 i != c->location->map->objects.end();) {

                obj = *i;
                if (this != obj && 
                    obj->getCoords() == coords) {                        
                    /* Converged with an object, destroy the object! */
                    i = c->location->map->removeObject(i);
                    retval = true;
                }
                else i++;
            }
        }      
        break;
    
    case WHIRLPOOL_ID:        
        {
            ObjectList::iterator i;

            if (coords == c->location->coords && (c->transportContext == TRANSPORT_SHIP)) {                    
                                
                /* Deal 10 damage to the ship */
                gameDamageShip(-1, 10);

                /* Send the party to Locke Lake */
                c->location->coords = MapCoords(127, 78);                    

                /* Destroy the whirlpool that sent you there */
                c->location->map->removeObject(this);
                retval = true;
                break;
            }
        
            /* See if the whirlpool is on top of any objects and destroy them! */
            for (i = c->location->map->objects.begin();
                 i != c->location->map->objects.end();) {
                
                obj = *i;                    
                if (this != obj && 
                    coords == obj->getCoords()) {
                    Monster *m = dynamic_cast<Monster*>(obj);
                                    
                    /* Make sure the object isn't a flying monster or object */
                    if (!tileIsBalloon(obj->getTile()) && (!m || !m->flies())) {
                        /* Destroy the object it met with */
                        i = c->location->map->removeObject(i);
                        retval = true;
                    }
                    else i++;
                }
                else i++;
            }            
        }

    default: break;
    }

    return retval;
}

/**
 * MonsterMgr class implementation
 */
MonsterMgr::MonsterMgr() {}

/**
 * Load monster info from xml
 */ 
void MonsterMgr::loadInfoFromXml() {
    xmlDocPtr doc;
    xmlNodePtr root, node;
    unsigned int i;
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

    doc = xmlParse("monsters.xml");
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "monsters") != 0)
        errorFatal("malformed monsters.xml");
    
    for (node = root->xmlChildrenNode; node; node = node->next) {
        Monster *m = new Monster;

        if (xmlNodeIsText(node) ||
            xmlStrcmp(node->name, (const xmlChar *) "monster") != 0)
            continue;

        m->name = xmlGetPropAsStr(node, "name");
        m->id = (unsigned short)xmlGetPropAsInt(node, "id");
        
        /* Get the leader if it's been included, otherwise the leader is itself */
        if (xmlPropExists(node, "leader"))        
            m->leader = (unsigned char)xmlGetPropAsInt(node, "leader");
        else m->leader = m->id;
        
        m->xp = (unsigned short)xmlGetPropAsInt(node, "exp");
        m->ranged = xmlGetPropAsBool(node, "ranged");
        m->setTile((MapTile)xmlGetPropAsInt(node, "tile"));
        m->frames = 1;
        m->camouflageTile = 0;

        m->worldrangedtile = 0;
        m->rangedhittile = HITFLASH_TILE;
        m->rangedmisstile = MISSFLASH_TILE;
        m->leavestile = 0;

        m->mattr = (MonsterAttrib)0;
        m->movementAttr = (MonsterMovementAttrib)0;
        m->slowedType = SLOWED_BY_TILE;
        m->basehp = 0;
        m->encounterSize = 0;
        m->resists = 0;

        /* get the encounter size */
        if (xmlPropExists(node, "encounterSize")) {
            m->encounterSize = 
                (unsigned char)xmlGetPropAsInt(node, "encounterSize");
        }

        /* get the base hp */
        if (xmlPropExists(node, "basehp")) {
            m->basehp =
                (unsigned short)xmlGetPropAsInt(node, "basehp");
            /* adjust basehp according to battle difficulty setting */
            m->basehp <<= (settings.battleDiff - 1);
        }

        /* get the camouflaged tile */
        if (xmlPropExists(node, "camouflageTile")) {        
            m->camouflageTile =
                (MapTile)xmlGetPropAsInt(node, "camouflageTile");
        }

        /* get the ranged tile for world map attacks */
        for (i = 0; i < sizeof(tiles) / sizeof(tiles[0]); i++) {
            if (xmlPropCmp(node, "worldrangedtile", tiles[i].name) == 0) {
                m->worldrangedtile = tiles[i].tile;
            }
        }

        /* get ranged hit tile */
        for (i = 0; i < sizeof(tiles) / sizeof(tiles[0]); i++) {
            if (xmlPropCmp(node, "rangedhittile", tiles[i].name) == 0) {
                m->rangedhittile = tiles[i].tile;
            }
            else if (xmlPropCmp(node, "rangedhittile", "random") == 0)
                m->mattr = (MonsterAttrib)(m->mattr | MATTR_RANDOMRANGED);
        }

        /* get ranged miss tile */
        for (i = 0; i < sizeof(tiles) / sizeof(tiles[0]); i++) {
            if (xmlPropCmp(node, "rangedmisstile", tiles[i].name) == 0) {
                m->rangedmisstile = tiles[i].tile;
            }
            else if (xmlPropCmp(node, "rangedhittile", "random") == 0)
                m->mattr = (MonsterAttrib)(m->mattr | MATTR_RANDOMRANGED);
        }

        /* find out if the monster leaves a tile behind on ranged attacks */
        m->leavestile = xmlGetPropAsBool(node, "leavestile");

        /* get effects that this monster is immune to */
        for (i = 0; i < sizeof(effects) / sizeof(effects[0]); i++) {
            if (xmlPropCmp(node, "resists", effects[i].name) == 0) {
                m->resists = effects[i].effect;
            }
        }

        /* get the number of frames for animation */
        if (xmlPropExists(node, "frames")) {
            m->frames =
                (unsigned char)xmlGetPropAsInt(node, "frames");
        }
        else m->frames = 2;

        /* Load monster attributes */
        for (i = 0; i < sizeof(booleanAttributes) / sizeof(booleanAttributes[0]); i++) {
            if (xmlGetPropAsBool(node, booleanAttributes[i].name)) {
                m->mattr = (MonsterAttrib)(m->mattr | booleanAttributes[i].mask);
            }
        }
        
        /* Load boolean attributes that affect movement */
        for (i = 0; i < sizeof(movementBoolean) / sizeof(movementBoolean[0]); i++) {
            if (xmlGetPropAsBool(node, movementBoolean[i].name)) {
                m->movementAttr = (MonsterMovementAttrib)(m->movementAttr | movementBoolean[i].mask);
            }
        }

        /* steals="" */
        for (i = 0; i < sizeof(steals) / sizeof(steals[0]); i++) {
            if (xmlPropCmp(node, "steals", steals[i].name) == 0) {
                m->mattr = (MonsterAttrib)(m->mattr | steals[i].mask);
            }
        }

        /* casts="" */
        for (i = 0; i < sizeof(casts) / sizeof(casts[0]); i++) {
            if (xmlPropCmp(node, "casts", casts[i].name) == 0) {
                m->mattr = (MonsterAttrib)(m->mattr | casts[i].mask);
            }
        }

        /* movement="" */
        for (i = 0; i < sizeof(movement) / sizeof(movement[0]); i++) {
            if (xmlPropCmp(node, "movement", movement[i].name) == 0) {
                m->movementAttr = (MonsterMovementAttrib)(m->movementAttr | movement[i].mask);
            }
        }

        /* Figure out which 'slowed' function to use */
        if (m->sails())
            /* sailing monsters (pirate ships) */
            m->slowedType = SLOWED_BY_WIND;
        else if (m->flies() || m->isIncorporeal())
            /* flying monsters (dragons, bats, etc.) and incorporeal monsters (ghosts, zorns) */
            m->slowedType = SLOWED_BY_NOTHING;
            
        /* add the monster to the list */
        monsters[m->id] = m;
    }

    xmlFreeDoc(doc);
}

/**
 * Returns a monster using a tile to find which one to create
 * or NULL if a monster with that tile cannot be found
 */ 
const Monster *MonsterMgr::getByTile(MapTile tile) const {
    MonsterMap::const_iterator i;

    for (i = monsters.begin(); i != monsters.end(); i++) {
        MapTile mtile = i->second->getTile();
        if ((tile >= mtile) && (tile < mtile + i->second->frames))
            return i->second;
    }
    return NULL;
}

/**
 * Returns the monster that has the corresponding id
 * or returns NULL if no monster with that id could
 * be found.
 */
const Monster *MonsterMgr::getById(MonsterId id) const {
    MonsterMap::const_iterator i = monsters.find(id);
    if (i != monsters.end())
        return i->second;
    else return NULL;
}

/**
 * Returns the monster that has the corresponding name
 * or returns NULL if no monster can be found with
 * that name (case insensitive)
 */ 
const Monster *MonsterMgr::getByName(string name) const {
    MonsterMap::const_iterator i;
    for (i = monsters.begin(); i != monsters.end(); i++) {
        if (strcasecmp(i->second->name.c_str(), name.c_str()) == 0)
            return i->second;
    }
    return NULL;
}

/**
 * Creates a random monster based on the tile given
 */ 
const Monster *MonsterMgr::randomForTile(MapTile tile) const {
    int era;
    MapTile randTile;    

    if (tileIsSailable(tile)) {        
        randTile = monsters.find(PIRATE_ID)->second->getTile();
        randTile += (xu4_random(8) << 1);
        return getByTile(randTile);        
    }
    else if (tileIsSwimable(tile)) {
        randTile = monsters.find(NIXIE_ID)->second->getTile();
        randTile += (xu4_random(5) << 1);
        return getByTile(randTile);
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

    randTile = monsters.find(ORC_ID)->second->getTile();
    randTile += ((era & xu4_random(0x10) & xu4_random(0x10)) << 2);
    return getByTile(randTile);
}

/**
 * Creates a random monster based on the dungeon level given
 */ 
const Monster *MonsterMgr::randomForDungeon(int dngLevel) const {
    int era;
    static std::vector<MonsterId> id_list;
    if (id_list.size() == 0) {
        id_list.push_back(RAT_ID);
        id_list.push_back(BAT_ID);
        id_list.push_back(GIANT_SPIDER_ID);
        id_list.push_back(GHOST_ID);
        id_list.push_back(SLIME_ID);
        id_list.push_back(TROLL_ID);
        id_list.push_back(GREMLIN_ID);
        id_list.push_back(MIMIC_ID);
        id_list.push_back(REAPER_ID);
        id_list.push_back(INSECT_SWARM_ID);
        id_list.push_back(GAZER_ID);
        id_list.push_back(PHANTOM_ID);
        id_list.push_back(ORC_ID);
        id_list.push_back(SKELETON_ID);
        id_list.push_back(ROGUE_ID);
    }
    
    era = 9 + dngLevel;
    return getById(era & xu4_random(0x100) & xu4_random(0x100));
}

/**
 * Creates a random ambushing monster
 */ 
const Monster *MonsterMgr::randomAmbushing() const {
    MonsterMap::const_iterator i;
    int numAmbushingMonsters = 0,
        randMonster;

    /* first, find out how many monsters exist that might ambush you */
    for (i = monsters.begin(); i != monsters.end(); i++) {
        if (i->second->ambushes())
            numAmbushingMonsters++;
    }
    
    if (numAmbushingMonsters > 0) {
        /* now, randomely select one of them */
        randMonster = xu4_random(numAmbushingMonsters);
        numAmbushingMonsters = 0;

        /* now, find the one we selected */
        for (i = monsters.begin(); i != monsters.end(); i++) {
            if (i->second->ambushes()) {
                /* found the monster - return it! */
                if (numAmbushingMonsters == randMonster)
                    return i->second;
                /* move on to the next monster */
                else numAmbushingMonsters++;
            }
        }
    }

    ASSERT(0, "failed to find an ambushing monster");
    return NULL;
}
