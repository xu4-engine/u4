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
    MUSIC_CASTLES,
    MUSIC_MAX
} Music;

int musicInit(void);
void musicDelete(void);
void musicPlay(void);
void musicStop(void);
void musicFadeOut(int msecs);
void musicFadeIn(int msecs);
void musicLordBritish(void);
void musicCamp(void);
void musicShopping(void);
void musicIntro(void);
void musicIntroSwitch(int n);
int musicToggle(void);

#endif
