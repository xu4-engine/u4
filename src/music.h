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

int init_music();
void play_music();
void lb_music();
void intro_music();

int musicToggle();

#endif
