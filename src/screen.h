/*
 * $Id$
 */

#ifndef SCREEN_H
#define SCREEN_H

#include "dngview.h"

typedef enum {
    BKGD_BORDERS,
    BKGD_INTRO,
    BKGD_INTRO_EXTENDED,
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
void screenReInit(void);

void screenDrawBackground(BackgroundType bkgd);
void screenDrawBackgroundInMapArea(BackgroundType bkgd);
void screenFreeBackgrounds();
const char *screenGetVgaFilename(BackgroundType bkgd);
const char *screenGetEgaFilename(BackgroundType bkgd);

void screenCycle(void);
void screenEraseMapArea(void);
void screenEraseTextArea(int x, int y, int width, int height);
void screenFindLineOfSight(void);
void screenGemUpdate(void);
void screenInvertRect(int x, int y, int w, int h);
void screenMessage(const char *fmt, ...);
void screenPrompt(void);
void screenRedrawMapArea(void);
void screenRedrawScreen(void);
void screenRedrawTextArea(int x, int y, int width, int height);
void screenScrollMessageArea(void);
void screenShake(int iterations);
void screenShowTile(unsigned char tile, int focus, int x, int y);
void screenShowGemTile(unsigned char tile, int focus, int x, int y);
void screenShowChar(int chr, int x, int y);
void screenShowCharMasked(int chr, int x, int y, unsigned char mask);
void screenTextAt(int x, int y, char *fmt, ...);
void screenUpdate(int showmap, int blackout);
void screenUpdateCursor(void);
void screenUpdateMoons(void);
void screenUpdateWind(void);
unsigned char screenViewportTile(unsigned int width, unsigned int height, int x, int y, int *focus);

void screenAnimateIntro(int frame);
void screenFixIntroScreen(BackgroundType bkgd, const unsigned char *sigData);
void screenFixIntroScreenExtended(BackgroundType bkgd);
void screenFreeIntroAnimations();
void screenFreeIntroBackgrounds();
void screenShowCard(int pos, int card);
void screenShowBeastie(int beast, int vertoffset, int frame);

void screenShowCursor(void);
void screenHideCursor(void);
void screenEnableCursor(void);
void screenDisableCursor(void);
void screenSetCursorPos(int x, int y);

void screenDungeonDrawWall(int xoffset, int distance, DungeonGraphicType type);

extern int screenCurrentCycle;

#define SCR_CYCLE_MAX 16

#endif
