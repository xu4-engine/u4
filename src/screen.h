/*
 * $Id$
 */

#ifndef SCREEN_H
#define SCREEN_H

typedef enum {
    BKGD_INTRO,
    BKGD_BORDERS,
    BKGD_TREE,
    BKGD_PORTAL,
    BKGD_OUTSIDE,
    BKGD_INSIDE,
    BKGD_WAGON,
    BKGD_GYPSY,
    BKGD_ABACUS,
    BKGD_MAX
} BackgroundType;

void screenInit(int scale);
void screenDrawBackground(BackgroundType bkgd);
void screenShowTile(int tile, int x, int y);
void screenShowChar(int chr, int x, int y);
void screenTextAt(int x, int y, char *fmt, ...);
void screenMessage(char *fmt, ...);
void screenScrollMessageArea(void);
void screenUpdate(void);
void screenForceRedraw(void);
void screenCycle(void);
void screenUpdateCursor(void);
void screenUpdateMoons(void);
void screenEnableCursor(void);
void screenDisableCursor(void);
void screenSetCursorPos(int x, int y);

extern int screenCurrentCycle;

#define SCR_CYCLE_MAX 16

#endif
