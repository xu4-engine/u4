/*
 * $Id$
 */

#ifndef MUSIC_H
#define MUSIC_H

#define CAMP_FADE_OUT_TIME          1000
#define CAMP_FADE_IN_TIME           0
#define INN_FADE_OUT_TIME           1000
#define INN_FADE_IN_TIME            5000

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
int musicIsPlaying(void);
void musicPlay(void);
void musicStop(void);
void musicFadeOut(int msecs);
void musicFadeIn(int msecs, int loadFromMap);
void musicLordBritish(void);
void musicHawkwind(void);
void musicCamp(void);
void musicShopping(void);
void musicIntro(void);
void musicIntroSwitch(int n);
int musicToggle(void);

#endif
