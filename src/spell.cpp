/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "u4.h"

#include "spell.h"

#include "annotation.h"
#include "combat.h"
#include "context.h"
#include "debug.h"
#include "direction.h"
#include "dungeon.h"
#include "event.h"
#include "game.h"
#include "location.h"
#include "map.h"
#include "mapmgr.h"
#include "monster.h"
#include "moongate.h"
#include "player.h"
#include "screen.h"
#include "settings.h"
#include "tile.h"
#include "utils.h"

SpellEffectCallback spellEffectCallback = NULL;

void spellMagicAttack(MapTile tile, Direction dir, int minDamage, int maxDamage);
bool spellMagicAttackAtCoord(MapCoords coords, int distance, void *data);

static int spellAwaken(int player);
static int spellBlink(int dir);
static int spellCure(int player);
static int spellDispel(int dir);
static int spellEField(int param);
static int spellFireball(int dir);
static int spellGate(int phase);
static int spellHeal(int player);
static int spellIceball(int dir);
static int spellJinx(int unused);
static int spellKill(int dir);
static int spellLight(int unused);
static int spellMMissle(int dir);
static int spellNegate(int unused);
static int spellOpen(int unused);
static int spellProtect(int unused);
static int spellRez(int player);
static int spellQuick(int unused);
static int spellSleep(int unused);
static int spellTremor(int unused);
static int spellUndead(int unused);
static int spellView(int unsued);
static int spellWinds(int fromdir);
static int spellXit(int unused);
static int spellYup(int unused);
static int spellZdown(int unused);

/* masks for reagents */
#define ASH (1 << REAG_ASH)
#define GINSENG (1 << REAG_GINSENG)
#define GARLIC (1 << REAG_GARLIC)
#define SILK (1 << REAG_SILK)
#define MOSS (1 << REAG_MOSS)
#define PEARL (1 << REAG_PEARL)
#define NIGHTSHADE (1 << REAG_NIGHTSHADE)
#define MANDRAKE (1 << REAG_MANDRAKE)

/* spell error messages */
static const struct {
    SpellCastError err;
    const char *msg;
} spellErrorMsgs[] = {
    { CASTERR_NOMIX, "None Mixed!\n" },        
    { CASTERR_MPTOOLOW, "Not Enough MP!\n" },
    { CASTERR_FAILED, "Failed!\n" },
    { CASTERR_WRONGCONTEXT, "Not here!\n" },
    { CASTERR_COMBATONLY, "Combat only!\nFailed!\n" },
    { CASTERR_DUNGEONONLY, "Dungeon only!\nFailed!\n" },
    { CASTERR_WORLDMAPONLY, "Outdoors only!\nFailed!\n" }
};

const Spell spells[] = {
    { "Awaken",       GINSENG | GARLIC,         CTX_ANY,        TRANSPORT_ANY,  &spellAwaken,  SPELLPRM_PLAYER,  5 },
    { "Blink",        SILK | MOSS,              CTX_WORLDMAP,   TRANSPORT_FOOT_OR_HORSE,
                                                                                &spellBlink,   SPELLPRM_DIR,     15 },
    { "Cure",         GINSENG | GARLIC,         CTX_ANY,        TRANSPORT_ANY,  &spellCure,    SPELLPRM_PLAYER,  5 },
    { "Dispell",      ASH | GARLIC | PEARL,     CTX_ANY,        TRANSPORT_ANY,  &spellDispel,  SPELLPRM_DIR,     20 },
    { "Energy Field", ASH | SILK | PEARL,       (LocationContext)(CTX_COMBAT | CTX_DUNGEON),
                                                                TRANSPORT_ANY,  &spellEField,  SPELLPRM_TYPEDIR, 10 },
    { "Fireball",     ASH | PEARL,              CTX_COMBAT,     TRANSPORT_ANY,  &spellFireball,SPELLPRM_DIR,     15 },
    { "Gate",         ASH | PEARL | MANDRAKE,   CTX_WORLDMAP,   TRANSPORT_FOOT_OR_HORSE,
                                                                                &spellGate,    SPELLPRM_PHASE,   40 },
    { "Heal",         GINSENG | SILK,           CTX_ANY,        TRANSPORT_ANY,  &spellHeal,    SPELLPRM_PLAYER,  10 },
    { "Iceball",      PEARL | MANDRAKE,         CTX_COMBAT,     TRANSPORT_ANY,  &spellIceball, SPELLPRM_DIR,     20 },
    { "Jinx",         PEARL | NIGHTSHADE | MANDRAKE,
                                                CTX_COMBAT,     TRANSPORT_ANY,  &spellJinx,    SPELLPRM_NONE,    30 },
    { "Kill",         PEARL | NIGHTSHADE,       CTX_COMBAT,     TRANSPORT_ANY,  &spellKill,    SPELLPRM_DIR,     25 },
    { "Light",        ASH,                      CTX_DUNGEON,    TRANSPORT_ANY,  &spellLight,   SPELLPRM_NONE,    5 },
    { "Magic missile", ASH | PEARL,             CTX_COMBAT,     TRANSPORT_ANY,  &spellMMissle, SPELLPRM_DIR,     5 },
    { "Negate",       ASH | GARLIC | MANDRAKE,  CTX_ANY,        TRANSPORT_ANY,  &spellNegate,  SPELLPRM_NONE,    20 },
    { "Open",         ASH | MOSS,               CTX_ANY,        TRANSPORT_ANY,  &spellOpen,    SPELLPRM_NONE,    5 },
    { "Protection",   ASH | GINSENG | GARLIC,   CTX_ANY,        TRANSPORT_ANY,  &spellProtect, SPELLPRM_NONE,    15 },
    { "Quickness",    ASH | GINSENG | MOSS,     CTX_ANY,        TRANSPORT_ANY,  &spellQuick,   SPELLPRM_NONE,    20 },
    { "Resurrect",    ASH | GINSENG | GARLIC | SILK | MOSS | MANDRAKE, 
                                                CTX_ANY,        TRANSPORT_ANY,  &spellRez,     SPELLPRM_PLAYER,  45 },
    { "Sleep",        SILK | GINSENG,           CTX_COMBAT,     TRANSPORT_ANY,  &spellSleep,   SPELLPRM_NONE,    15 },
    { "Tremor",       ASH | MOSS | MANDRAKE,    CTX_COMBAT,     TRANSPORT_ANY,  &spellTremor,  SPELLPRM_NONE,    30 },
    { "Undead",       ASH | GARLIC,             CTX_COMBAT,     TRANSPORT_ANY,  &spellUndead,  SPELLPRM_NONE,    15 },
    { "View",         NIGHTSHADE | MANDRAKE,    CTX_NON_COMBAT, TRANSPORT_ANY,  &spellView,    SPELLPRM_NONE,    15 },
    { "Winds",        ASH | MOSS,               CTX_WORLDMAP,   TRANSPORT_ANY,  &spellWinds,   SPELLPRM_FROMDIR, 10 },
    { "X-it",         ASH | SILK | MOSS,        CTX_DUNGEON,    TRANSPORT_ANY,  &spellXit,     SPELLPRM_NONE,    15 },
    { "Y-up",         SILK | MOSS,              CTX_DUNGEON,    TRANSPORT_ANY,  &spellYup,     SPELLPRM_NONE,    10 },
    { "Z-down",       SILK | MOSS,              CTX_DUNGEON,    TRANSPORT_ANY,  &spellZdown,   SPELLPRM_NONE,    5 }
};

#define N_SPELLS (sizeof(spells) / sizeof(spells[0]))

void playerSetSpellEffectCallback(SpellEffectCallback callback) {
    spellEffectCallback = callback;    
}

Mixture *mixtureNew() {
    Mixture *mix = new Mixture;
    memset(mix, 0, sizeof(Mixture));
    return mix;
}

void mixtureDelete(Mixture *mix) {
    memset(mix, 0, sizeof(Mixture));
    delete mix;
}

int mixtureAddReagent(Mixture *mix, Reagent reagent) {
    ASSERT(reagent < REAG_MAX, "invalid reagent: %d", reagent);
    if (c->saveGame->reagents[reagent] < 1)
        return 0;
    c->saveGame->reagents[reagent]--;
    mix->reagents[reagent]++;
    return 1;
}

int mixtureRemoveReagent(Mixture *mix, Reagent reagent) {
    ASSERT(reagent < REAG_MAX, "invalid reagent: %d", reagent);
    if (mix->reagents[reagent] == 0)
        return 0;
    c->saveGame->reagents[reagent]++;
    mix->reagents[reagent]--;
    return 1;
}

void mixtureRevert(Mixture *mix) {
    int reg;

    for (reg = 0; reg < REAG_MAX; reg++) {
        c->saveGame->reagents[reg] += mix->reagents[reg];
        mix->reagents[reg] = 0;
    }
}

const char *spellGetName(unsigned int spell) {
    ASSERT(spell < N_SPELLS, "invalid spell: %d", spell);

    return spells[spell].name;
}

int spellGetRequiredMP(unsigned int spell) {
    ASSERT(spell < N_SPELLS, "invalid spell: %d", spell);
    
    return spells[spell].mp;
}

LocationContext spellGetContext(unsigned int spell) {
    ASSERT(spell < N_SPELLS, "invalid spell: %d", spell);

    return spells[spell].context;
}

TransportContext spellGetTransportContext(unsigned int spell) {
    ASSERT(spell < N_SPELLS, "invalid spell: %d", spell);

    return spells[spell].transportContext;
}

string spellGetErrorMessage(unsigned int spell, SpellCastError error) {
    unsigned int i;
    SpellCastError err = error;

    /* try to find a more specific error message */
    if (err == CASTERR_WRONGCONTEXT) {
        switch(spells[spell].context) {
            case CTX_COMBAT: err = CASTERR_COMBATONLY; break;
            case CTX_DUNGEON: err = CASTERR_DUNGEONONLY; break;
            case CTX_WORLDMAP: err = CASTERR_WORLDMAPONLY; break;
            default: break;
        }
    }

    /* find the message that we're looking for and return it! */
    for (i = 0; i < sizeof(spellErrorMsgs) / sizeof(spellErrorMsgs[0]); i++) {
        if (err == spellErrorMsgs[i].err)
            return string(spellErrorMsgs[i].msg);
    }

    return string();
}

/**
 * Mix reagents for a spell.  Fails and returns false if the reagents
 * selected were not correct.
 */
int spellMix(unsigned int spell, const Mixture *mix) {
    int regmask, reg;

    ASSERT(spell < N_SPELLS, "invalid spell: %d", spell);

    regmask = 0;
    for (reg = 0; reg < REAG_MAX; reg++) {
        if (mix->reagents[reg] > 0)
            regmask |= (1 << reg);
    }

    if (regmask != spells[spell].components)
        return 0;

    c->saveGame->mixtures[spell]++;

    return 1;
}

SpellParam spellGetParamType(unsigned int spell) {
    ASSERT(spell < N_SPELLS, "invalid spell: %d", spell);

    return spells[spell].paramType;
}

/**
 * Casts spell.  Fails and returns false if no mixture is available,
 * the character doesn't have enough magic points, or the context is
 * invalid.  The error code is updated with the reason for failure.
 */
int spellCast(unsigned int spell, int character, int param, SpellCastError *error, int spellEffect) {
    int player = (spells[spell].paramType == SPELLPRM_PLAYER) ? param : -1;
    
    ASSERT(spell < N_SPELLS, "invalid spell: %d", spell);
    ASSERT(character >= 0 && character < c->saveGame->members, "character out of range: %d", character);

    *error = CASTERR_NOERROR;

    if (c->saveGame->mixtures[spell] == 0) {
        *error = CASTERR_NOMIX;
        return 0;
    }

    /* subtract the mixture for even trying to cast the spell */
    c->saveGame->mixtures[spell]--;
        
    if ((c->location->context & spells[spell].context) == 0) {
        *error = CASTERR_WRONGCONTEXT;        
        return 0;
    }

    if ((c->transportContext & spells[spell].transportContext) == 0) {
        *error = CASTERR_FAILED;
        return 0;
    }

    if (c->players[character].mp < spells[spell].mp) {
        *error = CASTERR_MPTOOLOW;
        return 0;
    }    

    /* If there's a negate magic aura, spells fail! */
    if (c->aura != AURA_NEGATE) {    

        /* subtract the mp needed for the spell */
        c->players[character].mp -= spells[spell].mp;

        if (spellEffect)
            (*spellEffectCallback)(spell + 'a', player, SOUND_MAGIC);
    
        if (!(*spells[spell].spellFunc)(param)) {
            *error = CASTERR_FAILED;
            return 0;
        }    
    } else {
        *error = CASTERR_FAILED;
        return 0;
    }

    return 1;
}

/**
 * Makes a special magic ranged attack in the given direction
 */
MapTile spellMagicAttackTile;
int spellMagicAttackDamage;

void spellMagicAttack(MapTile tile, Direction dir, int minDamage, int maxDamage) {
    CoordActionInfo *info;

    spellMagicAttackDamage = ((minDamage >= 0) && (minDamage < maxDamage)) ?
        xu4_random((maxDamage + 1) - minDamage) + minDamage :
        maxDamage;

    spellMagicAttackTile = tile;
        
    /* setup the spell */
    info = new CoordActionInfo;
    info->handleAtCoord = &spellMagicAttackAtCoord;
    info->origin = combatInfo.party[FOCUS].obj->getCoords();
    info->prev = MapCoords(-1, -1);
    info->range = 11;
    info->validDirections = MASK_DIR_ALL;
    info->player = FOCUS;
    info->blockedPredicate = &tileCanAttackOver;
    info->blockBefore = 1;
    info->firstValidDistance = 1;
    info->dir = MASK_DIR(dir);

    gameDirectionalAction(info);
    delete info;
}

bool spellMagicAttackAtCoord(MapCoords coords, int distance, void *data) {
    int monster;
    CoordActionInfo* info = (CoordActionInfo*)data;
    MapCoords old = info->prev;
    int attackdelay = MAX_BATTLE_SPEED - settings.battleSpeed;    
    
    info->prev = coords;

    /* Remove the last weapon annotation left behind */
    if ((distance > 0) && (old.x >= 0) && (old.y >= 0))
        c->location->map->annotations->remove(old, spellMagicAttackTile);

    /* Check to see if we might hit something */
    if (coords.x != -1 && coords.y != -1) {

        monster = combatMonsterAt(coords);

        if (monster == -1) {
            c->location->map->annotations->add(coords, spellMagicAttackTile, true);
            gameUpdateScreen();
        
            /* Based on attack speed setting in setting struct, make a delay for
               the attack annotation */
            if (attackdelay > 0)
                eventHandlerSleep(attackdelay * 2);

            return 0;
        }
        else {
            /* show the 'hit' tile */
            attackFlash(coords, spellMagicAttackTile, 3);

            /* apply the damage to the monster */
            combatApplyDamageToMonster(monster, spellMagicAttackDamage, FOCUS);
        }
    }

    return 1;
}

static int spellAwaken(int player) {
    ASSERT(player < 8, "player out of range: %d", player);

    if (player < c->saveGame->members && 
        c->players[player].status == STAT_SLEEPING) {
        
        /* restore the party member to their original state */
        if (combatInfo.party[player].status)
            c->players[player].status = combatInfo.party[player].status;
        else c->players[player].status = STAT_GOOD;

        /* wake the member up! */
        if (combatInfo.party[player].obj)
            combatInfo.party[player].obj->setTile(tileForClass(c->players[player].klass));

        return 1;
    }

    return 0;
}

static int spellBlink(int dir) {
    int i,        
        failed = 0,
        distance,
        diff,
        *var;
    Direction reverseDir = dirReverse((Direction)dir);
    MapCoords coords = c->location->coords;
    
    /* figure out what numbers we're working with */
    var = (dir & (DIR_WEST | DIR_EAST)) ? &coords.x : &coords.y;
        
    /* find the distance we are going to move */
    distance = (*var) % 0x10;
    if (dir == DIR_EAST || dir == DIR_SOUTH)
        distance = 0x10 - distance;
    
    /* see if we move another 16 spaces over */
    diff = 0x10 - distance;
    if ((diff > 0) && (xu4_random(diff * diff) > distance))
        distance += 0x10;

    /* test our distance, and see if it works */
    for (i = 0; i < distance; i++)
        coords.move((Direction)dir, c->location->map);    
    
    i = distance;   
    /* begin walking backward until you find a valid spot */
    while ((i-- > 0) && !tileIsWalkable((*c->location->tileAt)(c->location->map, coords, WITH_OBJECTS)))
        coords.move(reverseDir, c->location->map);
    
    if (tileIsWalkable((*c->location->tileAt)(c->location->map, coords, WITH_OBJECTS))) {
        /* we didn't move! */
        if (c->location->coords == coords)
            failed = 1;

        c->location->coords = coords;
    } else failed = 1;    

    return (failed ? 0 : 1);
}

static int spellCure(int player) {
    ASSERT(player < 8, "player out of range: %d", player);

    return playerHeal(HT_CURE, player);
}

static int spellDispel(int dir) {    
    MapTile tile, newTile;    
    MapCoords field;

    /* 
     * get the location of the avatar (or current party member, if in battle)
     */
    locationGetCurrentPosition(c->location, &field);        

    /*
     * find where we want to dispel the field
     */
    field.move((Direction)dir, c->location->map);    

    /*
     * get a replacement tile for the field
     */
    newTile = locationGetReplacementTile(c->location, field);

    /*
     * if there is a field annotation, remove it and replace it with a valid
     * replacement annotation.  We do this because sometimes dungeon triggers
     * create annotations, that, if just removed, leave a wall tile behind
     * (or other unwalkable surface).  So, we need to provide a valid replacement
     * annotation to fill in the gap :)
     */
    AnnotationList a = c->location->map->annotations->allAt(field);
    if (a.size() > 0) {
        AnnotationList::iterator i;
        for (i = a.begin(); i != a.end(); i++) {            
            if (tileCanDispel(i->getTile())) {
                c->location->map->annotations->remove(&*i);
                c->location->map->annotations->add(field, newTile);
                break;
            }                
        }
    }    

    /*
     * if the map tile itself is a field, overlay it with a replacement tile
     */
    tile = (*c->location->tileAt)(c->location->map, field, WITHOUT_OBJECTS);    
    if (!tileCanDispel(tile))
        return 0;
    
    c->location->map->annotations->add(field, newTile);

    return 1;
}

static int spellEField(int param) {    
    MapTile fieldTile;
    int fieldType;
    int dir;
    MapTile tile;    
    MapCoords coords;
    
    /* Unpack fieldType and direction */
    fieldType = param >> 4;
    dir = param & 0xF;
    
    /* Make sure params valid */
    switch (fieldType) {
        case ENERGYFIELD_FIRE: fieldTile = FIREFIELD_TILE; break;
        case ENERGYFIELD_LIGHTNING: fieldTile = LIGHTNINGFIELD_TILE; break;
        case ENERGYFIELD_POISON: fieldTile = POISONFIELD_TILE; break;
        case ENERGYFIELD_SLEEP: fieldTile = SLEEPFIELD_TILE; break;
        default: return 0; break;
    }

    locationGetCurrentPosition(c->location, &coords);        
    
    coords.move((Direction)dir, c->location->map);    
    if (MAP_IS_OOB(c->location->map, coords))
        return 0;
    else {
        /*
         * Observed behaviour on Amiga version of Ultima IV:
         * Field cast on other field: Works, unless original field is lightning
         * in which case it doesn't.
         * Field cast on monster: Works, monster remains the visible tile
         * Field cast on top of field and then dispel = no fields left
         * The code below seems to produce this behaviour.
         */
        tile = (*c->location->tileAt)(c->location->map, coords, WITH_GROUND_OBJECTS);
        if (!tileIsWalkable(tile)) return 0;
        
        /* Get rid of old field, if any */
        AnnotationList a = c->location->map->annotations->allAt(coords);
        if (a.size() > 0) {
            AnnotationList::iterator i;
            for (i = a.begin(); i != a.end(); i++) {                
                if (tileCanDispel(i->getTile()))
                    c->location->map->annotations->remove(&*i);
            }
        }     
            
        c->location->map->annotations->add(coords, fieldTile);
    }

    return 1;
}

static int spellFireball(int dir) {
    spellMagicAttack(HITFLASH_TILE, (Direction)dir, 24, 128);    
    return 1;
}

static int spellGate(int phase) {
    const MapCoords *moongate;

    moongate = moongateGetGateCoordsForPhase(phase);
    if (moongate) 
        c->location->coords = *moongate;

    return 1;    
}

static int spellHeal(int player) {
    ASSERT(player < 8, "player out of range: %d", player);

    playerHeal(HT_HEAL, player);
    return 1;
}

static int spellIceball(int dir) {
    spellMagicAttack(MAGICFLASH_TILE, (Direction)dir, 32, 224);    
    return 1;
}

static int spellJinx(int unused) {
    c->aura = AURA_JINX;
    c->auraDuration = 10;
    return 1;
}

static int spellKill(int dir) {
    spellMagicAttack(WHIRLPOOL_TILE, (Direction)dir, -1, 232);
    return 1;
}

static int spellLight(int unused) {
    c->saveGame->torchduration += 100;
    return 1;
}

static int spellMMissle(int dir) {
    spellMagicAttack(MISSFLASH_TILE, (Direction)dir, 64, 16);
    return 1;
}

static int spellNegate(int unused) {
    c->aura = AURA_NEGATE;
    c->auraDuration = 10;
    return 1;
}

static int spellOpen(int unused) {    
    gameGetChest(-1);
    return 1;
}

static int spellProtect(int unused) {
    c->aura = AURA_PROTECTION;
    c->auraDuration = 10;
    return 1;
}

static int spellRez(int player) {
    ASSERT(player < 8, "player out of range: %d", player);

    return playerHeal(HT_RESURRECT, player);
}

static int spellQuick(int unused) {
    c->aura = AURA_QUICKNESS;
    c->auraDuration = 10;
    return 1;
}

static int spellSleep(int unused) {
    int i;

    /* try to put each monster to sleep */
    for (i = 0; i < AREA_MONSTERS; i++) { 
        if (combatInfo.monsters[i].obj) {
            if ((combatInfo.monsters[i].obj->monster->resists != EFFECT_SLEEP) &&
                xu4_random(0xFF) >= combatInfo.monsters[i].hp) {
                combatInfo.monsters[i].status = STAT_SLEEPING;
                combatInfo.monsters[i].obj->setAnimated(false); /* freeze monster */
            }
        }
    }

    return 1;
}

static int spellTremor(int unused) {
    int i;        
    MapCoords coords;

    for (i = 0; i < AREA_MONSTERS; i++) {        
        /* make sure we have a valid monster */
        if (!combatInfo.monsters[i].obj)
            continue;
        /* monsters with over 192 hp are unaffected */
        else if (combatInfo.monsters[i].hp > 192)
            continue;
        else {
            coords = combatInfo.monsters[i].obj->getCoords();

            /* Deal maximum damage to monster */
            if (xu4_random(2) == 0) {
                combatApplyDamageToMonster(i, 0xFF, FOCUS);
                attackFlash(coords, HITFLASH_TILE, 1);
            }
            /* Deal enough damage to monster to make it flee */
            else if (xu4_random(2) == 0) {
                if (combatInfo.monsters[i].hp > 23)
                    combatApplyDamageToMonster(i, combatInfo.monsters[i].hp-23, FOCUS);                
                attackFlash(coords, HITFLASH_TILE, 1);
            }
        }
    }
    
    return 1;
}

static int spellUndead(int unused) {    
    int i;
    
    for (i = 0; i < AREA_MONSTERS; i++) {
        /* Deal enough damage to undead to make them flee */
        if (combatInfo.monsters[i].obj && monsterIsUndead(combatInfo.monsters[i].obj->monster) && xu4_random(2) == 0)
            combatInfo.monsters[i].hp = 23;        
    }
    
    return 1;
}

static int spellView(int unsued) {    
    gamePeerGem();
    return 1;
}

static int spellWinds(int fromdir) {
    c->windDirection = fromdir;
    return 1;
}

static int spellXit(int unused) {
    if (!mapIsWorldMap(c->location->map)) {
        screenMessage("Leaving...\n");
        gameExitToParentMap();
        musicPlay();
        return 1;
    }
    return 0;
}

static int spellYup(int unused) {
    MapTile tile;
    int i;
    MapCoords coords = c->location->coords;

    /* can't cast in the Abyss */
    if (c->location->map->id == MAP_ABYSS)
        return 0;
    /* staying in the dungeon */
    else if (coords.z > 0) {
        for (i = 0; i < 0x20; i++) {
            coords = MapCoords(xu4_random(8), xu4_random(8), coords.z - 1);
            tile = (*c->location->tileAt)(c->location->map, coords, WITH_OBJECTS);

            if (dungeonTokenForTile(tile) == DUNGEON_CORRIDOR) {
                c->location->coords = coords;
                return 1;
            }
        }
    /* exiting the dungeon */
    } else {
        screenMessage("Leaving...\n");
        gameExitToParentMap();
        musicPlay();
        return 1;
    }
    
    /* didn't find a place to go, failed! */
    return 0;
}

static int spellZdown(int unused) {
    MapTile tile;
    int i;
    MapCoords coords = c->location->coords;
    
    /* can't cast in the Abyss */
    if (c->location->map->id == MAP_ABYSS)
        return 0;
    /* can't go lower than level 8 */
    else if (coords.z >= 7)
        return 0;
    else {
        for (i = 0; i < 0x20; i++) {
            coords = MapCoords(xu4_random(8), xu4_random(8), coords.z + 1);
            tile = (*c->location->tileAt)(c->location->map, coords, WITH_OBJECTS);

            if (dungeonTokenForTile(tile) == DUNGEON_CORRIDOR) {
                c->location->coords = coords;
                return 1;
            }
        }
    }
    
    /* didn't find a place to go, failed! */
    return 0;
}
