/*
 * $Id$
 */

#ifndef SPELL_H
#define SPELL_H

#include <stddef.h>

#include "savegame.h"

typedef enum {
    CASTERR_NOERROR,            /* success */
    CASTERR_NOMIX,              /* no mixture available */
    CASTERR_WRONGCONTEXT,       /* e.g. spell must be cast in combat */
    CASTERR_MPTOOLOW           /* caster doesn't have enough mp */
} SpellCastError;

typedef struct _Spell {
    const char *name;
    int components;
    int context;
    int mp;
} Spell;

int spellMix(int spell, int n_regs, int *regs);
int spellCast(int spell, int character, SpellCastError *error);

#endif
