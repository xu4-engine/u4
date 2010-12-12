/*
 * $Id$
 */

#ifndef SPELL_H
#define SPELL_H

#include "context.h"
#include "location.h"
#include "map.h"
#include "savegame.h"
#include "sound.h"

enum SpellCastError {
    CASTERR_NOERROR,            /* success */
    CASTERR_NOMIX,              /* no mixture available */    
    CASTERR_MPTOOLOW,           /* caster doesn't have enough mp */
    CASTERR_FAILED,             /* the spell failed */
    CASTERR_WRONGCONTEXT,       /* generic 'wrong-context' error (generrally finds the correct
                                   context error message on its own) */
    CASTERR_COMBATONLY,         /* e.g. spell must be cast in combat */
    CASTERR_DUNGEONONLY,        /* e.g. spell must be cast in dungeons */
    CASTERR_WORLDMAPONLY,       /* e.g. spell must be cast on the world map */
};

/* Field types for the Energy field spell */
enum EnergyFieldType {
    ENERGYFIELD_NONE,
    ENERGYFIELD_FIRE,
    ENERGYFIELD_LIGHTNING,
    ENERGYFIELD_POISON,
    ENERGYFIELD_SLEEP
};

/**
 * The ingredients for a spell mixture.
 */
class Ingredients {
public:
    Ingredients();
    bool addReagent(Reagent reagent);
    bool removeReagent(Reagent reagent);
    int getReagent(Reagent reagent) const;
    void revert();
    bool checkMultiple(int mixes) const;
    void multiply(int mixes);

private:
    unsigned short reagents[REAG_MAX];
};

struct Spell {
    typedef enum {
        PARAM_NONE,             /* none */
        PARAM_PLAYER,           /* number of a player required */
        PARAM_DIR,              /* direction required */
        PARAM_TYPEDIR,          /* type of field and direction required (energy field) */
        PARAM_PHASE,            /* phase required (gate) */
        PARAM_FROMDIR           /* direction from required (winds) */
    } Param;

    typedef enum {
        SFX_NONE,               /* none */
        SFX_INVERT,             /* invert the screen (moongates, most normal spells) */
        SFX_TREMOR              /* tremor spell */
    } SpecialEffects;

    const char *name;
    int components;
    LocationContext context;
    TransportContext transportContext;
    int (*spellFunc)(int);
    Param paramType;
    int mp;
};

typedef void (*SpellEffectCallback)(int spell, int player, Sound sound);

void spellSetEffectCallback(SpellEffectCallback callback);
const char *spellGetName(unsigned int spell);
int spellGetRequiredMP(unsigned int spell);
LocationContext spellGetContext(unsigned int spell);
TransportContext spellGetTransportContext(unsigned int spell);
string spellGetErrorMessage(unsigned int spell, SpellCastError error);
int spellMix(unsigned int spell, const Ingredients *ingredients);
Spell::Param spellGetParamType(unsigned int spell);
SpellCastError spellCheckPrerequisites(unsigned int spell, int character);
bool spellCast(unsigned int spell, int character, int param, SpellCastError *error, bool spellEffect);
const Spell* getSpell(int i);

#endif
