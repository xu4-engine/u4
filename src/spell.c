/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "u4.h"
#include "spell.h"
#include "direction.h"
#include "context.h"
#include "map.h"
#include "annotation.h"
#include "ttype.h"
#include "screen.h"

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

/* spell context flags */
#define SC_NORMAL  (1 << 0)
#define SC_DUNGEON (1 << 1)
#define SC_COMBAT  (1 << 2)
#define SC_ANY (SC_NORMAL | SC_DUNGEON | SC_COMBAT)

const Spell spells[] = {
    { "Awaken",       GINSENG | GARLIC,         SC_ANY,      &spellAwaken,  SPELLPRM_PLAYER,  5 },
    { "Blink",        SILK | MOSS,              SC_NORMAL,   &spellBlink,   SPELLPRM_DIR,     15 },
    { "Cure",         GINSENG | GARLIC,         SC_ANY,      &spellCure,    SPELLPRM_PLAYER,  5 },
    { "Dispel",       ASH | GARLIC | PEARL,     SC_ANY,      &spellDispel,  SPELLPRM_DIR,     20 },
    { "Energy Field", ASH | SILK | PEARL,       SC_ANY,      &spellEField,  SPELLPRM_TYPEDIR, 10 },
    { "Fireball",     ASH | PEARL,              SC_COMBAT,   &spellFireball,SPELLPRM_DIR,     15 },
    { "Gate",         ASH | PEARL | MANDRAKE,   SC_NORMAL,   &spellGate,    SPELLPRM_PHASE,   40 },
    { "Heal",         GINSENG | SILK,           SC_ANY,      &spellHeal,    SPELLPRM_PLAYER,  10 },
    { "Iceball",      PEARL | MANDRAKE,         SC_COMBAT,   &spellIceball, SPELLPRM_DIR,     20 },
    { "Jinx",         PEARL | NIGHTSHADE | MANDRAKE,
                                                SC_COMBAT,   &spellJinx,    SPELLPRM_NONE,    30 },
    { "Kill",         PEARL | NIGHTSHADE,       SC_COMBAT,   &spellKill,    SPELLPRM_DIR,     25 },
    { "Light",        ASH,                      SC_DUNGEON,  &spellLight,   SPELLPRM_NONE,    5 },
    { "Magic missile", ASH | PEARL,             SC_COMBAT,   &spellMMissle, SPELLPRM_DIR,     5 },
    { "Negate",       ASH | GARLIC | MANDRAKE,  SC_COMBAT,   &spellNegate,  SPELLPRM_NONE,    20 },
    { "Open",         ASH | MOSS,               SC_NORMAL,   &spellOpen,    SPELLPRM_NONE,    5 },
    { "Protection",   ASH | GINSENG | GARLIC,   SC_ANY,      &spellProtect, SPELLPRM_NONE,    15 },
    { "Quickness",    ASH | GINSENG | MOSS,     SC_ANY,      &spellQuick,   SPELLPRM_NONE,    20 },
    { "Resurrect",    ASH | GINSENG | GARLIC | SILK | MOSS | MANDRAKE, 
                                                SC_ANY,      &spellRez,     SPELLPRM_PLAYER,  45 },
    { "Sleep",        SILK | GINSENG,           SC_COMBAT,   &spellSleep,   SPELLPRM_NONE,    15 },
    { "Tremor",       ASH | MOSS | MANDRAKE,    SC_COMBAT,   &spellTremor,  SPELLPRM_NONE,    30 },
    { "Undead",       ASH | GARLIC,             SC_ANY,      &spellUndead,  SPELLPRM_NONE,    15 },
    { "View",         NIGHTSHADE | MANDRAKE,    SC_NORMAL | SC_DUNGEON, 
                                                             &spellView,    SPELLPRM_NONE,    15 },
    { "Winds",        ASH | MOSS,               SC_NORMAL,   &spellWinds,   SPELLPRM_FROMDIR, 10 },
    { "X-it",         ASH | SILK | MOSS,        SC_DUNGEON,  &spellXit,     SPELLPRM_NONE,    15 },
    { "Y-up",         SILK | MOSS,              SC_DUNGEON,  &spellYup,     SPELLPRM_NONE,    10 },
    { "Z-down",       SILK | MOSS,              SC_DUNGEON,  &spellZdown,   SPELLPRM_NONE,    5 }
};

#define N_SPELLS (sizeof(spells) / sizeof(spells[0]))

Mixture *mixtureNew() {
    Mixture *mix = (Mixture *) malloc(sizeof(Mixture));
    memset(mix, 0, sizeof(Mixture));
    return mix;
}

void mixtureDelete(Mixture *mix) {
    memset(mix, 0, sizeof(Mixture));
    free(mix);
}

void mixtureAddReagent(Mixture *mix, Reagent reagent) {
    assert(reagent < REAG_MAX);
    c->saveGame->reagents[reagent]--;
    mix->reagents[reagent]++;
}

const char *spellGetName(unsigned int spell) {
    assert(spell < N_SPELLS);

    return spells[spell].name;
}

/**
 * Mix reagents for a spell.  Fails and returns false if the reagents
 * selected were not correct.
 */
int spellMix(unsigned int spell, const Mixture *mix) {
    int regmask, reg;

    assert(spell < N_SPELLS);

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
    assert(spell < N_SPELLS);

    return spells[spell].paramType;
}

/**
 * Casts spell.  Fails and returns false if no mixture is available,
 * the character doesn't have enough magic points, or the context is
 * invalid.  The error code is updated with the reason for failure.
 */
int spellCast(unsigned int spell, int character, int param, SpellCastError *error) {
    assert(spell < N_SPELLS);
    assert(character >= 0 && character < c->saveGame->members);

    *error = CASTERR_NOERROR;

    if (c->saveGame->mixtures[spell] == 0) {
        *error = CASTERR_NOMIX;
        return 0;
    }
        
    /* 
     * FIXME: handle dungeon and combat contexts when they are
     * implemented 
     */
    if ((spells[spell].context | SC_NORMAL) == 0) {
        *error = CASTERR_WRONGCONTEXT;
        return 0;
    }

    if (c->saveGame->players[character].mp < spells[spell].mp) {
        *error = CASTERR_MPTOOLOW;
        return 0;
    }

    c->saveGame->mixtures[spell]--;
    c->saveGame->players[character].mp -= spells[spell].mp;

    if (!(*spells[spell].spellFunc)(param)) {
        *error = CASTERR_FAILED;
        return 0;
    }

    return 1;
}

static int spellAwaken(int player) {
    assert(player < 8);

    if (player < c->saveGame->members && 
        c->saveGame->players[player].status == STAT_SLEEPING) {
        c->saveGame->players[player].status = STAT_GOOD;
        return 1;
    }

    return 0;
}

static int spellBlink(int dir) {
    return 1;
}

static int spellCure(int player) {
    assert(player < 8);

    if (player < c->saveGame->members && 
        c->saveGame->players[player].status == STAT_POISONED) {
        c->saveGame->players[player].status = STAT_GOOD;
        return 1;
    }

    return 0;
}

static int spellDispel(int dir) {
    int x, y;
    unsigned char tile;
    const Annotation *a;

    x = c->saveGame->x;
    y = c->saveGame->y;
    dirMove((Direction) dir, &x, &y);
    if (MAP_IS_OOB(c->map, x, y))
        return 0;

    /*
     * if there is a field annotation, remove it
     */
    a = annotationAt(x, y);
    if (a && tileCanDispel(a->tile)) {
        annotationRemove(x, y, a->tile);
        return 1;
    }

    /*
     * if the map tile itself is a field, overlay it with a brick
     * annotation
     */
    tile = mapTileAt(c->map, x, y);
    
    if (!tileCanDispel(tile))
        return 0;

    annotationAdd(x, y, -1, BRICKFLOOR_TILE);

    return 1;
}

static int spellEField(int dir) {
    int x, y;

    x = c->saveGame->x;
    y = c->saveGame->y;
    dirMove((Direction) dir, &x, &y);
    if (MAP_IS_OOB(c->map, x, y))
        return 0;

    annotationAdd(x, y, -1, LIGHTNINGFIELD_TILE);

    return 1;
}

static int spellFireball(int dir) {
    return 1;
}

static int spellGate(int phase) {
    return 1;
}

static int spellHeal(int player) {
    assert(player < 8);

    if (player < c->saveGame->members && 
        c->saveGame->players[player].status != STAT_DEAD) {
        c->saveGame->players[player].hp = c->saveGame->players[player].hpMax;
        return 1;
    }

    return 0;
}

static int spellIceball(int dir) {
    return 1;
}

static int spellJinx(int unused) {
    return 1;
}

static int spellKill(int dir) {
    return 1;
}

static int spellLight(int unused) {
    return 1;
}

static int spellMMissle(int dir) {
    return 1;
}

static int spellNegate(int unused) {
    return 1;
}

static int spellOpen(int unused) {
    return 1;
}

static int spellProtect(int unused) {
    return 1;
}

static int spellRez(int player) {
    assert(player < 8);

    if (player < c->saveGame->members && 
        c->saveGame->players[player].status == STAT_DEAD) {
        c->saveGame->players[player].status = STAT_GOOD;
        return 1;
    }

    return 0;
}

static int spellQuick(int unused) {
    return 1;
}

static int spellSleep(int unused) {
    return 1;
}

static int spellTremor(int unused) {
    return 1;
}

static int spellUndead(int unused) {
    return 1;
}

static int spellView(int unsued) {
    return 1;
}

static int spellWinds(int fromdir) {
    c->windDirection = fromdir;
    return 1;
}

static int spellXit(int unused) {
    return 1;
}

static int spellYup(int unused) {
    return 1;
}

static int spellZdown(int unused) {
    return 1;
}
