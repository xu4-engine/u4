/*
 * $Id$
 */

#ifndef AREA_H
#define AREA_H

#ifdef __cplusplus
extern "C" {
#endif

#define AREA_MONSTERS 16
#define AREA_PLAYERS 8

typedef struct _Area {
    struct {
        unsigned char x, y;
    } monster_start[AREA_MONSTERS];
    struct {
        unsigned char x, y;
    } player_start[AREA_PLAYERS];
} Area;

#ifdef __cplusplus
}
#endif

#endif
