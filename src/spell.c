/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "u4.h"
#include "spell.h"
#include "context.h"
#include "screen.h"


/* place holder for mp values I haven't figured out yet */
#define UNKNOWN 13

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
    { "Awaken",       GINSENG | GARLIC,        SC_ANY,     5 },
    { "Blink",        SILK | MOSS,             SC_NORMAL,  15 },
    { "Cure",         GINSENG | GARLIC,        SC_ANY,     5 },
    { "Dispel",       ASH | GARLIC | PEARL,    SC_ANY,     20 },
    { "Energy Field", ASH | SILK | PEARL,      SC_ANY,     UNKNOWN },
    { "Fireball",     ASH | PEARL,             SC_COMBAT,  15 },
    { "Gate",         ASH | PEARL | MANDRAKE,  SC_NORMAL,  UNKNOWN },
    { "Heal",         GINSENG | SILK,          SC_ANY,     10 },
    { "Iceball",      PEARL | MANDRAKE,        SC_COMBAT,  UNKNOWN },
    { "Jinx",         PEARL | NIGHTSHADE | MANDRAKE, SC_COMBAT, UNKNOWN },
    { "Kill",         PEARL | NIGHTSHADE,      SC_COMBAT,  UNKNOWN },
    { "Light",        ASH,                     SC_DUNGEON, UNKNOWN },
    { "Magic missile", ASH | PEARL,            SC_COMBAT,  UNKNOWN },
    { "Negate",       ASH | GARLIC | MANDRAKE, SC_COMBAT,  UNKNOWN },
    { "Open",         ASH | MOSS,              SC_NORMAL,  5 },
    { "Protection",   ASH | GINSENG | GARLIC,  SC_ANY,     UNKNOWN },
    { "Quickness",    ASH | GINSENG | MOSS,    SC_ANY,     UNKNOWN },
    { "Resurrect",    ASH | GINSENG | GARLIC | SILK | MOSS | MANDRAKE, SC_ANY, 45 },
    { "Sleep",        SILK | GINSENG,          SC_COMBAT,  15 },
    { "Tremor",       ASH | MOSS | MANDRAKE,   SC_COMBAT,  30 },
    { "Undead",       ASH | GARLIC,            SC_ANY,     UNKNOWN },
    { "View",         NIGHTSHADE | MANDRAKE,   SC_NORMAL | SC_DUNGEON, UNKNOWN },
    { "Winds",        ASH | MOSS,              SC_NORMAL,  10 },
    { "X-it",         ASH | SILK | MOSS,       SC_DUNGEON, UNKNOWN },
    { "Y-up",         SILK | MOSS,             SC_DUNGEON, UNKNOWN },
    { "Z-down",       SILK | MOSS,             SC_DUNGEON, UNKNOWN }
};

#define N_SPELLS (sizeof(spells) / sizeof(spells[0]))

/**
 * Mix reagents for a spell.  Fails and returns false if the reagents
 * selected were not correct.
 */
int spellMix(int spell, int n_regs, int *regs) {
    int regmask, i;

    assert(spell >= 0 && spell < N_SPELLS);

    regmask = 0;
    for (i = 0; i < n_regs; i++) {
        assert(regs[i] < REAG_MAX);
        regmask |= (1 << regs[i]);
        assert(c->saveGame->reagents[regs[i]] > 0);
        c->saveGame->reagents[regs[i]]--;
    }

    if (regmask != spells[spell].components)
        return 0;

    c->saveGame->mixtures[spell]++;

    return 1;
}

/**
 * Casts spell.  Fails and returns false if no mixture is available,
 * the character doesn't have enough magic points, or the context is
 * invalid.  The error code is updated with the reason for failure.
 */
int spellCast(int spell, int character, SpellCastError *error) {
    assert(spell >= 0 && spell < N_SPELLS);
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

    screenMessage("%s!\n", spells[spell].name);

    return 1;
}
