/*
 * $Id$
 */

#ifndef SOUND_H
#define SOUND_H

enum Sound {
    SOUND_WALK,
    SOUND_BLOCKED,
    SOUND_ERROR,
    SOUND_CANNON,
    SOUND_MISSED,
    SOUND_CREATUREATTACK,
    SOUND_RUMBLE,
    SOUND_PLAYERHIT,
    SOUND_MAGIC,
    SOUND_LBHEAL,
    SOUND_WHIRLPOOL,
    SOUND_STORM,
    SOUND_MOONGATE,
    SOUND_FLEE,
    SOUND_MAX
};

int soundInit(void);
void soundDelete(void);
void soundPlay(Sound sound, bool onlyOnce = true);

#endif /* SOUND_H */
