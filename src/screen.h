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
void screenFixIntroScreen(const unsigned char *sigData);
int screenLoadIntroAnimations();
void screenFreeIntroAnimations();
void screenFreeIntroBackgrounds();
void screenDrawBackground(BackgroundType bkgd);
void screenShowTile(unsigned char tile, int x, int y);
void screenShowGemTile(unsigned char tile, int x, int y);
void screenShowChar(int chr, int x, int y);
void screenShowCharMasked(int chr, int x, int y, unsigned char mask);
void screenTextAt(int x, int y, char *fmt, ...);
void screenMessage(const char *fmt, ...);
void screenScrollMessageArea(void);
unsigned char screenViewportTile(int width, int height, int x, int y);
void screenUpdate(void);
void screenForceRedraw(void);
void screenAnimateIntro(int frame);
void screenEraseTextArea(int x, int y, int width, int height);
void screenShowCard(int pos, int card);
void screenShowBeastie(int beast, int frame);
void screenCycle(void);
void screenUpdateCursor(void);
void screenUpdateMoons(void);
void screenUpdateWind(void);
void screenEnableCursor(void);
void screenDisableCursor(void);
void screenSetCursorPos(int x, int y);
void screenFindLineOfSight(void);

extern int screenCurrentCycle;

#define SCR_CYCLE_MAX 16

#endif
