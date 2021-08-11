/**
 * screen.h
 * @brief Declares interfaces for working with the screen
 *
 * This file declares interfaces for manipulating areas of the screen, or the
 * entire screen.  Functions like drawing images, tiles, and other items are
 * declared, as well as functions to draw pixels and rectangles and to update
 * areas of the screen.
 *
 * Most of the functions here are obsolete and are slowly being
 * migrated to the xxxView classes.
 *
 * @todo
 *  <ul>
 *      <li>migrate rest of text output logic to TextView</li>
 *      <li>migrate rest of dungeon drawing logic to DungeonView</li>
 *  </ul>
 */

#ifndef SCREEN_H
#define SCREEN_H

#include <vector>
#include <string>

#include "direction.h"
#include "types.h"
#include "u4file.h"

class Image;
class Map;
class Tile;
class TileView;
class Coords;

#if __GNUC__
#define PRINTF_LIKE(x,y)  __attribute__ ((format (printf, (x), (y))))
#else
#define PRINTF_LIKE(x,y)
#endif

enum ScreenFilter {
    ScreenFilter_point,
    ScreenFilter_2xBi,
    ScreenFilter_2xSaI,
    ScreenFilter_Scale2x
};

enum LayoutType {
    LAYOUT_STANDARD,
    LAYOUT_GEM,
    LAYOUT_DUNGEONGEM
};

struct Layout {
    StringId name;
    LayoutType type;
    struct {
        int16_t width, height;
    } tileshape;
    struct {
        int16_t x, y;
        int16_t width, height;
    } viewport;
};

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

class TileAnimSet;

// Expose a few Screen members via this struct.
struct ScreenState {
    const TileAnimSet* tileanims;
    int currentCycle;
    int vertOffset;
    bool formatIsABGR;
};

#define SCR_CYCLE_PER_SECOND 4

void screenInit(void);
void screenRefreshTimerInit(void);
void screenDelete(void);
void screenReInit(void);
void screenSwapBuffers();
void screenWait(int numberOfAnimationFrames);

void screenIconify(void);

const std::vector<std::string> &screenGetGemLayoutNames();
const char** screenGetFilterNames();
const char** screenGetLineOfSightStyles();

void screenDrawImageInMapArea(Symbol bkgd);

void screenCycle(void);
void screenEraseMapArea(void);
void screenEraseTextArea(int x, int y, int width, int height);
void screenGemUpdate(void);

void screenCrLf();
void screenMessage(const char *fmt, ...) PRINTF_LIKE(1, 2);
void screenPrompt(void);
void screenRedrawMapArea(void);
void screenShake(int iterations);
void screenShowChar(int chr, int x, int y);
void screenShowCharMasked(int chr, int x, int y, unsigned char mask);
void screenTextAt(int x, int y, const char *fmt, ...) PRINTF_LIKE(3, 4);
void screenTextColor(int color);
bool screenTileUpdate(TileView *view, const Coords &coords);
void screenUpdate(TileView *view, bool showmap, bool blackout);
void screenUpdateCursor(void);
void screenUpdateMoons(void);
void screenUpdateWind(void);
std::vector<MapTile> screenViewportTile(unsigned int width, unsigned int height, int x, int y, bool &focus);

void screenShowCursor(void);
void screenHideCursor(void);
void screenEnableCursor(void);
void screenDisableCursor(void);
void screenSetCursorPos(int x, int y);
bool screenToggle3DDungeonView();
void screenMakeDungeonView();

void screenSetMouseCursor(MouseCursor cursor);
void screenShowMouseCursor(bool visible);
int screenPointInMouseArea(int x, int y, MouseArea *area);

Image *screenScale(Image *src, int scale, int n, int filter);
Image *screenScaleDown(Image *src, int scale);
ScreenState* screenState();

#define SCR_CYCLE_MAX 16

#endif
