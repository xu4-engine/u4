/*
 * $Id$
 */

#ifndef SOUND_H
#define SOUND_H

typedef enum {
    SOUND_WALK,
    SOUND_BLOCKED,
    SOUND_ERROR,
    SOUND_CANNON,
    SOUND_MISSED,
    SOUND_MONSTERATTACK,
    SOUND_RUMBLE,
    SOUND_PLAYERHIT,
    SOUND_MAGIC,
    SOUND_LBHEAL,
    SOUND_WHIRLPOOL,
    SOUND_STORM
} Sound;

int soundInit(void);
void soundDelete(void);
void soundPlay(Sound sound);

#endif /* SOUND_H */
