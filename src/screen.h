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
    BKGD_HONCOM,
    BKGD_VALJUS,
    BKGD_SACHONOR,
    BKGD_SPIRHUM,
    BKGD_ANIMATE,
    BKGD_MAX
} BackgroundType;

typedef struct {
    unsigned char tile;
    int hasFocus;
} ScreenTileInfo;

void screenInit(void);
void screenDelete(void);
void screenFixIntroScreen(const unsigned char *sigData);
void screenFreeIntroAnimations();
void screenFreeIntroBackgrounds();
void screenDrawBackground(BackgroundType bkgd);
void screenShowTile(const ScreenTileInfo *tileInfo, int x, int y);
void screenShowGemTile(const ScreenTileInfo *tileInfo, int x, int y);
void screenShowChar(int chr, int x, int y);
void screenShowCharMasked(int chr, int x, int y, unsigned char mask);
void screenTextAt(int x, int y, char *fmt, ...);
void screenMessage(const char *fmt, ...);
void screenScrollMessageArea(void);
ScreenTileInfo screenViewportTile(int width, int height, int x, int y);
void screenUpdate(int showmap);
void screenGemUpdate(void);
void screenRedrawScreen(void);
void screenRedrawMapArea(void);
void screenRedrawTextArea(int x, int y, int width, int height);
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
