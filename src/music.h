/*
 * $Id$
 */

#ifndef MUSIC_H
#define MUSIC_H

typedef enum {
    MUSIC_NONE,
    MUSIC_OUTSIDE,
    MUSIC_TOWNS,
    MUSIC_SHRINES,
    MUSIC_SHOPPING,
    MUSIC_RULEBRIT,
    MUSIC_FANFARE,
    MUSIC_DUNGEON,
    MUSIC_COMBAT,
    MUSIC_CASTLES
} Music;

int musicInit(int sound);
void musicDelete();
void musicPlay(void);
void musicLordBritish(void);
void musicIntro(void);
int musicToggle();

#endif
