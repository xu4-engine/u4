/*
 * $Id$
 */

#ifndef SPELL_H
#define SPELL_H

#include <stddef.h>

#include "context.h"
#include "savegame.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CASTERR_NOERROR,            /* success */
    CASTERR_NOMIX,              /* no mixture available */    
    CASTERR_MPTOOLOW,           /* caster doesn't have enough mp */
    CASTERR_FAILED,             /* the spell failed */
    CASTERR_WRONGCONTEXT,       /* generic 'wrong-context' error (generrally finds the correct
                                   context error message on its own) */
    CASTERR_COMBATONLY,         /* e.g. spell must be cast in combat */
    CASTERR_DUNGEONONLY,        /* e.g. spell must be cast in dungeons */
    CASTERR_WORLDMAPONLY,       /* e.g. spell must be cast on the world map */
} SpellCastError;

typedef enum {
    SPELLPRM_NONE,              /* none */
    SPELLPRM_PLAYER,            /* number of a player required */
    SPELLPRM_DIR,               /* direction required */
    SPELLPRM_TYPEDIR,           /* type of field and direction required (energy field) */
    SPELLPRM_PHASE,             /* phase required (gate) */
    SPELLPRM_FROMDIR            /* direction from required (winds) */
} SpellParam;

typedef enum {
    SPELLEFFECT_NONE,           /* none */
    SPELLEFFECT_INVERT,         /* invert the screen (moongates, most normal spells) */
    SPELLEFFECT_TREMOR          /* tremor spell */
} SpellEffect;

/* Field types for the Energy field spell */
typedef enum {
    ENERGYFIELD_NONE,
    ENERGYFIELD_FIRE,
    ENERGYFIELD_LIGHTNING,
    ENERGYFIELD_POISON,
    ENERGYFIELD_SLEEP
} EnergyFieldType;

typedef struct _Mixture {
    unsigned short reagents[REAG_MAX];
} Mixture;

typedef struct _Spell {
    const char *name;
    int components;
    LocationContext context;
    TransportContext transportContext;
    int (*spellFunc)(int);
    SpellParam paramType;
    int mp;
} Spell;

typedef void (*SpellEffectCallback)(unsigned int spell, int player, Sound sound);
extern SpellEffectCallback spellEffectCallback;

void playerSetSpellEffectCallback(SpellEffectCallback callback);
Mixture *mixtureNew();
void mixtureDelete(Mixture *mix);
int mixtureAddReagent(Mixture *mix, Reagent reagent);
int mixtureRemoveReagent(Mixture *mix, Reagent reagent);
void mixtureRevert(Mixture *mix);
const char *spellGetName(unsigned int spell);
int spellGetRequiredMP(unsigned int spell);
LocationContext spellGetContext(unsigned int spell);
TransportContext spellGetTransportContext(unsigned int spell);
const char *spellGetErrorMessage(unsigned int spell, SpellCastError error);
int spellMix(unsigned int spell, const Mixture *mix);
SpellParam spellGetParamType(unsigned int spell);
int spellCast(unsigned int spell, int character, int param, SpellCastError *error, int spellEffect);

#ifdef __cplusplus
}
#endif

#endif
