/*
 * $Id$
 */

#ifndef SCREEN_H
#define SCREEN_H

typedef enum {
    BKGD_BORDERS,
    BKGD_INTRO,
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
    BKGD_COMPASSN,
    BKGD_COURAGE,
    BKGD_HONESTY,
    BKGD_HONOR,
    BKGD_HUMILITY,
    BKGD_JUSTICE,
    BKGD_LOVE,
    BKGD_SACRIFIC,
    BKGD_SPIRIT,
    BKGD_TRUTH,
    BKGD_VALOR,
    BKGD_RUNE_INF,
    BKGD_SHRINE_HON,
    BKGD_SHRINE_COM,
    BKGD_SHRINE_VAL,
    BKGD_SHRINE_JUS,
    BKGD_SHRINE_SAC,
    BKGD_SHRINE_HNR,
    BKGD_SHRINE_SPI,
    BKGD_SHRINE_HUM,
    BKGD_MAX
} BackgroundType;

void screenInit(void);
void screenDelete(void);
void screenFixIntroScreen(const unsigned char *sigData);
void screenFreeIntroAnimations();
void screenFreeIntroBackgrounds();
void screenDrawBackground(BackgroundType bkgd);
void screenDrawBackgroundInMapArea(BackgroundType bkgd);
void screenShowTile(unsigned char tile, int focus, int x, int y);
void screenShowGemTile(unsigned char tile, int focus, int x, int y);
void screenShowChar(int chr, int x, int y);
void screenShowCharMasked(int chr, int x, int y, unsigned char mask);
void screenTextAt(int x, int y, char *fmt, ...);
void screenPrompt(void);
void screenMessage(const char *fmt, ...);
unsigned char screenViewportTile(int width, int height, int x, int y, int *focus);
void screenScrollMessageArea(void);
void screenInvertRect(int x, int y, int w, int h);
void screenUpdate(int showmap, int blackout);
void screenGemUpdate(void);
void screenRedrawScreen(void);
void screenRedrawMapArea(void);
void screenRedrawTextArea(int x, int y, int width, int height);
void screenAnimateIntro(int frame);
void screenEraseTextArea(int x, int y, int width, int height);
void screenShowCard(int pos, int card);
void screenShowBeastie(int beast, int vertoffset, int frame);
void screenCycle(void);
void screenUpdateCursor(void);
void screenUpdateMoons(void);
void screenUpdateWind(void);
void screenShowCursor(void);
void screenHideCursor(void);
void screenEnableCursor(void);
void screenDisableCursor(void);
void screenSetCursorPos(int x, int y);
void screenFindLineOfSight(void);

extern int screenCurrentCycle;

#define SCR_CYCLE_MAX 16

#endif
