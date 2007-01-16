/*
 * $Id$
 */

#ifndef SOUND_H
#define SOUND_H

enum Sound {
    SOUND_TITLE_FADE,       // the intro title fade
    SOUND_WALK_NORMAL,      // walk, world and town
    SOUND_WALK_SLOWED,      // walk, slow progress
    SOUND_WALK_COMBAT,      // walk, combat
    SOUND_BLOCKED,          // location blocked
    SOUND_ERROR,            // error/bad command
    SOUND_PC_ATTACK,        // PC attacks
    SOUND_PC_STRUCK,        // PC damaged
    SOUND_NPC_ATTACK,       // NPC attacks
    SOUND_NPC_STRUCK,       // NPC damaged
    SOUND_ACID,             // effect, acid damage
    SOUND_SLEEP,            // effect, sleep
    SOUND_POISON_EFFECT,    // effect, poison
    SOUND_POISON_DAMAGE,    // damage, poison
    SOUND_EVADE,            // trap evaded
    SOUND_FLEE,             // flee combat
    SOUND_ITEM_STOLEN,      // item was stolen from a PC, food or gold
    SOUND_LBHEAL,           // LB heals party
    SOUND_LEVELUP,          // PC level up
    SOUND_MOONGATE,         // moongate used

    SOUND_CANNON,
    SOUND_RUMBLE,
    SOUND_MAGIC,
    SOUND_WHIRLPOOL,
    SOUND_STORM,

    SOUND_MISSED,
    SOUND_CREATUREATTACK,
    SOUND_PLAYERHIT,
    SOUND_MAX
};

int soundInit(void);
void soundDelete(void);
bool soundLoad(Sound sound);
void soundPlay(Sound sound, bool onlyOnce = true);
void soundStop(int channel = 1);

#endif /* SOUND_H */
