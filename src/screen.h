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
#include "txf_draw.h"
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
    FILTER_POINT,
    FILTER_HQX,
    FILTER_XBR_LV2,
    FILTER_XBRZ,
    FILTER_POINT_43,
    FILTER_XBRZ_43
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
    const TxfHeader* const* fontTable;
    int currentCycle;
    int vertOffset;
    int displayW;       // Full display pixel dimensions.
    int displayH;
    int aspectW;        // Aspect-correct pixel dimensions.
    int aspectH;
    int aspectX;        // Origin of aspect-correct area on display.
    int aspectY;
    int16_t cursorX;
    int16_t cursorY;
    bool cursorVisible;
};

#define SCR_CYCLE_PER_SECOND 4

void screenInit(int layerCount);
void screenRefreshTimerInit(void);
void screenDelete(void);
void screenReInit(void);
void screenSetLayer(int layer, void (*renderFunc)(ScreenState*, void*),
                    void* data);
bool screenLayerUsed(int layer);
void screenSwapBuffers();
void screenWait(int numberOfAnimationFrames);
void screenUploadToGPU();

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
void screenMessageCenter(const char* text, int newlines);
void screenMessageN(const char* buffer, int buflen);
void screenPrompt(void);
void screenRedrawMapArea(void);
void screenShake(int iterations);
void screenShowChar(int chr, int x, int y);
void screenTextAt(int x, int y, const char *fmt, ...) PRINTF_LIKE(3, 4);
void screenTextColor(int color);
bool screenTileUpdate(TileView *view, const Coords &coords);
#ifdef GPU_RENDER
void screenDisableMap();
void screenUpdateMap(TileView* view, const Map* map, const Coords& center);
#endif
void screenUpdate(TileView *view, bool showmap, bool blackout);
void screenUpdateMoons(void);
void screenUpdateWind(void);
std::vector<MapTile> screenViewportTile(unsigned int width, unsigned int height, int x, int y, bool &focus);

void screenShowCursor(bool on = true);
#define screenHideCursor()  screenShowCursor(false)
void screenSetCursorPos(int x, int y);

bool screenToggle3DDungeonView();
void screenMakeDungeonView();
void screenDetectDungeonTraps();

void screenSetMouseCursor(MouseCursor cursor);
void screenShowMouseCursor(bool visible);
void screenPointToMouseArea(int* x, int* y);
int  pointInMouseArea(int x, int y, const MouseArea *area);

const ScreenState* screenState();

#define SCR_CYCLE_MAX 16

#endif
