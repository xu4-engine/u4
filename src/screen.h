/*
 * $Id$
 */

/**
 * @file
 * @brief Declares interfaces for working with the screen
 *
 * This file declares interfaces for manipulating areas of the screen, or the 
 * entire screen.  Functions like drawing images, tiles, and other items are
 * declared, as well as functions to draw pixels and rectangles and to update
 * areas of the screen.
 *
 * @todo
 *  <ul>
 *      <li>Create a View class</li>
 *      <li>Create a TextView and MapView class as subclasses of the View class</li>
 *      <li>Create game-specific U4GameView and U4IntroView classes</li>
 *      <li>make an ImageMgr class to store images and retrieve by name</li>
 *  </ul>
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

void screenIconify(void);

const std::vector<string> &screenGetGemLayoutNames(void);

void screenDrawImage(const string &name, int x = 0, int y = 0);
void screenDrawImageInMapArea(const string &bkgd);

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
void screenShowTile(MapTile* tile, bool focus, int x, int y);
void screenShowGemTile(MapTile* tile, bool focus, int x, int y);
void screenShowChar(int chr, int x, int y);
void screenShowCharMasked(int chr, int x, int y, unsigned char mask);
void screenTextAt(int x, int y, const char *fmt, ...) PRINTF_LIKE(3, 4);
void screenUpdate(int showmap, int blackout);
void screenUpdateCursor(void);
void screenUpdateMoons(void);
void screenUpdateWind(void);
MapTile* screenViewportTile(unsigned int width, unsigned int height, int x, int y, bool &focus);

void screenAnimateIntro(const string &frame);
void screenShowCard(int pos, int card);
void screenShowAbacusBeads(int row, int selectedVirtue, int rejectedVirtue);
void screenShowBeastie(int beast, int vertoffset, int frame);

void screenShowCursor(void);
void screenHideCursor(void);
void screenEnableCursor(void);
void screenDisableCursor(void);
void screenSetCursorPos(int x, int y);

void screenDungeonDrawTile(MapTile *tile, int distance, Direction orientation);
void screenDungeonDrawWall(int xoffset, int distance, Direction orientation, DungeonGraphicType type);

void screenSetMouseCursor(MouseCursor cursor);
int screenPointInMouseArea(int x, int y, MouseArea *area);

extern int screenCurrentCycle;

#define SCR_CYCLE_MAX 16

#endif
