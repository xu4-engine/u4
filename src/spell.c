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
#include "event.h"
#include "game.h"
#include "location.h"
#include "monster.h"
#include "moongate.h"
#include "player.h"
#include "screen.h"     /* for spells not implemented yet, should be removable when implemented */
#include "settings.h"
#include "ttype.h"

SpellEffectCallback spellEffectCallback = NULL;

void spellMagicAttack(unsigned char tile, Direction dir, int minDamage, int maxDamage);
int spellMagicAttackAtCoord(int x, int y, int distance, void *data);

static int spellAwaken(int player);
static int spellBlink(int dir);
static int spellCure(int player);
static int spellDispel(int dir);
static int spellEField(int dir);
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
    { "Energy Field", ASH | SILK | PEARL,       CTX_COMBAT | CTX_DUNGEON,
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
    Mixture *mix = (Mixture *) malloc(sizeof(Mixture));
    memset(mix, 0, sizeof(Mixture));
    return mix;
}

void mixtureDelete(Mixture *mix) {
    memset(mix, 0, sizeof(Mixture));
    free(mix);
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

const char *spellGetErrorMessage(unsigned int spell, SpellCastError error) {
    int i;
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
            return spellErrorMsgs[i].msg;
    }

    return NULL;
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

    if (c->saveGame->players[character].mp < spells[spell].mp) {
        *error = CASTERR_MPTOOLOW;
        return 0;
    }    

    /* If there's a negate magic aura, spells fail! */
    if (c->aura != AURA_NEGATE) {    

        /* subtract the mp needed for the spell */
        c->saveGame->players[character].mp -= spells[spell].mp;

        if (spellEffect)
            (*spellEffectCallback)(spell + 'a', player, 1);
    
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
unsigned char spellMagicAttackTile;
int spellMagicAttackDamage;

void spellMagicAttack(unsigned char tile, Direction dir, int minDamage, int maxDamage) {
    extern CombatInfo combatInfo;        
    CoordActionInfo *info;

    spellMagicAttackDamage = ((minDamage >= 0) && (minDamage < maxDamage)) ?
        rand() % ((maxDamage + 1) - minDamage) + minDamage :
        maxDamage;

    spellMagicAttackTile = tile;
        
    /* setup the spell */
    info = (CoordActionInfo *) malloc(sizeof(CoordActionInfo));
    info->handleAtCoord = &spellMagicAttackAtCoord;
    info->origin_x = combatInfo.party[FOCUS].obj->x;
    info->origin_y = combatInfo.party[FOCUS].obj->y;
    info->prev_x = info->prev_y = -1;
    info->range = 11;
    info->validDirections = MASK_DIR_ALL;
    info->player = FOCUS;
    info->blockedPredicate = &tileCanAttackOver;
    info->blockBefore = 1;
    info->firstValidDistance = 1;
    info->dir = MASK_DIR(dir);

    gameDirectionalAction(info);
    free(info);
}

int spellMagicAttackAtCoord(int x, int y, int distance, void *data) {
    int monster;
//    int i;
    extern CombatInfo combatInfo;
    CoordActionInfo* info = (CoordActionInfo*)data;
    int oldx = info->prev_x,
        oldy = info->prev_y;
    int attackdelay = MAX_BATTLE_SPEED - settings->battleSpeed;    
    
    info->prev_x = x;
    info->prev_y = y;

    /* Remove the last weapon annotation left behind */
    if ((distance > 0) && (oldx >= 0) && (oldy >= 0))
        annotationRemove(oldx, oldy, c->location->z, c->location->map->id, spellMagicAttackTile);

    /* Check to see if we might hit something */
    if (x != -1 && y != -1) {

        monster = combatMonsterAt(x, y, c->location->z);        

        if (monster == -1) {
            annotationSetVisual(annotationAddTemporary(x, y, c->location->z, c->location->map->id, spellMagicAttackTile));
            gameUpdateScreen();
        
            /* Based on attack speed setting in setting struct, make a delay for
               the attack annotation */
            if (attackdelay > 0)
                eventHandlerSleep(attackdelay * 2);

            return 0;
        }
        else {
            /* show the 'hit' tile */
            attackFlash(x, y, spellMagicAttackTile, 3);

            /* apply the damage to the monster */
            combatApplyDamageToMonster(monster, spellMagicAttackDamage, FOCUS);
        }
    }

    return 1;
}

static int spellAwaken(int player) {
    extern CombatInfo combatInfo;
    ASSERT(player < 8, "player out of range: %d", player);

    if (player < c->saveGame->members && 
        c->saveGame->players[player].status == STAT_SLEEPING) {
        
        /* restore the party member to their original state */
        if (combatInfo.party[player].status)
            c->saveGame->players[player].status = combatInfo.party[player].status;
        else c->saveGame->players[player].status = STAT_GOOD;

        /* wake the member up! */
        if (combatInfo.party[player].obj)
            combatInfo.party[player].obj->tile = tileForClass(c->saveGame->players[player].klass);        

        return 1;
    }

    return 0;
}

static int spellBlink(int dir) {
    int i,
        x = c->location->x,
        y = c->location->y,        
        failed = 0,
        distance,
        diff,
        *var;
    Direction reverseDir = dirReverse(dir);
    
    /* figure out what numbers we're working with */
    var = (dir & (DIR_WEST | DIR_EAST)) ? &x : &y;
        
    /* find the distance we are going to move */
    distance = (*var) % 0x10;
    if (dir == DIR_EAST || dir == DIR_SOUTH)
        distance = 0x10 - distance;
    
    /* see if we move another 16 spaces over */
    diff = 0x10 - distance;
    if ((diff > 0) && (rand() % (diff * diff) > distance))
        distance += 0x10;

    /* test our distance, and see if it works */
    for (i = 0; i < distance; i++) {
        mapDirMove(c->location->map, dir, &x, &y);        
    }    
    
    i = distance;   
    /* begin walking backward until you find a valid spot */
    while ((i-- > 0) && !tileIsWalkable(mapGroundTileAt(c->location->map, x, y, c->location->z)))
        mapDirMove(c->location->map, reverseDir, &x, &y);
    
    if (tileIsWalkable(mapGroundTileAt(c->location->map, x, y, c->location->z))) {
        /* we didn't move! */
        if (c->location->x == x && c->location->y == y)
            failed = 1;

        c->location->x = x;
        c->location->y = y;
    } else failed = 1;    

    return (failed ? 0 : 1);
}

static int spellCure(int player) {
    ASSERT(player < 8, "player out of range: %d", player);

    return playerHeal(c->saveGame, HT_CURE, player);
}

static int spellDispel(int dir) {
    int x, y, z;
    unsigned char tile;
    const Annotation *a;
    extern CombatInfo combatInfo;
    
    /* get the current location, based on context */
    if (c->location->context == CTX_COMBAT) {
        x = combatInfo.party[FOCUS].obj->x;
        y = combatInfo.party[FOCUS].obj->y;
        z = combatInfo.party[FOCUS].obj->z;
    }
    else {
        x = c->location->x;
        y = c->location->y;
        z = c->location->z;
    }

    mapDirMove(c->location->map, (Direction) dir, &x, &y);
    if (MAP_IS_OOB(c->location->map, x, y))
        return 0;

    /*
     * if there is a field annotation, remove it
     */
    a = annotationAt(x, y, z, c->location->map->id);
    if (a && tileCanDispel(a->tile)) {
        annotationRemove(x, y, z, c->location->map->id, a->tile);
        return 1;
    }

    /*
     * if the map tile itself is a field, overlay it with a brick
     * annotation
     */
    tile = (c->location->context & ~CTX_DUNGEON) ?
        mapTileAt(c->location->map, x, y, z) :
        mapDungeonTileAt(c->location->map, x, y, z);
    
    if (!tileCanDispel(tile))
        return 0;
    
    annotationAdd(x, y, z, c->location->map->id, BRICKFLOOR_TILE);

    return 1;
}

static int spellEField(int dir) {
    int x, y, z;
    extern CombatInfo combatInfo;

    z = c->location->z;

    if (c->location->context == CTX_DUNGEON) {
        x = c->location->x;
        y = c->location->y;        
    } else {
        x = combatInfo.party[FOCUS].obj->x;
        y = combatInfo.party[FOCUS].obj->y;
    }

    dirMove((Direction) dir, &x, &y);
    if (MAP_IS_OOB(c->location->map, x, y))
        return 0;
    
    annotationAdd(x, y, z, c->location->map->id, LIGHTNINGFIELD_TILE);

    return 1;
}

static int spellFireball(int dir) {
    spellMagicAttack(HITFLASH_TILE, (Direction)dir, 24, 128);    
    return 1;
}

static int spellGate(int phase) {
    const Moongate *moongate;

    moongate = moongateGetGateForPhase(phase);
    c->location->x = moongate->x;
    c->location->y = moongate->y;        

    return 1;    
}

static int spellHeal(int player) {
    ASSERT(player < 8, "player out of range: %d", player);

    return playerHeal(c->saveGame, HT_HEAL, player);
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

    return playerHeal(c->saveGame, HT_RESURRECT, player);
}

static int spellQuick(int unused) {
    c->aura = AURA_QUICKNESS;
    c->auraDuration = 10;
    return 1;
}

static int spellSleep(int unused) {
    extern CombatInfo combatInfo;
    int i;

    /* try to put each monster to sleep */
    for (i = 0; i < AREA_MONSTERS; i++) { 
        if (combatInfo.monsters[i].obj) {
            if ((combatInfo.monsters[i].obj->monster->resists != EFFECT_SLEEP) &&
                ((rand() % 0xFF) >= combatInfo.monsters[i].hp)) {
                combatInfo.monsters[i].status = STAT_SLEEPING;
                combatInfo.monsters[i].obj->canAnimate = 0; /* freeze monster */
            }
        }
    }

    return 1;
}

static int spellTremor(int unused) {
    extern CombatInfo combatInfo;
    int i, x, y;        

    for (i = 0; i < AREA_MONSTERS; i++) {
        /* FIXME: monster selection (50/50) guessed */
        if (combatInfo.monsters[i].obj && (rand() % 2 != 0)) {               

            x = combatInfo.monsters[i].obj->x;
            y = combatInfo.monsters[i].obj->y;
            annotationSetVisual(annotationAddTemporary(x, y, c->location->z, c->location->map->id, HITFLASH_TILE));
            
            eventHandlerSleep(50);
            gameUpdateScreen();

            /* FIXME: damage guessed */
            combatApplyDamageToMonster(i, rand() % 0xFF, FOCUS);

            annotationRemove(x, y, c->location->z, c->location->map->id, HITFLASH_TILE);
        }
    }
    
    return 1;
}

static int spellUndead(int unused) {    
    int i;
    extern CombatInfo combatInfo;
    
    for (i = 0; i < AREA_MONSTERS; i++) {
        /* Deal enough damage to undead to make them flee */
        if (combatInfo.monsters[i].obj && monsterIsUndead(combatInfo.monsters[i].obj->monster) && (rand() % 2 == 0))
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
        gameExitToParentMap(c);
        musicPlay();
        return 1;
    }
    return 0;
}

static int spellYup(int unused) {
    unsigned char tile = mapDungeonTileAt(c->location->map, c->location->x, c->location->y, c->location->z - 1);

    if (c->location->z > 0) {
        if (tileIsDungeonWalkable(tile))
            c->location->z--;
        else return 0;
    } else {
        screenMessage("Leaving...\n");
        gameExitToParentMap(c);
        musicPlay();
    }    
    
    return 1;
}

static int spellZdown(int unused) {
    unsigned char tile = mapDungeonTileAt(c->location->map, c->location->x, c->location->y, c->location->z + 1);

    if (c->location->z < 7) {
        if (tileIsDungeonWalkable(tile))
            c->location->z++;
        else return 0;
    } else return 0;
    
    return 1;
}
