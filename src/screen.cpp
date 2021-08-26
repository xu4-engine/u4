/*
 * screen.cpp
 */

#include <cstdio>
#include <cstdarg>
#include <cfloat>
#include <cstring>
#include <assert.h>
#include "u4.h"

#include "screen.h"

#include "config.h"
#include "context.h"
#include "debug.h"
#include "dungeonview.h"
#include "error.h"
#include "event.h"
#include "imagemgr.h"
#include "object.h"
#include "scale.h"
#include "settings.h"
#include "textcolor.h"
#include "tileanim.h"
#include "tileset.h"
#include "tileview.h"
#include "annotation.h"
#include "xu4.h"

#ifdef USE_GL
#include "gpu.h"
#endif

#ifdef IOS
#include "ios_helpers.h"
#endif

using std::vector;

struct Screen {
    vector<string> gemLayoutNames;
    const Layout* gemLayout;
    const Layout* dungeonGemLayout;
    DungeonView* dungeonView;
    std::map<string, int> dungeonTileChars;
    ImageInfo* charsetInfo;
    ImageInfo* gemTilesInfo;
    ScreenState state;
    Scaler filterScaler;
    int width;
    int height;
    int cursorX;
    int cursorY;
    int cursorStatus;
    int cursorEnabled;
    int needPrompt;
    uint8_t blockingGrid[VIEWPORT_W * VIEWPORT_H];
    uint8_t screenLos[VIEWPORT_W * VIEWPORT_H];

    Screen() {
        gemLayout = NULL;
        dungeonGemLayout = NULL;
        dungeonView = NULL;
        charsetInfo = NULL;
        gemTilesInfo = NULL;
        state.tileanims = NULL;
        state.currentCycle = 0;
        state.vertOffset = 0;
        state.formatIsABGR = true;
        width = height = 0;
        cursorX = cursorY = 0;
        cursorStatus = 0;
        cursorEnabled = 1;
        needPrompt = 1;
    }

    ~Screen() {
        delete dungeonView;
    }
};

static void screenLoadLayoutsFromConf(Screen*);
static void screenFindLineOfSight();

static const int BufferSize = 1024;

extern bool verbose;

// Just extern the system functions here. That way people aren't tempted to call them as part of the public API.
extern void screenInit_sys(const Settings*, int* dim, int reset);
extern void screenDelete_sys();

static void initDungeonTileChars(std::map<string, int>& dungeonTileChars) {
    dungeonTileChars.clear();
    dungeonTileChars["brick_floor"] = CHARSET_FLOOR;
    dungeonTileChars["up_ladder"] = CHARSET_LADDER_UP;
    dungeonTileChars["down_ladder"] = CHARSET_LADDER_DOWN;
    dungeonTileChars["up_down_ladder"] = CHARSET_LADDER_UPDOWN;
    dungeonTileChars["chest"] = '$';
    dungeonTileChars["ceiling_hole"] = CHARSET_FLOOR;
    dungeonTileChars["floor_hole"] = CHARSET_FLOOR;
    dungeonTileChars["magic_orb"] = CHARSET_ORB;
    dungeonTileChars["ceiling_hole"] = 'T';
    dungeonTileChars["floor_hole"] = 'T';
    dungeonTileChars["fountain"] = 'F';
    dungeonTileChars["secret_door"] = CHARSET_SDOOR;
    dungeonTileChars["brick_wall"] = CHARSET_WALL;
    dungeonTileChars["dungeon_door"] = CHARSET_ROOM;
    dungeonTileChars["avatar"] = CHARSET_REDDOT;
    dungeonTileChars["dungeon_room"] = CHARSET_ROOM;
    dungeonTileChars["dungeon_altar"] = CHARSET_ANKH;
    dungeonTileChars["energy_field"] = '^';
    dungeonTileChars["fire_field"] = '^';
    dungeonTileChars["poison_field"] = '^';
    dungeonTileChars["sleep_field"] = '^';
}

static void screenInit_data(Screen* scr, Settings& settings) {
    scr->filterScaler = scalerGet(settings.filter);
    if (! scr->filterScaler)
        errorFatal("Invalid filter %d", settings.filter);

    /* If we can't use VGA graphics then reset to EGA. */
    if (! u4isUpgradeAvailable() && settings.videoType == "VGA")
        settings.videoType = "EGA";

    // Create a special purpose image that represents the whole screen.
    xu4.screenImage = Image::create
#ifdef USE_GL
        (320, 200);
#else
        (320 * settings.scale, 200 * settings.scale);
#endif
    xu4.screenImage->fill(Image::black);

    xu4.imageMgr = new ImageMgr;

    scr->charsetInfo = xu4.imageMgr->get(BKGD_CHARSET);
    if (! scr->charsetInfo)
        errorLoadImage(BKGD_CHARSET);

#ifdef USE_GL
    ImageInfo* shapes = xu4.imageMgr->get(BKGD_SHAPES);
    gpu_setTilesTexture(xu4.gpu, shapes->tex);
#endif

    assert(scr->state.tileanims == NULL);
    scr->state.tileanims = xu4.config->newTileAnims(settings.videoType.c_str());
    if (! scr->state.tileanims)
        errorFatal("Unable to find \"%s\" tile animations", settings.videoType.c_str());

    scr->gemTilesInfo = NULL;
    screenLoadLayoutsFromConf(scr);

    if (verbose)
        printf("using %s scaler\n", screenGetFilterNames()[ settings.filter ]);

    EventHandler::setKeyRepeat(settings.keydelay, settings.keyinterval);

    initDungeonTileChars(scr->dungeonTileChars);
    if (scr->dungeonView)
        scr->dungeonView->cacheGraphicData();

    Tileset::loadImages();
}

static void screenDelete_data(Screen* scr) {
    Tileset::unloadImages();

    delete scr->state.tileanims;
    scr->state.tileanims = NULL;

    delete xu4.imageMgr;
    xu4.imageMgr = NULL;

    delete xu4.screenImage;
    xu4.screenImage = NULL;
}


enum ScreenSystemStage {
    SYS_CLEAN = 0,  // Initial screen setup.
    SYS_RESET = 1   // Reconfigure screen settings.
};

/*
 * Sets xu4.screen, xu4.screenSys, xu4.screenImage & xu4.gpu pointers.
 */
void screenInit() {
    xu4.screen = new Screen;
    screenInit_sys(xu4.settings, &xu4.screen->width, SYS_CLEAN);
    screenInit_data(xu4.screen, *xu4.settings);
}

void screenDelete() {
    if (xu4.screen) {
        screenDelete_data(xu4.screen);
        screenDelete_sys();
        delete xu4.screen;
        xu4.screen = NULL;
    }
}

/**
 * Re-initializes the screen and implements any changes made in settings
 */
void screenReInit() {
    screenDelete_data(xu4.screen);
    screenInit_sys(xu4.settings, &xu4.screen->width, SYS_RESET);
    screenInit_data(xu4.screen, *xu4.settings); // Load new backgrounds, etc.
}

void screenTextAt(int x, int y, const char *fmt, ...) {
    char buffer[BufferSize];
    unsigned int i;

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, BufferSize, fmt, args);
    va_end(args);

    for (i = 0; i < strlen(buffer); i++)
        screenShowChar(buffer[i], x + i, y);
}

void screenPrompt() {
    Screen* scr = xu4.screen;
    if (scr->needPrompt && scr->cursorEnabled && c->col == 0) {
        screenMessage("%c", CHARSET_PROMPT);
        scr->needPrompt = 0;
    }
}

static void screenScrollMessageArea();

/*
 * Do carriage return and line feed in message area.
 */
void screenCrLf() {
    c->col = 0;
    c->line++;

    /* scroll the message area, if necessary */
    if (c->line == TEXT_AREA_H) {
        c->line--;
        screenHideCursor();
        screenScrollMessageArea();
        screenShowCursor();
    }
}

void screenMessage(const char *fmt, ...) {
#ifdef IOS
    static bool recursed = false;
#endif

    if (!c)
        return; //Because some cases (like the intro) don't have the context initiated.
    char buffer[BufferSize];
    unsigned int i;
    int wordlen;

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, BufferSize, fmt, args);
    va_end(args);
#ifdef IOS
    if (recursed)
        recursed = false;
    else
        U4IOS::drawMessageOnLabel(string(buffer, 1024));
#endif

    screenHideCursor();

    /* scroll the message area, if necessary */
    if (c->line == TEXT_AREA_H) {
        c->line--;
        screenScrollMessageArea();
    }

    for (i = 0; i < strlen(buffer); i++) {
        // include whitespace and color-change codes
        wordlen = strcspn(buffer + i, " \b\t\n\024\025\026\027\030\031");

        /* backspace */
        if (buffer[i] == '\b') {
            c->col--;
            if (c->col < 0) {
                c->col += 16;
                c->line--;
            }
            continue;
        }

        /* color-change codes */
        switch (buffer[i])
        {
            case FG_GREY:
            case FG_BLUE:
            case FG_PURPLE:
            case FG_GREEN:
            case FG_RED:
            case FG_YELLOW:
            case FG_WHITE:
                screenTextColor(buffer[i]);
                continue;
        }

        /* check for word wrap */
        if ((c->col + wordlen > 16) || buffer[i] == '\n' || c->col == 16) {
            if (buffer[i] == '\n' || buffer[i] == ' ')
                i++;
            c->line++;
            c->col = 0;
#ifdef IOS
            recursed = true;
#endif
            screenMessage("%s", buffer + i);
            return;
        }

        /* code for move cursor right */
        if (buffer[i] == 0x12) {
            c->col++;
            continue;
        }
        /* don't show a space in column 1.  Helps with Hawkwind. */
        if (buffer[i] == ' ' && c->col == 0)
          continue;
        screenShowChar(buffer[i], TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line);
        c->col++;
    }

    screenSetCursorPos(TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line);
    screenShowCursor();

    xu4.screen->needPrompt = 1;
}

const vector<string>& screenGetGemLayoutNames() {
    return xu4.screen->gemLayoutNames;
}

const char** screenGetFilterNames() {
    static const char* filterNames[] = {
        "point", "2xBi", "2xSaI", "Scale2x", NULL
    };
    return filterNames;
}

const char** screenGetLineOfSightStyles() {
    static const char* lineOfSightStyles[] = {"DOS", "Enhanced", NULL};
    return lineOfSightStyles;
}

static void screenLoadLayoutsFromConf(Screen* scr) {
    // Save gem layout names and find one to use.
    uint32_t count;
    uint32_t i;
    const Layout* layout = xu4.config->layouts(&count);

    scr->gemLayout = scr->dungeonGemLayout = NULL;
    scr->gemLayoutNames.clear();

    for (i = 0; i < count; ++i) {
        if (layout[i].type == LAYOUT_GEM) {
            const char* name = xu4.config->confString( layout[i].name );
            scr->gemLayoutNames.push_back(name);

            if (! scr->gemLayout && xu4.settings->gemLayout == name)
                scr->gemLayout = layout + i;
        } else if (layout[i].type == LAYOUT_DUNGEONGEM) {
            if (! scr->dungeonGemLayout)
                scr->dungeonGemLayout = layout + i;
        }
    }

    if (! scr->gemLayout)
        errorFatal("no gem layout named %s found!\n", xu4.settings->gemLayout.c_str());
    if (! scr->dungeonGemLayout)
        errorFatal("no dungeon gem layout found!\n");
}

vector<MapTile> screenViewportTile(unsigned int width, unsigned int height, int x, int y, bool &focus) {
    Map* map = c->location->map;
    Coords center = c->location->coords;
    static MapTile grass = map->tileset->getByName(Tile::sym.grass)->getId();

    if (map->width <= width &&
        map->height <= height) {
        center.x = map->width / 2;
        center.y = map->height / 2;
    }

    Coords tc = center;

    tc.x += x - (width / 2);
    tc.y += y - (height / 2);

    /* Wrap the location if we can */
    map_wrap(tc, map);

    /* off the edge of the map: pad with grass tiles */
    if (MAP_IS_OOB(map, tc)) {
        focus = false;
        vector<MapTile> result;
        result.push_back(grass);
        return result;
    }

    return c->location->tilesAt(tc, focus);
}

/*
 * Return true if coords is visible and tiles were drawn.
 */
bool screenTileUpdate(TileView *view, const Coords &coords)
{
    Location* loc = c->location;
    if (loc->map->flags & FIRST_PERSON)
        return false;

    // Get the tiles
    bool focus;
    Coords mc(coords);
    map_wrap(mc, loc->map);
    vector<MapTile> tiles = loc->tilesAt(mc, focus);

    // Get the screen coordinates
    int x = coords.x;
    int y = coords.y;

    if (loc->map->width > VIEWPORT_W || loc->map->height > VIEWPORT_H)
    {
        //Center the coordinates to the viewport if you're on centered-view map.
        x = x - loc->coords.x + VIEWPORT_W / 2;
        y = y - loc->coords.y + VIEWPORT_H / 2;
    }

    // Draw if it is on screen
    if (x >= 0 && y >= 0 && x < VIEWPORT_W && y < VIEWPORT_H &&
        xu4.screen->screenLos[y*VIEWPORT_W + x])
    {
        view->drawTile(tiles, x, y);
        if (focus)
            view->drawFocus(x, y);
        return true;
    }
    return false;
}

#ifdef GPU_RENDER
struct SpriteRenderData {
    float* attr;
    const float* uvTable;
    float rect[4];
    int cx, cy;
};

#define VIEW_TILE_SIZE  (2.0f / VIEWPORT_W)

static void drawSprite(const Coords* loc, VisualId vid, void* user) {
    SpriteRenderData* rd = (SpriteRenderData*) user;
    float* rect = rd->rect;
    const float halfTile = VIEW_TILE_SIZE * -0.5f;
    int uvIndex = VID_INDEX(vid);

    rect[0] = halfTile + (float) (loc->x - rd->cx) * VIEW_TILE_SIZE;
    rect[1] = halfTile + (float) (rd->cy - loc->y) * VIEW_TILE_SIZE;
    rd->attr = gpu_emitQuad(rd->attr, rect, rd->uvTable + uvIndex*4);
#if 0
    printf("KR drawSprite %d,%d vid:%d:%d\n",
            loc->x, loc->y, VID_BANK(vid), VID_INDEX(vid));
#endif
}
#endif

/**
 * Redraw the screen.  If showmap is set, the normal map is drawn in
 * the map area.  If blackout is set, the map area is blacked out. If
 * neither is set, the map area is left untouched.
 */
void screenUpdate(TileView *view, bool showmap, bool blackout) {
    ASSERT(c != NULL, "context has not yet been initialized");

#ifdef GPU_RENDER
    //static const float clearColor[4] = {0.0, 0.5, 0.8, 1.0};
    Location* loc = c->location;
    const Map* map = loc->map;

    if (blackout) {
        screenEraseMapArea();
        goto raster_update;
    }
    else if (map->flags & FIRST_PERSON) {
        xu4.screen->dungeonView->display(c, view);
        xu4.game->mapArea.update();

raster_update:
        screenUpdateCursor();
        screenUpdateMoons();
        screenUpdateWind();

        gpu_viewport(0, 0, xu4.screen->width, xu4.screen->height);
        gpu_background(xu4.gpu, NULL, xu4.screenImage);
    }
    else if (showmap) {
        ImageInfo* shapes = xu4.imageMgr->get(BKGD_SHAPES);
        const Coords& coord = loc->coords;   // Center of view.

        screenUpdateCursor();
        screenUpdateMoons();
        screenUpdateWind();

        gpu_viewport(0, 0, xu4.screen->width, xu4.screen->height);
        gpu_background(xu4.gpu, NULL, xu4.screenImage);

#if 0
        // Unscaled pixel rect. on screen.
        printf( "KR view %d,%d %d,%d\n",
                view->x, view->y, view->width, view->height );
#endif
        const int* vrect = view->screenRect;
        gpu_viewport(vrect[0], vrect[1], vrect[2], vrect[3]);
        gpu_drawMap(xu4.gpu, map, shapes->tileTexCoord,
                    coord.x, coord.y, VIEWPORT_W / 2);

        {
        SpriteRenderData rd;

        rd.uvTable = shapes->tileTexCoord;
        rd.rect[2] = rd.rect[3] = VIEW_TILE_SIZE;
        rd.cx = coord.x;
        rd.cy = coord.y;
        rd.attr = gpu_beginDraw(xu4.gpu);

        map->queryVisible(coord, VIEWPORT_W / 2, drawSprite, &rd);

        gpu_endDraw(xu4.gpu, rd.attr);
        }
    }
#else
    if (blackout)
    {
        screenEraseMapArea();
    }
    else if (c->location->map->flags & FIRST_PERSON) {
        xu4.screen->dungeonView->display(c, view);
        screenRedrawMapArea();
    }
    else if (showmap) {
        MapTile black = c->location->map->tileset->getByName(Tile::sym.black)->getId();
        vector<MapTile> viewTiles[VIEWPORT_W][VIEWPORT_H];
        uint8_t* blocked = xu4.screen->blockingGrid;
        bool focus;
        int focusX, focusY;
        int x, y;

        focusX = -1;
        for (y = 0; y < VIEWPORT_H; y++) {
            for (x = 0; x < VIEWPORT_W; x++) {
                viewTiles[x][y] = screenViewportTile(VIEWPORT_W, VIEWPORT_H,
                                                     x, y, focus);
                *blocked++ = viewTiles[x][y].front().getTileType()->isOpaque();
                if (focus) {
                    focusX = x;
                    focusY = y;
                }
            }
        }

        screenFindLineOfSight();

        const uint8_t* lineOfSight = xu4.screen->screenLos;
        for (y = 0; y < VIEWPORT_H; y++) {
            for (x = 0; x < VIEWPORT_W; x++) {
                if (*lineOfSight++)
                    view->drawTile(viewTiles[x][y], x, y);
                else
                    view->drawTile(black, x, y);
            }
        }
        if (focusX >= 0)
            view->drawFocus(focusX, focusY);
        screenRedrawMapArea();
    }

    screenUpdateCursor();
    screenUpdateMoons();
    screenUpdateWind();
#endif
}

void screenDrawImageInMapArea(Symbol name) {
    ImageInfo *info;

    info = xu4.imageMgr->get(name);
    if (!info)
        errorLoadImage(name);

    SCALED_VAR;
    info->image->drawSubRect(SCALED(BORDER_WIDTH), SCALED(BORDER_HEIGHT),
                             SCALED(BORDER_WIDTH), SCALED(BORDER_HEIGHT),
                             VIEWPORT_W * SCALED(TILE_WIDTH),
                             VIEWPORT_H * SCALED(TILE_HEIGHT));
}


/**
 * Change the current text color
 */
void screenTextColor(int color) {
    if (!xu4.settings->enhancements ||
        !xu4.settings->enhancementsOptions.textColorization) {
        return;
    }

    switch (color)
    {
        case FG_GREY:
        case FG_BLUE:
        case FG_PURPLE:
        case FG_GREEN:
        case FG_RED:
        case FG_YELLOW:
        case FG_WHITE:
            xu4.screen->charsetInfo->image->setFontColorFG((ColorFG)color);
    }
}

/**
 * Draw a character from the charset onto the screen.
 */
void screenShowChar(int chr, int x, int y) {
    ImageInfo* charset = xu4.screen->charsetInfo;
    SCALED_VAR
    int charW = charset->image->width();
    int charH = SCALED(CHAR_HEIGHT);

    charset->image->drawSubRect(x * charW, y * charH,
                                0, chr * charH, charW, charH);
}

/**
 * Scroll the text in the message area up one position.
 */
static void screenScrollMessageArea() {
    ImageInfo* charset = xu4.screen->charsetInfo;
    ASSERT(charset != NULL && charset->image != NULL, "charset not initialized!");

    SCALED_VAR
    Image* screen = xu4.screenImage;
    int charW = charset->image->width();
    int charH = SCALED(CHAR_HEIGHT);

    screen->drawSubRectOn(screen,
                          TEXT_AREA_X * charW, TEXT_AREA_Y * charH,
                          TEXT_AREA_X * charW, (TEXT_AREA_Y + 1) * charH,
                          TEXT_AREA_W * charW, (TEXT_AREA_H - 1) * charH);

    screen->fillRect(TEXT_AREA_X * charW,
                     TEXT_AREA_Y * charH + (TEXT_AREA_H - 1) * charH,
                     TEXT_AREA_W * charW, charH, 0, 0, 0);
}

void screenCycle() {
    int cycle = xu4.screen->state.currentCycle + 1;
    if (cycle >= SCR_CYCLE_MAX)
        cycle = 0;
    xu4.screen->state.currentCycle = cycle;
}

void screenUpdateCursor() {
    Screen* scr = xu4.screen;
    int phase = scr->state.currentCycle * SCR_CYCLE_PER_SECOND / SCR_CYCLE_MAX;

    ASSERT(phase >= 0 && phase < 4, "derived an invalid cursor phase: %d", phase);

    if (scr->cursorStatus) {
        screenShowChar(31 - phase, scr->cursorX, scr->cursorY);
    }
}

void screenUpdateMoons() {
    int trammelChar, feluccaChar;

    /* show "L?" for the dungeon level */
    if (c->location->context == CTX_DUNGEON) {
        screenShowChar('L', 11, 0);
        screenShowChar('1'+c->location->coords.z, 12, 0);
    }
    /* show the current moons (non-combat) */
    else if ((c->location->context & CTX_NON_COMBAT) == c->location->context) {
        trammelChar = (c->saveGame->trammelphase == 0) ?
            MOON_CHAR + 7 :
            MOON_CHAR + c->saveGame->trammelphase - 1;
        feluccaChar = (c->saveGame->feluccaphase == 0) ?
            MOON_CHAR + 7 :
            MOON_CHAR + c->saveGame->feluccaphase - 1;

        screenShowChar(trammelChar, 11, 0);
        screenShowChar(feluccaChar, 12, 0);
    }
}

void screenUpdateWind() {

    /* show the direction we're facing in the dungeon */
    if (c->location->context == CTX_DUNGEON) {
        screenEraseTextArea(WIND_AREA_X, WIND_AREA_Y, WIND_AREA_W, WIND_AREA_H);
        screenTextAt(WIND_AREA_X, WIND_AREA_Y, "Dir: %5s", getDirectionName((Direction)c->saveGame->orientation));
    }
    /* show the wind direction */
    else if ((c->location->context & CTX_NON_COMBAT) == c->location->context) {
        screenEraseTextArea(WIND_AREA_X, WIND_AREA_Y, WIND_AREA_W, WIND_AREA_H);
        screenTextAt(WIND_AREA_X, WIND_AREA_Y, "Wind %5s", getDirectionName((Direction) c->windDirection));
    }
}

void screenShowCursor() {
    Screen* scr = xu4.screen;
    if (! scr->cursorStatus && scr->cursorEnabled) {
        scr->cursorStatus = 1;
        screenUpdateCursor();
    }
}

void screenHideCursor() {
    Screen* scr = xu4.screen;
    if (scr->cursorStatus) {
        screenEraseTextArea(scr->cursorX, scr->cursorY, 1, 1);
    }
    scr->cursorStatus = 0;
}

void screenEnableCursor(void) {
    xu4.screen->cursorEnabled = 1;
}

void screenDisableCursor(void) {
    screenHideCursor();
    xu4.screen->cursorEnabled = 0;
}

void screenSetCursorPos(int x, int y) {
    Screen* scr = xu4.screen;
    scr->cursorX = x;
    scr->cursorY = y;
}

bool screenToggle3DDungeonView() {
    DungeonView* view = xu4.screen->dungeonView;
    if (view)
        return view->toggle3DDungeonView();
    return false;
}

void screenMakeDungeonView() {
    if (xu4.screen->dungeonView)
        return;
    xu4.screen->dungeonView = new DungeonView(BORDER_WIDTH, BORDER_HEIGHT,
                                              VIEWPORT_W, VIEWPORT_H);
}

#define BLOCKING(x,y)   blocking[(y) * VIEWPORT_W + (x)]
#define LOS(x,y)        lineOfSight[(y) * VIEWPORT_W + (x)]

/**
 * Finds which tiles in the viewport are visible from the avatars
 * location in the middle. (original DOS algorithm)
 */
static void screenFindLineOfSightDOS(const uint8_t* blocking, uint8_t* lineOfSight) {
    int x, y;
    const int halfW = VIEWPORT_W / 2;
    const int halfH = VIEWPORT_H / 2;

    LOS(halfW, halfH) = 1;

    for (x = halfW - 1; x >= 0; x--)
        if (LOS(x + 1, halfH) && ! BLOCKING(x + 1, halfH))
            LOS(x, halfH) = 1;

    for (x = halfW + 1; x < VIEWPORT_W; x++)
        if (LOS(x - 1, halfH) && ! BLOCKING(x - 1, halfH))
            LOS(x, halfH) = 1;

    for (y = halfH - 1; y >= 0; y--)
        if (LOS(halfW, y + 1) && ! BLOCKING(halfW, y + 1))
            LOS(halfW, y) = 1;

    for (y = halfH + 1; y < VIEWPORT_H; y++)
        if (LOS(halfW, y - 1) && ! BLOCKING(halfW, y - 1))
            LOS(halfW, y) = 1;

    for (y = halfH - 1; y >= 0; y--) {

        for (x = halfW - 1; x >= 0; x--) {
            if (LOS(x, y + 1) && ! BLOCKING(x, y + 1))
                LOS(x, y) = 1;
            else if (LOS(x + 1, y) && ! BLOCKING(x + 1, y))
                LOS(x, y) = 1;
            else if (LOS(x + 1, y + 1) && ! BLOCKING(x + 1, y + 1))
                LOS(x, y) = 1;
        }

        for (x = halfW + 1; x < VIEWPORT_W; x++) {
            if (LOS(x, y + 1) && ! BLOCKING(x, y + 1))
                LOS(x, y) = 1;
            else if (LOS(x - 1, y) && ! BLOCKING(x - 1, y))
                LOS(x, y) = 1;
            else if (LOS(x - 1, y + 1) && ! BLOCKING(x - 1, y + 1))
                LOS(x, y) = 1;
        }
    }

    for (y = halfH + 1; y < VIEWPORT_H; y++) {

        for (x = halfW - 1; x >= 0; x--) {
            if (LOS(x, y - 1) && ! BLOCKING(x, y - 1))
                LOS(x, y) = 1;
            else if (LOS(x + 1, y) && ! BLOCKING(x + 1, y))
                LOS(x, y) = 1;
            else if (LOS(x + 1, y - 1) && ! BLOCKING(x + 1, y - 1))
                LOS(x, y) = 1;
        }

        for (x = halfW + 1; x < VIEWPORT_W; x++) {
            if (LOS(x, y - 1) && ! BLOCKING(x, y - 1))
                LOS(x, y) = 1;
            else if (LOS(x - 1, y) && ! BLOCKING(x - 1, y))
                LOS(x, y) = 1;
            else if (LOS(x - 1, y - 1) && ! BLOCKING(x - 1, y - 1))
                LOS(x, y) = 1;
        }
    }
}

#if 1
/*
 * bitmasks for LOS shadows
 */
#define ____H 0x01    // obscured along the horizontal face
#define ___C_ 0x02    // obscured at the center
#define __V__ 0x04    // obscured along the vertical face
#define _N___ 0x80    // start of new raster

#define ___CH 0x03
#define __VCH 0x07
#define __VC_ 0x06

#define _N__H 0x81
#define _N_CH 0x83
#define _NVCH 0x87
#define _NVC_ 0x86
#define _NV__ 0x84

/**
 * Finds which tiles in the viewport are visible from the avatars
 * location in the middle.
 *
 * A new, more accurate LOS function
 *
 * Based somewhat off Andy McFadden's 1994 article,
 *   "Improvements to a Fast Algorithm for Calculating Shading
 *   and Visibility in a Two-Dimensional Field"
 *   -----
 *   http://www.fadden.com/techmisc/fast-los.html
 *
 * This function uses a lookup table to get the correct shadowmap,
 * therefore, the table will need to be updated if the viewport
 * dimensions increase. Also, the function assumes that the
 * viewport width and height are odd values and that the player
 * is always at the center of the screen.
 */
static void screenFindLineOfSightEnhanced(const uint8_t* blocking, uint8_t* lineOfSight) {
    /*
     * the shadow rasters for each viewport octant
     *
     * shadowRaster[0][0]    // number of raster segments in this shadow
     * shadowRaster[0][1]    // #1 shadow bitmask value (low three bits) + "newline" flag (high bit)
     * shadowRaster[0][2]    // #1 length
     * shadowRaster[0][3]    // #2 shadow bitmask value
     * shadowRaster[0][4]    // #2 length
     * shadowRaster[0][5]    // #3 shadow bitmask value
     * shadowRaster[0][6]    // #3 length
     * ...etc...
     */
    static const uint8_t colRasterIndex[5] = { 0, 0, 2, 5, 9 };
    static const uint8_t shadowRaster[14][13] = {
        { 6, __VCH, 4, _N_CH, 1, __VCH, 3, _N___, 1, ___CH, 1, __VCH, 1 },    // raster_1_0
        { 6, __VC_, 1, _NVCH, 2, __VC_, 1, _NVCH, 3, _NVCH, 2, _NVCH, 1 },    // raster_1_1
        //
        { 4, __VCH, 3, _N__H, 1, ___CH, 1, __VCH, 1,     0, 0,     0, 0 },    // raster_2_0
        { 6, __VC_, 2, _N_CH, 1, __VCH, 2, _N_CH, 1, __VCH, 1, _N__H, 1 },    // raster_2_1
        { 6, __V__, 1, _NVCH, 1, __VC_, 1, _NVCH, 1, __VC_, 1, _NVCH, 1 },    // raster_2_2
        //
        { 2, __VCH, 2, _N__H, 2,     0, 0,     0, 0,     0, 0,     0, 0 },    // raster_3_0
        { 3, __VC_, 2, _N_CH, 1, __VCH, 1,     0, 0,     0, 0,     0, 0 },    // raster_3_1
        { 3, __VC_, 1, _NVCH, 2, _N_CH, 1,     0, 0,     0, 0,     0, 0 },    // raster_3_2
        { 3, _NVCH, 1, __V__, 1, _NVCH, 1,     0, 0,     0, 0,     0, 0 },    // raster_3_3
        //
        { 2, __VCH, 1, _N__H, 1,     0, 0,     0, 0,     0, 0,     0, 0 },    // raster_4_0
        { 2, __VC_, 1, _N__H, 1,     0, 0,     0, 0,     0, 0,     0, 0 },    // raster_4_1
        { 2, __VC_, 1, _N_CH, 1,     0, 0,     0, 0,     0, 0,     0, 0 },    // raster_4_2
        { 2, __V__, 1, _NVCH, 1,     0, 0,     0, 0,     0, 0,     0, 0 },    // raster_4_3
        { 2, __V__, 1, _NVCH, 1,     0, 0,     0, 0,     0, 0,     0, 0 }     // raster_4_4
    };
    static const char octantSign[8 * 3] = {
    // xSign, ySign, reflect
         1,  1,  0,     // lower-right
         1,  1,  1,
         1, -1,  1,     // lower-left
        -1,  1,  0,
        -1, -1,  0,     // upper-left
        -1, -1,  1,
        -1,  1,  1,     // upper-right
         1, -1,  0,
    };

    /*
     * As each viewport tile is processed, it will store the bitmask for the shadow it casts.
     * Later, after processing all octants, the entire viewport will be marked visible except
     * for those tiles that have the __VCH bitmask.
     */
    const int _OCTANTS = 8;
    const int _NUM_RASTERS_COLS = 4;

    int octant;
    int xOrigin, yOrigin, xSign, ySign, reflect;
    int xTile, yTile, xTileOffset, yTileOffset;
    int currentRaster;
    int maxWidth, maxHeight;
    const char* osign = octantSign;

    // determine the origin point
    xOrigin = VIEWPORT_W / 2;
    yOrigin = VIEWPORT_H / 2;

    for (octant = 0; octant < _OCTANTS; octant++) {
        xSign   = *osign++;
        ySign   = *osign++;
        reflect = *osign++;

        // make sure the segment doesn't reach out of bounds
        if (reflect) {
            // swap height and width
            maxWidth  = yOrigin;
            maxHeight = xOrigin;
        } else {
            maxWidth  = xOrigin;
            maxHeight = yOrigin;
        }

        // check the visibility of each tile
        for (int currentCol = 1; currentCol <= _NUM_RASTERS_COLS; currentCol++) {
            for (int currentRow = 0; currentRow <= currentCol; currentRow++) {
                // swap X and Y to reflect the octant rasters
                if (reflect) {
                    xTile = xOrigin+(currentRow*ySign);
                    yTile = yOrigin+(currentCol*xSign);
                }
                else {
                    xTile = xOrigin+(currentCol*xSign);
                    yTile = yOrigin+(currentRow*ySign);
                }

                if (BLOCKING(xTile, yTile)) {
                    // a wall was detected, so go through the raster for this
                    // wall segment and mark everything behind it with the
                    // appropriate shadow bitmask.

                    // first, get the correct raster (0-13)
                    currentRaster = currentRow + colRasterIndex[currentCol];

                    xTileOffset = 0;
                    yTileOffset = 0;

                    //========================================
                    for (int currentSegment = 0; currentSegment < shadowRaster[currentRaster][0]; currentSegment++) {
                        // each shadow segment is 2 bytes
                        int shadowType   = shadowRaster[currentRaster][currentSegment*2+1];
                        int shadowLength = shadowRaster[currentRaster][currentSegment*2+2];

                        // update the raster length to make sure it fits in the viewport
                        shadowLength = (shadowLength+1+yTileOffset > maxWidth ? maxWidth : shadowLength);

                        // check to see if we should move up a row
                        if (shadowType & 0x80) {
                            // remove the flag from the shadowType
                            shadowType ^= _N___;
                            if (currentRow + yTileOffset > maxHeight)
                                break;

                            xTileOffset = yTileOffset;
                            yTileOffset++;
                        }

                        /* it is seemingly unnecessary to swap the edges for
                         * shadow tiles, because we only care about shadow
                         * tiles that have all three parts (V, C, and H)
                         * flagged.  if a tile has fewer than three, it is
                         * ignored during the draw phase, so vertical and
                         * horizontal shadow edge accuracy isn't important
                         */
                        // if reflecting the octant, swap the edges
                        /*
                        if (reflect) {
                            int shadowTemp = 0;
                            // swap the vertical and horizontal shadow edges
                            if (shadowType & __V__) { shadowTemp |= ____H; }
                            if (shadowType & ___C_) { shadowTemp |= ___C_; }
                            if (shadowType & ____H) { shadowTemp |= __V__; }
                            shadowType = shadowTemp;
                        }
                        */

                        for (int currentShadow = 1; currentShadow <= shadowLength; currentShadow++) {
                            // apply the shadow to the shadowMap
                            if (reflect) {
                                LOS(xTile + ((yTileOffset) * ySign), yTile + ((currentShadow+xTileOffset) * xSign)) |= shadowType;
                            }
                            else {
                                LOS(xTile + ((currentShadow+xTileOffset) * xSign), yTile + ((yTileOffset) * ySign)) |= shadowType;
                            }
                        }
                        xTileOffset += shadowLength;
                    }  // currentSegment
                    //========================================

                }  // BLOCKING
            }  // currentRow
        }  // currentCol
    }  // octant

    // go through all tiles on the viewable area and set the appropriate visibility
    uint8_t* end = lineOfSight + VIEWPORT_W * VIEWPORT_H;
    while (lineOfSight != end) {
        // if the shadow flags equal __VCH, hide it, otherwise it's fully visible
        if ((*lineOfSight & __VCH) == __VCH)
            *lineOfSight = 0;
        else
            *lineOfSight = 1;
        ++lineOfSight;
    }
}
#else

typedef struct {
    const uint8_t* blocking;
    uint8_t* visible;
    //float visible[VIEWPORT_W * VIEWPORT_H];
} GridSC;

#define GSC_TYPE                GridSC
#define GSC_XDIM(g)             VIEWPORT_W
#define GSC_YDIM(g)             VIEWPORT_H
#define GSC_IS_WALL(g,x,y)      g->blocking[VIEWPORT_W * y + x]
#define GSC_SET_LIGHT(g,x,y,ds) g->visible[VIEWPORT_W * y + x] = 1
#include "support/gridShadowCast.c"

static void screenFindLineOfSightEnhanced(const uint8_t* blocking, uint8_t* lineOfSight) {
    GridSC grid;
    int viewPos[2];

    grid.blocking = blocking;
    grid.visible  = lineOfSight;
    viewPos[0] = VIEWPORT_W/2;
    viewPos[1] = VIEWPORT_H/2;

#if 0
    // Mark cells as not visible.
    for (int i = 0; i < VIEWPORT_W*VIEWPORT_H; ++i)
        grid.visible[i] = -1.0f;
#endif

    gsc_computeVisibility(&grid, viewPos, 8.0f);

#if 0
    for (int i = 0; i < VIEWPORT_W*VIEWPORT_H; ++i) {
        lineOfSight[i] = (grid.visible[i] >= 0.0) ? 1 : 0;
        //printf( "KR los %d %f\n", i, grid.visible[i]);
    }
#endif
}
#endif

//#define CPU_TEST
#include "support/cpuCounter.h"

/**
 * Finds which tiles in the viewport are visible from the avatars
 * location in the middle.
 * Uses Screen blockingGrid to build screenLos.
 */
static void screenFindLineOfSight() {
    if (c->location->map->flags & NO_LINE_OF_SIGHT) {
        // The map has the no line of sight flag, all is visible
        memset(xu4.screen->screenLos, 1, VIEWPORT_W * VIEWPORT_H);
    } else {
        // otherwise calculate it from the map data
        memset(xu4.screen->screenLos, 0, VIEWPORT_W * VIEWPORT_H);

        CPU_START()
        if (xu4.settings->lineOfSight == 0)
            screenFindLineOfSightDOS(xu4.screen->blockingGrid,
                                     xu4.screen->screenLos);
        else
            screenFindLineOfSightEnhanced(xu4.screen->blockingGrid,
                                          xu4.screen->screenLos);
        CPU_END()
    }
}

/**
 * Generates terms a and b for equation "ax + b = y" that defines the
 * line containing the two given points.  Vertical lines are special
 * cased to return DBL_MAX for a and the x coordinate as b since they
 * cannot be represented with the above formula.
 */
static void screenGetLineTerms(int x1, int y1, int x2, int y2, double *a, double *b) {
    if (x2 - x1 == 0) {
        *a = DBL_MAX;
        *b = x1;
    }
    else {
        *a = ((double)(y2 - y1)) / ((double)(x2 - x1));
        *b = y1 - ((*a) * x1);
    }
}

/**
 * Determine if two points are on the same side of a line (or both on
 * the line).  The line is defined by the terms a and b of the
 * equation "ax + b = y".
 */
static int screenPointsOnSameSideOfLine(int x1, int y1, int x2, int y2, double a, double b) {
    double p1, p2;

    if (a == DBL_MAX) {
        p1 = x1 - b;
        p2 = x2 - b;
    }
    else {
        p1 = x1 * a + b - y1;
        p2 = x2 * a + b - y2;
    }

    if ((p1 > 0.0 && p2 > 0.0) ||
        (p1 < 0.0 && p2 < 0.0) ||
        (p1 == 0.0 && p2 == 0.0))
        return 1;

    return 0;
}

static int screenPointInTriangle(int x, int y, int tx1, int ty1, int tx2, int ty2, int tx3, int ty3) {
    double a[3], b[3];

    screenGetLineTerms(tx1, ty1, tx2, ty2, &(a[0]), &(b[0]));
    screenGetLineTerms(tx2, ty2, tx3, ty3, &(a[1]), &(b[1]));
    screenGetLineTerms(tx3, ty3, tx1, ty1, &(a[2]), &(b[2]));

    if (!screenPointsOnSameSideOfLine(x, y, tx3, ty3, a[0], b[0]))
        return 0;
    if (!screenPointsOnSameSideOfLine(x, y, tx1, ty1, a[1], b[1]))
        return 0;
    if (!screenPointsOnSameSideOfLine(x, y, tx2, ty2, a[2], b[2]))
        return 0;

    return 1;
}

/**
 * Determine if the given point is within a mouse area.
 */
int screenPointInMouseArea(int x, int y, MouseArea *area) {
    ASSERT(area->npoints == 2 || area->npoints == 3, "unsupported number of points in area: %d", area->npoints);
    unsigned int scale = xu4.settings->scale;

    /* two points define a rectangle */
    if (area->npoints == 2) {
        if (x >= (int)(area->point[0].x * scale) &&
            y >= (int)(area->point[0].y * scale) &&
            x < (int)(area->point[1].x * scale) &&
            y < (int)(area->point[1].y * scale)) {
            return 1;
        }
    }

    /* three points define a triangle */
    else if (area->npoints == 3) {
        return screenPointInTriangle(x, y,
                                     area->point[0].x * scale, area->point[0].y * scale,
                                     area->point[1].x * scale, area->point[1].y * scale,
                                     area->point[2].x * scale, area->point[2].y * scale);
    }

    return 0;
}

void screenRedrawMapArea() {
    xu4.game->mapArea.update();
}

void screenEraseMapArea() {
    SCALED_VAR
    xu4.screenImage->fillRect(SCALED(BORDER_WIDTH), SCALED(BORDER_HEIGHT),
                              VIEWPORT_W * SCALED(TILE_WIDTH),
                              VIEWPORT_H * SCALED(TILE_HEIGHT),
                              0, 0, 0);
}

void screenEraseTextArea(int x, int y, int width, int height) {
    SCALED_VAR
    int charW = SCALED(CHAR_WIDTH);
    int charH = SCALED(CHAR_HEIGHT);
    xu4.screenImage->fillRect(x * charW, y * charH,
                              width * charW, height * charH, 0, 0, 0);
}

/**
 * Do the tremor spell effect where the screen shakes.
 */
void screenShake(int iterations) {
    if (xu4.settings->screenShakes) {
        Screen* scr = xu4.screen;
        SCALED_VAR
        int shakeOffset = SCALED(1);

        for (int i = 0; i < iterations; i++) {
            // shift the screen down and make the top row black
            scr->state.vertOffset = shakeOffset;
            EventHandler::wait_msecs(xu4.settings->shakeInterval);

            // shift the screen back up
            scr->state.vertOffset = 0;
            EventHandler::wait_msecs(xu4.settings->shakeInterval);
        }
    }
}

/**
 * Draw a tile graphic on the screen.
 */
static void screenShowGemTile(const Layout *layout, Map *map, MapTile &t, bool focus, int x, int y) {
    Screen* scr = xu4.screen;
    unsigned int scale = xu4.settings->scale;
    unsigned int tile = xu4.config->usaveIds()->ultimaId(t);

    if (map->type == Map::DUNGEON) {
        ImageInfo* charsetInfo = scr->charsetInfo;
        ASSERT(charsetInfo, "charset not initialized");

        std::map<string, int>::iterator charIndex = scr->dungeonTileChars.find(t.getTileType()->nameStr());
        if (charIndex != scr->dungeonTileChars.end()) {
            charsetInfo->image->drawSubRect(
                (layout->viewport.x + (x * layout->tileshape.width)) * scale,
                (layout->viewport.y + (y * layout->tileshape.height)) * scale,
                0,
                charIndex->second * layout->tileshape.height * scale,
                layout->tileshape.width * scale,
                layout->tileshape.height * scale);
        }
    }
    else {
        if (scr->gemTilesInfo == NULL) {
            scr->gemTilesInfo = xu4.imageMgr->get(BKGD_GEMTILES);
            if (! scr->gemTilesInfo)
                errorLoadImage(BKGD_GEMTILES);
        }

        if (tile < 128) {
            scr->gemTilesInfo->image->drawSubRect(
                (layout->viewport.x + (x * layout->tileshape.width)) * scale,
                (layout->viewport.y + (y * layout->tileshape.height)) * scale,
                0,
                tile * layout->tileshape.height * scale,
                layout->tileshape.width * scale,
                layout->tileshape.height * scale);
        } else {
            xu4.screenImage->fillRect(
                (layout->viewport.x + (x * layout->tileshape.width)) * scale,
                (layout->viewport.y + (y * layout->tileshape.height)) * scale,
                layout->tileshape.width * scale,
                layout->tileshape.height * scale,
                0, 0, 0);
        }
    }
}

void screenGemUpdate() {
    MapTile tile;
    int x, y;
    unsigned int scale = xu4.settings->scale;

    xu4.screenImage->fillRect(BORDER_WIDTH * scale,
                              BORDER_HEIGHT * scale,
                              VIEWPORT_W * TILE_WIDTH * scale,
                              VIEWPORT_H * TILE_HEIGHT * scale,
                              0, 0, 0);

    const Layout *layout;
    if (c->location->map->type == Map::DUNGEON)
        layout = xu4.screen->dungeonGemLayout;
    else
        layout = xu4.screen->gemLayout;


    //TODO, move the code responsible for determining 'peer' visibility to a non SDL specific part of the code.
    if (c->location->map->type == Map::DUNGEON) {
        //DO THE SPECIAL DUNGEON MAP TRAVERSAL
        std::vector<std::vector<int> > drawnTiles(layout->viewport.width, vector<int>(layout->viewport.height, 0));
        std::list<std::pair<int,int> > coordStack;

        //Put the avatar's position on the stack
        int center_x = layout->viewport.width / 2 - 1;
        int center_y = layout->viewport.height / 2 - 1;
        int avt_x = c->location->coords.x - 1;
        int avt_y = c->location->coords.y - 1;

        coordStack.push_back(std::pair<int,int>(center_x, center_y));
        bool weAreDrawingTheAvatarTile = true;

        //And draw each tile on the growing stack until it is empty
        while (coordStack.size() > 0) {
            std::pair<int,int> currentXY = coordStack.back();
            coordStack.pop_back();

            x = currentXY.first;
            y = currentXY.second;

            if (    x < 0 || x >= layout->viewport.width ||
                y < 0 || y >= layout->viewport.height)
                continue;   //Skip out of range tiles

            if (drawnTiles[x][y])
                continue;   //Skip already considered tiles

            drawnTiles[x][y] = 1;

            // DRAW THE ACTUAL TILE
            bool focus;


            vector<MapTile> tiles = screenViewportTile(layout->viewport.width,
                                                       layout->viewport.height, x - center_x + avt_x, y - center_y + avt_y, focus);
            tile = tiles.front();

            TileId avatarTileId = c->location->map->tileset->getByName(Tile::sym.avatar)->getId();


            if (!weAreDrawingTheAvatarTile)
            {
                //Hack to avoid showing the avatar tile multiple times in cycling dungeon maps
                if (tile.getId() == avatarTileId)
                    tile = c->location->map->getTileFromData(c->location->coords);
            }

            screenShowGemTile(layout, c->location->map, tile, focus, x, y);

            if (!tile.getTileType()->isOpaque() || tile.getTileType()->isWalkable() ||  weAreDrawingTheAvatarTile)
            {
                //Continue the search so we can see through all walkable objects, non-opaque objects (like creatures)
                //or the avatar position in those rare circumstances where he is stuck in a wall

                //by adding all relative adjacency combinations to the stack for drawing
                coordStack.push_back(std::pair<int,int>(x   + 1 ,   y   - 1 ));
                coordStack.push_back(std::pair<int,int>(x   + 1 ,   y       ));
                coordStack.push_back(std::pair<int,int>(x   + 1 ,   y   + 1 ));

                coordStack.push_back(std::pair<int,int>(x       ,   y   - 1 ));
                coordStack.push_back(std::pair<int,int>(x       ,   y   + 1 ));

                coordStack.push_back(std::pair<int,int>(x   - 1 ,   y   - 1 ));
                coordStack.push_back(std::pair<int,int>(x   - 1 ,   y       ));
                coordStack.push_back(std::pair<int,int>(x   - 1 ,   y   + 1 ));

                // We only draw the avatar tile once, it is the first tile drawn
                weAreDrawingTheAvatarTile = false;
            }
        }

    } else {
        //DO THE REGULAR EVERYTHING-IS-VISIBLE MAP TRAVERSAL
        for (x = 0; x < layout->viewport.width; x++) {
            for (y = 0; y < layout->viewport.height; y++) {
                bool focus;
                tile = screenViewportTile(layout->viewport.width,
                                          layout->viewport.height, x, y, focus).front();
                screenShowGemTile(layout, c->location->map, tile, focus, x, y);
            }
        }
    }

    screenRedrawMapArea();

    screenUpdateCursor();
    screenUpdateMoons();
    screenUpdateWind();
}

/**
 * Scale an image up.  The resulting image will be scale * the
 * original dimensions.  The original image is no longer deleted.
 * n is the number of tiles in the image; each tile is filtered
 * seperately. filter determines whether or not to filter the
 * resulting image.
 */
Image *screenScale(Image *src, int scale, int n, int filter) {
    Image *dest = NULL;

    if (n == 0)
        n = 1;

    Scaler filterScaler = xu4.screen->filterScaler;
    if (filterScaler) {
        while (filter && (scale % 2 == 0)) {
            dest = (*filterScaler)(src, 2, n);
            src = dest;
            scale /= 2;
        }
        if (scale == 3 && scaler3x(xu4.settings->filter)) {
            dest = (*filterScaler)(src, 3, n);
            src = dest;
            scale /= 3;
        }
    }

    if (scale != 1)
        dest = (*scalerGet(ScreenFilter_point))(src, scale, n);

    if (!dest)
        dest = Image::duplicate(src);

    return dest;
}

/**
 * Scale an image down.  The resulting image will be 1/scale * the
 * original dimensions.  The original image is no longer deleted.
 */
Image *screenScaleDown(Image *src, int scale) {
    int x, y;
    Image *dest;

    dest = Image::create(src->width() / scale, src->height() / scale);
    if (!dest)
        return NULL;

    for (y = 0; y < src->height(); y+=scale) {
        for (x = 0; x < src->width(); x+=scale) {
            unsigned int index;
            src->getPixelIndex(x, y, index);
            dest->putPixelIndex(x / scale, y / scale, index);
        }
    }

    return dest;
}

ScreenState* screenState() {
    return &xu4.screen->state;
}

#ifdef IOS
//Unsure if implementation required in iOS.
void inline screenWait(int numberOfAnimationFrames){};
#endif
