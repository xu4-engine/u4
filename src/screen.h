/*
 * $Id$
 */

#ifndef SCREEN_H
#define SCREEN_H

#include <vector>

#include "direction.h"
#include "dngview.h"
#include "types.h"
#include "u4file.h"

#if __GNUC__
#define PRINTF_LIKE(x,y)  __attribute__ ((format (printf, (x), (y))))
#else
#define PRINTF_LIKE(x,y)
#endif

#define BKGD_SHAPES "tiles"
#define BKGD_CHARSET "charset"
#define BKGD_BORDERS "borders"
#define BKGD_INTRO "title"
#define BKGD_INTRO_EXTENDED "titlex"
#define BKGD_TREE "tree"
#define BKGD_PORTAL "portal"
#define BKGD_OUTSIDE "outside"
#define BKGD_INSIDE "inside"
#define BKGD_WAGON "wagon"
#define BKGD_GYPSY "gypsy"
#define BKGD_ABACUS "abacus"
#define BKGD_HONCOM "honcom"
#define BKGD_VALJUS "valjus"
#define BKGD_SACHONOR "sachonor"
#define BKGD_SPIRHUM "spirhum"
#define BKGD_ANIMATE "beasties"
#define BKGD_KEY "key"
#define BKGD_HONESTY "honesty"
#define BKGD_COMPASSN "compassn"
#define BKGD_VALOR "valor"
#define BKGD_JUSTICE "justice"
#define BKGD_SACRIFIC "sacrific"
#define BKGD_HONOR "honor"
#define BKGD_SPIRIT "spirit"
#define BKGD_HUMILITY "humility"
#define BKGD_TRUTH "truth"
#define BKGD_LOVE "love"
#define BKGD_COURAGE "courage"
#define BKGD_STONCRCL "stoncrcl"
#define BKGD_RUNE_INF "rune0"
#define BKGD_SHRINE_HON "rune1"
#define BKGD_SHRINE_COM "rune2"
#define BKGD_SHRINE_VAL "rune3"
#define BKGD_SHRINE_JUS "rune4"
#define BKGD_SHRINE_SAC "rune5"
#define BKGD_SHRINE_HNR "rune6"
#define BKGD_SHRINE_SPI "rune7"
#define BKGD_SHRINE_HUM "rune8"
#define BKGD_GEMTILES "gemtiles"

typedef enum {
    MC_DEFAULT,
    MC_WEST,
    MC_NORTH,
    MC_EAST,
    MC_SOUTH
} MouseCursor;

typedef struct _MouseArea {
    int npoints;
    struct {
        int x, y;
    } point[4];
    MouseCursor cursor;
    int command[3];
} MouseArea;

#define SCR_CYCLE_PER_SECOND 4

void screenInit(void);
void screenDelete(void);
void screenReInit(void);

const std::vector<string> &screenGetImageSetNames(void);
const std::vector<string> &screenGetGemLayoutNames(void);

void screenDrawImage(const string &bkgd);
void screenDrawImageInMapArea(const string &bkgd);
void screenFreeImages();

void screenCycle(void);
void screenFillRect(int x, int y, int w, int h, int r, int g, int b);
void screenEraseMapArea(void);
void screenEraseTextArea(int x, int y, int width, int height);
void screenFindLineOfSight(void);
void screenGemUpdate(void);
void screenInvertRect(int x, int y, int w, int h);
void screenMessage(const char *fmt, ...) PRINTF_LIKE(1, 2);
void screenPrompt(void);
void screenRedrawMapArea(void);
void screenRedrawScreen(void);
void screenRedrawTextArea(int x, int y, int width, int height);
void screenScrollMessageArea(void);
void screenShake(int iterations);
void screenShowTile(MapTile* tile, int focus, int x, int y);
void screenShowGemTile(MapTile* tile, int focus, int x, int y);
void screenShowChar(int chr, int x, int y);
void screenShowCharMasked(int chr, int x, int y, unsigned char mask);
void screenTextAt(int x, int y, const char *fmt, ...) PRINTF_LIKE(3, 4);
void screenUpdate(int showmap, int blackout);
void screenUpdateCursor(void);
void screenUpdateMoons(void);
void screenUpdateWind(void);
MapTile* screenViewportTile(unsigned int width, unsigned int height, int x, int y, int *focus);

void screenAnimateIntro(const string &frame);
void screenFreeIntroAnimations();
void screenFreeIntroBackgrounds();
void screenShowCard(int pos, int card);
void screenShowAbacusBeads(int row, int selectedVirtue, int rejectedVirtue);
void screenShowBeastie(int beast, int vertoffset, int frame);

void screenShowCursor(void);
void screenHideCursor(void);
void screenEnableCursor(void);
void screenDisableCursor(void);
void screenSetCursorPos(int x, int y);

void screenDungeonDrawTile(int distance, MapTile *tile);
void screenDungeonDrawWall(int xoffset, int distance, Direction orientation, DungeonGraphicType type);

void screenSetMouseCursor(MouseCursor cursor);
int screenPointInMouseArea(int x, int y, MouseArea *area);

extern int screenCurrentCycle;

#define SCR_CYCLE_MAX 16

#endif
