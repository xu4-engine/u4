/*
 * $Id$
 */

#ifndef SCREEN_H
#define SCREEN_H

typedef enum {
    BKGD_INTRO,
    BKGD_BORDERS
} BackgroundType;

void screenInit(int scale);
void screenDrawBackground(BackgroundType bkgd);
void screenShowTile(int tile, int x, int y);
void screenShowChar(int chr, int x, int y);
void screenTextAt(int x, int y, char *fmt, ...);
void screenMessage(char *fmt, ...);
void screenScrollMessageArea();
void screenUpdate();
void screenForceRedraw();
void screenAnimate();
void screenUpdateCursor();
void screenUpdateMoons();

extern int screenCycle;

#define SCR_CYCLE_MAX 16

#endif
