/*
 * $Id$
 */

#ifndef CON_H
#define CON_H

#define CON_MONSTERS 16
#define CON_PLAYERS 8

typedef struct _Con {
    struct {
        unsigned char x, y;
    } monster_start[CON_MONSTERS];
    struct {
        unsigned char x, y;
    } player_start[CON_PLAYERS];
} Con;

#endif
