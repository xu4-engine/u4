/*
 * $Id$
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

static void screenLoadLayoutsFromConf();

static Scaler filterScaler;
static vector<string> gemLayoutNames;
static const Layout* gemLayout = NULL;
static const Layout* dungeonGemLayout = NULL;
std::map<string, int> dungeonTileChars;
TileAnimSet *tileanims = NULL;
ImageInfo *charsetInfo = NULL;
ImageInfo *gemTilesInfo = NULL;

void screenFindLineOfSight(vector<MapTile> viewportTiles[VIEWPORT_W][VIEWPORT_H]);
void screenFindLineOfSightDOS(vector<MapTile> viewportTiles[VIEWPORT_W][VIEWPORT_H]);
void screenFindLineOfSightEnhanced(vector<MapTile> viewportTiles[VIEWPORT_W][VIEWPORT_H]);

int screenNeedPrompt = 1;
int screenCurrentCycle = 0;
int screenCursorX = 0;
int screenCursorY = 0;
int screenCursorStatus = 0;
int screenCursorEnabled = 1;
int screenVertOffset = 0;
int screenLos[VIEWPORT_W][VIEWPORT_H];

static const int BufferSize = 1024;

extern bool verbose;

// Just extern the system functions here. That way people aren't tempted to call them as part of the public API.
extern void screenInit_sys(const Settings*, int reset);
extern void screenDelete_sys();

static void screenInit_data(Settings& settings) {
    filterScaler = scalerGet(settings.filter);
    if (!filterScaler)
        errorFatal("Invalid filter %d", settings.filter);

    // Create a special purpose image that represents the whole screen.
    xu4.screenImage = Image::create(320 * settings.scale, 200 * settings.scale);
    xu4.screenImage->fill(Image::black);

    charsetInfo = xu4.imageMgr->get(BKGD_CHARSET);
    if (!charsetInfo)
        errorFatal("ERROR 1001: Unable to load the \"%s\" data file.\t\n\nIs %s installed?\n\nVisit the XU4 website for additional information.\n\thttp://xu4.sourceforge.net/", BKGD_CHARSET, settings.game.c_str());

    /* if we can't use vga, reset to default:ega */
    if (!u4isUpgradeAvailable() && settings.videoType == "VGA")
        settings.videoType = "EGA";

#ifdef USE_GL
    ImageInfo* shapes = xu4.imageMgr->get(BKGD_SHAPES);
    gpu_setTilesTexture(xu4.gpu, shapes->tex);
#endif

    assert(tileanims == NULL);
    tileanims = xu4.config->newTileAnims( settings.videoType.c_str() );
    if (! tileanims)
        errorFatal("Unable to find \"%s\" tile animations", settings.videoType.c_str());

    gemTilesInfo = NULL;
    screenLoadLayoutsFromConf();

    if (verbose)
        printf("using %s scaler\n", screenGetFilterNames()[ settings.filter ]);

    EventHandler::setKeyRepeat(settings.keydelay, settings.keyinterval);

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

    Tileset::loadImages();
}

static void screenDelete_data() {
    Tileset::unloadImages();

    delete tileanims;
    tileanims = NULL;

    delete xu4.screenImage;
    xu4.screenImage = NULL;
}


enum ScreenSystemStage {
    SYS_CLEAN = 0,  // Initial screen setup.
    SYS_RESET = 1   // Reconfigure screen settings.
};

/*
 * Sets xu4.screen & xu4.gpu pointers.
 */
void screenInit() {
    screenInit_sys(xu4.settings, SYS_CLEAN);
    screenInit_data(*xu4.settings);
}

void screenDelete() {
    screenDelete_data();
    screenDelete_sys();
}

/**
 * Re-initializes the screen and implements any changes made in settings
 */
void screenReInit() {
    screenDelete_data();
    screenInit_sys(xu4.settings, SYS_RESET);
    screenInit_data(*xu4.settings); // Load new backgrounds, etc.
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
    if (screenNeedPrompt && screenCursorEnabled && c->col == 0) {
        screenMessage("%c", CHARSET_PROMPT);
        screenNeedPrompt = 0;
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
    if (c->line == 12) {
        screenScrollMessageArea();
        c->line--;
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

    screenNeedPrompt = 1;
}

const vector<string> &screenGetGemLayoutNames() {
    return gemLayoutNames;
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

static void screenLoadLayoutsFromConf() {
    // Save gem layout names and find one to use.
    uint32_t count;
    uint32_t i;
    const Layout* layout = xu4.config->layouts(&count);

    gemLayout = dungeonGemLayout = NULL;
    gemLayoutNames.clear();

    for (i = 0; i < count; ++i) {
        if (layout[i].type == LAYOUT_GEM) {
            const char* name = xu4.config->symbolName( layout[i].name );
            gemLayoutNames.push_back(name);

            if (! gemLayout && xu4.settings->gemLayout == name)
                gemLayout = layout + i;
        } else if (layout[i].type == LAYOUT_DUNGEONGEM) {
            if (! dungeonGemLayout)
                dungeonGemLayout = layout + i;
        }
    }

    if (! gemLayout)
        errorFatal("no gem layout named %s found!\n", xu4.settings->gemLayout.c_str());
    if (! dungeonGemLayout)
        errorFatal("no dungeon gem layout found!\n");
}

vector<MapTile> screenViewportTile(unsigned int width, unsigned int height, int x, int y, bool &focus) {
    MapCoords center = c->location->coords;
    static MapTile grass = c->location->map->tileset->getByName(Tile::sym.grass)->getId();

    if (c->location->map->width <= width &&
        c->location->map->height <= height) {
        center.x = c->location->map->width / 2;
        center.y = c->location->map->height / 2;
    }

    MapCoords tc = center;

    tc.x += x - (width / 2);
    tc.y += y - (height / 2);

    /* Wrap the location if we can */
    tc.wrap(c->location->map);

    /* off the edge of the map: pad with grass tiles */
    if (MAP_IS_OOB(c->location->map, tc)) {
        focus = false;
        vector<MapTile> result;
        result.push_back(grass);
        return result;
    }

    return c->location->tilesAt(tc, focus);
}

bool screenTileUpdate(TileView *view, const Coords &coords, bool redraw)
{
    if (c->location->map->flags & FIRST_PERSON)
        return false;

    // Get the tiles
    bool focus;
    MapCoords mc(coords);
    mc.wrap(c->location->map);
    vector<MapTile> tiles = c->location->tilesAt(mc, focus);

    // Get the screen coordinates
    int x = coords.x;
    int y = coords.y;

    if (c->location->map->width > VIEWPORT_W || c->location->map->height > VIEWPORT_H)
    {
        //Center the coordinates to the viewport if you're on centered-view map.
        x = x - c->location->coords.x + VIEWPORT_W / 2;
        y = y - c->location->coords.y + VIEWPORT_H / 2;
    }

    // Draw if it is on screen
    if (x >= 0 && y >= 0 && x < VIEWPORT_W && y < VIEWPORT_H && screenLos[x][y])
    {
        view->drawTile(tiles, focus, x, y);

        if (redraw)
        {
            //screenRedrawMapArea();
        }
        return true;
    }
    return false;
}

#ifdef USE_GL
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

#ifdef USE_GL
    //static const float clearColor[4] = {0.0, 0.5, 0.8, 1.0};
    const Image* screen = xu4.screenImage;
    Location* loc = c->location;
    const Map* map = loc->map;

    if (blackout) {
        screenEraseMapArea();
        goto raster_update;
    }
    else if (map->flags & FIRST_PERSON) {
        DungeonViewer.display(c, view);
        xu4.game->mapArea.update();

raster_update:
        screenUpdateCursor();
        screenUpdateMoons();
        screenUpdateWind();

        gpu_viewport(0, 0, screen->width(), screen->height());
        gpu_background(xu4.gpu, NULL, screen);
    }
    else if (showmap) {
        ImageInfo* shapes = xu4.imageMgr->get(BKGD_SHAPES);
        const MapCoords& coord = loc->coords;   // Center of view.

        screenUpdateCursor();
        screenUpdateMoons();
        screenUpdateWind();

        gpu_viewport(0, 0, screen->width(), screen->height());
        gpu_background(xu4.gpu, NULL, screen);

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
        DungeonViewer.display(c, view);
        screenRedrawMapArea();
    }
    else if (showmap) {
        MapTile black = c->location->map->tileset->getByName(Tile::sym.black)->getId();
        vector<MapTile> viewportTiles[VIEWPORT_W][VIEWPORT_H];
        bool viewportFocus[VIEWPORT_W][VIEWPORT_H];
        int x, y;

        for (y = 0; y < VIEWPORT_H; y++) {
            for (x = 0; x < VIEWPORT_W; x++) {
                viewportTiles[x][y] = screenViewportTile(VIEWPORT_W, VIEWPORT_H, x, y, viewportFocus[x][y]);
            }
        }

        screenFindLineOfSight(viewportTiles);

        for (y = 0; y < VIEWPORT_H; y++) {
            for (x = 0; x < VIEWPORT_W; x++) {
                if (screenLos[x][y]) {
                    view->drawTile(viewportTiles[x][y], viewportFocus[x][y], x, y);
                }
                else
                    view->drawTile(black, false, x, y);
            }
        }
        screenRedrawMapArea();
    }

    screenUpdateCursor();
    screenUpdateMoons();
    screenUpdateWind();
#endif
}

/**
 * Draw an image or subimage on the screen.
 */
void screenDrawImage(const string &name, int x, int y) {
    ImageInfo *info = xu4.imageMgr->get(name);
    if (info) {
        info->image->draw(x, y);
        return;
    }

    SubImage *subimage = xu4.imageMgr->getSubImage(name);
    if (subimage)
        info = xu4.imageMgr->get(subimage->srcImageName);

    if (info) {
        if (info) {
            unsigned int scale = xu4.settings->scale;
            info->image->drawSubRect(x, y,
                                     subimage->x * (scale / info->prescale),
                                     subimage->y * (scale / info->prescale),
                                     subimage->width * (scale / info->prescale),
                                     subimage->height * (scale / info->prescale));
            return;
        }
    }
    errorFatal("ERROR 1006: Unable to load the image \"%s\".\t\n\nIs %s installed?\n\nVisit the XU4 website for additional information.\n\thttp://xu4.sourceforge.net/", name.c_str(), xu4.settings->game.c_str());
}

void screenDrawImageInMapArea(const string &name) {
    ImageInfo *info;

    info = xu4.imageMgr->get(name);
    if (!info)
        errorFatal("ERROR 1004: Unable to load data files.\t\n\nIs %s installed?\n\nVisit the XU4 website for additional information.\n\thttp://xu4.sourceforge.net/", xu4.settings->game.c_str());

    unsigned int scale = xu4.settings->scale;
    info->image->drawSubRect(BORDER_WIDTH * scale, BORDER_HEIGHT * scale,
                             BORDER_WIDTH * scale, BORDER_HEIGHT * scale,
                             VIEWPORT_W * TILE_WIDTH * scale,
                             VIEWPORT_H * TILE_HEIGHT * scale);
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
            charsetInfo->image->setFontColorFG((ColorFG)color);
    }
}

/**
 * Draw a character from the charset onto the screen.
 */
void screenShowChar(int chr, int x, int y) {
    unsigned int scale = xu4.settings->scale;
    charsetInfo->image->drawSubRect(x * charsetInfo->image->width(), y * (CHAR_HEIGHT * scale),
                                    0, chr * (CHAR_HEIGHT * scale),
                                    charsetInfo->image->width(), CHAR_HEIGHT * scale);
}

/**
 * Scroll the text in the message area up one position.
 */
void screenScrollMessageArea() {
    ASSERT(charsetInfo != NULL && charsetInfo->image != NULL, "charset not initialized!");

    unsigned int scale = xu4.settings->scale;
    Image* screen = xu4.screenImage;

    screen->drawSubRectOn(screen,
                          TEXT_AREA_X * charsetInfo->image->width(),
                          TEXT_AREA_Y * CHAR_HEIGHT * scale,
                          TEXT_AREA_X * charsetInfo->image->width(),
                          (TEXT_AREA_Y + 1) * CHAR_HEIGHT * scale,
                          TEXT_AREA_W * charsetInfo->image->width(),
                          (TEXT_AREA_H - 1) * CHAR_HEIGHT * scale);


    screen->fillRect(TEXT_AREA_X * charsetInfo->image->width(),
                     TEXT_AREA_Y * CHAR_HEIGHT * scale + (TEXT_AREA_H - 1) * CHAR_HEIGHT * scale,
                     TEXT_AREA_W * charsetInfo->image->width(),
                     CHAR_HEIGHT * scale,
                     0, 0, 0);
}

void screenCycle() {
    if (++screenCurrentCycle >= SCR_CYCLE_MAX)
        screenCurrentCycle = 0;
}

void screenUpdateCursor() {
    int phase = screenCurrentCycle * SCR_CYCLE_PER_SECOND / SCR_CYCLE_MAX;

    ASSERT(phase >= 0 && phase < 4, "derived an invalid cursor phase: %d", phase);

    if (screenCursorStatus) {
        screenShowChar(31 - phase, screenCursorX, screenCursorY);
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
    if (!screenCursorStatus && screenCursorEnabled) {
        screenCursorStatus = 1;
        screenUpdateCursor();
    }
}

void screenHideCursor() {
    if (screenCursorStatus) {
        screenEraseTextArea(screenCursorX, screenCursorY, 1, 1);
    }
    screenCursorStatus = 0;
}

void screenEnableCursor(void) {
    screenCursorEnabled = 1;
}

void screenDisableCursor(void) {
    screenHideCursor();
    screenCursorEnabled = 0;
}

void screenSetCursorPos(int x, int y) {
    screenCursorX = x;
    screenCursorY = y;
}

/**
 * Finds which tiles in the viewport are visible from the avatars
 * location in the middle. (original DOS algorithm)
 */
void screenFindLineOfSight(vector <MapTile> viewportTiles[VIEWPORT_W][VIEWPORT_H]) {
    int x, y;

    if (!c)
        return;

    /*
     * if the map has the no line of sight flag, all is visible
     */
    if (c->location->map->flags & NO_LINE_OF_SIGHT) {
        for (y = 0; y < VIEWPORT_H; y++) {
            for (x = 0; x < VIEWPORT_W; x++) {
                screenLos[x][y] = 1;
            }
        }
        return;
    }

    /*
     * otherwise calculate it from the map data
     */
    for (y = 0; y < VIEWPORT_H; y++) {
        for (x = 0; x < VIEWPORT_W; x++) {
            screenLos[x][y] = 0;
        }
    }

    if (xu4.settings->lineOfSight == 0)
        screenFindLineOfSightDOS(viewportTiles);
    else
        screenFindLineOfSightEnhanced(viewportTiles);
}


/**
 * Finds which tiles in the viewport are visible from the avatars
 * location in the middle. (original DOS algorithm)
 */
void screenFindLineOfSightDOS(vector <MapTile> viewportTiles[VIEWPORT_W][VIEWPORT_H]) {
    int x, y;

    screenLos[VIEWPORT_W / 2][VIEWPORT_H / 2] = 1;

    for (x = VIEWPORT_W / 2 - 1; x >= 0; x--)
        if (screenLos[x + 1][VIEWPORT_H / 2] &&
            !viewportTiles[x + 1][VIEWPORT_H / 2].front().getTileType()->isOpaque())
            screenLos[x][VIEWPORT_H / 2] = 1;

    for (x = VIEWPORT_W / 2 + 1; x < VIEWPORT_W; x++)
        if (screenLos[x - 1][VIEWPORT_H / 2] &&
            !viewportTiles[x - 1][VIEWPORT_H / 2].front().getTileType()->isOpaque())
            screenLos[x][VIEWPORT_H / 2] = 1;

    for (y = VIEWPORT_H / 2 - 1; y >= 0; y--)
        if (screenLos[VIEWPORT_W / 2][y + 1] &&
            !viewportTiles[VIEWPORT_W / 2][y + 1].front().getTileType()->isOpaque())
            screenLos[VIEWPORT_W / 2][y] = 1;

    for (y = VIEWPORT_H / 2 + 1; y < VIEWPORT_H; y++)
        if (screenLos[VIEWPORT_W / 2][y - 1] &&
            !viewportTiles[VIEWPORT_W / 2][y - 1].front().getTileType()->isOpaque())
            screenLos[VIEWPORT_W / 2][y] = 1;

    for (y = VIEWPORT_H / 2 - 1; y >= 0; y--) {

        for (x = VIEWPORT_W / 2 - 1; x >= 0; x--) {
            if (screenLos[x][y + 1] &&
                !viewportTiles[x][y + 1].front().getTileType()->isOpaque())
                screenLos[x][y] = 1;
            else if (screenLos[x + 1][y] &&
                     !viewportTiles[x + 1][y].front().getTileType()->isOpaque())
                screenLos[x][y] = 1;
            else if (screenLos[x + 1][y + 1] &&
                     !viewportTiles[x + 1][y + 1].front().getTileType()->isOpaque())
                screenLos[x][y] = 1;
        }

        for (x = VIEWPORT_W / 2 + 1; x < VIEWPORT_W; x++) {
            if (screenLos[x][y + 1] &&
                !viewportTiles[x][y + 1].front().getTileType()->isOpaque())
                screenLos[x][y] = 1;
            else if (screenLos[x - 1][y] &&
                     !viewportTiles[x - 1][y].front().getTileType()->isOpaque())
                screenLos[x][y] = 1;
            else if (screenLos[x - 1][y + 1] &&
                     !viewportTiles[x - 1][y + 1].front().getTileType()->isOpaque())
                screenLos[x][y] = 1;
        }
    }

    for (y = VIEWPORT_H / 2 + 1; y < VIEWPORT_H; y++) {

        for (x = VIEWPORT_W / 2 - 1; x >= 0; x--) {
            if (screenLos[x][y - 1] &&
                !viewportTiles[x][y - 1].front().getTileType()->isOpaque())
                screenLos[x][y] = 1;
            else if (screenLos[x + 1][y] &&
                     !viewportTiles[x + 1][y].front().getTileType()->isOpaque())
                screenLos[x][y] = 1;
            else if (screenLos[x + 1][y - 1] &&
                     !viewportTiles[x + 1][y - 1].front().getTileType()->isOpaque())
                screenLos[x][y] = 1;
        }

        for (x = VIEWPORT_W / 2 + 1; x < VIEWPORT_W; x++) {
            if (screenLos[x][y - 1] &&
                !viewportTiles[x][y - 1].front().getTileType()->isOpaque())
                screenLos[x][y] = 1;
            else if (screenLos[x - 1][y] &&
                     !viewportTiles[x - 1][y].front().getTileType()->isOpaque())
                screenLos[x][y] = 1;
            else if (screenLos[x - 1][y - 1] &&
                     !viewportTiles[x - 1][y - 1].front().getTileType()->isOpaque())
                screenLos[x][y] = 1;
        }
    }
}

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
void screenFindLineOfSightEnhanced(vector <MapTile> viewportTiles[VIEWPORT_W][VIEWPORT_H]) {
    int x, y;

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
    const int shadowRaster[14][13] = {
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

    /*
     * As each viewport tile is processed, it will store the bitmask for the shadow it casts.
     * Later, after processing all octants, the entire viewport will be marked visible except
     * for those tiles that have the __VCH bitmask.
     */
    const int _OCTANTS = 8;
    const int _NUM_RASTERS_COLS = 4;

    int octant;
    int xOrigin, yOrigin, xSign, ySign, reflect, xTile, yTile, xTileOffset, yTileOffset;

    for (octant = 0; octant < _OCTANTS; octant++) {
        switch (octant) {
            case 0:  xSign=  1;  ySign=  1;  reflect=false;  break;        // lower-right
            case 1:  xSign=  1;  ySign=  1;  reflect=true;   break;
            case 2:  xSign=  1;  ySign= -1;  reflect=true;   break;        // lower-left
            case 3:  xSign= -1;  ySign=  1;  reflect=false;  break;
            case 4:  xSign= -1;  ySign= -1;  reflect=false;  break;        // upper-left
            case 5:  xSign= -1;  ySign= -1;  reflect=true;   break;
            case 6:  xSign= -1;  ySign=  1;  reflect=true;   break;        // upper-right
            case 7:  xSign=  1;  ySign= -1;  reflect=false;  break;
        }

        // determine the origin point for the current LOS octant
        xOrigin = VIEWPORT_W / 2;
        yOrigin = VIEWPORT_H / 2;

        // make sure the segment doesn't reach out of bounds
        int maxWidth      = xOrigin;
        int maxHeight     = yOrigin;
        int currentRaster = 0;

        // just in case the viewport isn't square, swap the width and height
        if (reflect) {
            // swap height and width for later use
            maxWidth ^= maxHeight;
            maxHeight ^= maxWidth;
            maxWidth ^= maxHeight;
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

                if (viewportTiles[xTile][yTile].front().getTileType()->isOpaque()) {
                    // a wall was detected, so go through the raster for this wall
                    // segment and mark everything behind it with the appropriate
                    // shadow bitmask.
                    //
                    // first, get the correct raster
                    //
                    if ((currentCol==1) && (currentRow==0)) { currentRaster=0; }
                    else if ((currentCol==1) && (currentRow==1)) { currentRaster=1; }
                    else if ((currentCol==2) && (currentRow==0)) { currentRaster=2; }
                    else if ((currentCol==2) && (currentRow==1)) { currentRaster=3; }
                    else if ((currentCol==2) && (currentRow==2)) { currentRaster=4; }
                    else if ((currentCol==3) && (currentRow==0)) { currentRaster=5; }
                    else if ((currentCol==3) && (currentRow==1)) { currentRaster=6; }
                    else if ((currentCol==3) && (currentRow==2)) { currentRaster=7; }
                    else if ((currentCol==3) && (currentRow==3)) { currentRaster=8; }
                    else if ((currentCol==4) && (currentRow==0)) { currentRaster=9; }
                    else if ((currentCol==4) && (currentRow==1)) { currentRaster=10; }
                    else if ((currentCol==4) && (currentRow==2)) { currentRaster=11; }
                    else if ((currentCol==4) && (currentRow==3)) { currentRaster=12; }
                    else { currentRaster=13; }  // currentCol and currentRow must equal 4

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
//                            if (currentRow + yTileOffset >= maxHeight) {
                            if (currentRow + yTileOffset > maxHeight) {
                                break;
                            }
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
//                        if (reflect) {
//                            int shadowTemp = 0;
//                            // swap the vertical and horizontal shadow edges
//                            if (shadowType & __V__) { shadowTemp |= ____H; }
//                            if (shadowType & ___C_) { shadowTemp |= ___C_; }
//                            if (shadowType & ____H) { shadowTemp |= __V__; }
//                            shadowType = shadowTemp;
//                        }

                        for (int currentShadow = 1; currentShadow <= shadowLength; currentShadow++) {
                            // apply the shadow to the shadowMap
                            if (reflect) {
                                screenLos[xTile + ((yTileOffset) * ySign)][yTile + ((currentShadow+xTileOffset) * xSign)] |= shadowType;
                            }
                            else {
                                screenLos[xTile + ((currentShadow+xTileOffset) * xSign)][yTile + ((yTileOffset) * ySign)] |= shadowType;
                            }
                        }
                        xTileOffset += shadowLength;
                    }  // for (int currentSegment = 0; currentSegment < shadowRaster[currentRaster][0]; currentSegment++)
                    //========================================

                }  // if (viewportTiles[xTile][yTile].front().getTileType()->isOpaque())
            }  // for (int currentRow = 0; currentRow <= currentCol; currentRow++)
        }  // for (int currentCol = 1; currentCol <= _NUM_RASTERS_COLS; currentCol++)
    }  // for (octant = 0; octant < _OCTANTS; octant++)

    // go through all tiles on the viewable area and set the appropriate visibility
    for (y = 0; y < VIEWPORT_H; y++) {
        for (x = 0; x < VIEWPORT_W; x++) {
            // if the shadow flags equal __VCH, hide it, otherwise it's fully visible
            //
            if ((screenLos[x][y] & __VCH) == __VCH) {
                screenLos[x][y] = 0;
            }
            else {
                screenLos[x][y] = 1;
            }
        }
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
    unsigned int scale = xu4.settings->scale;
    xu4.screenImage->fillRect(BORDER_WIDTH * scale, BORDER_HEIGHT * scale,
                              VIEWPORT_W * TILE_WIDTH * scale,
                              VIEWPORT_H * TILE_HEIGHT * scale,
                              0, 0, 0);
}

void screenEraseTextArea(int x, int y, int width, int height) {
    unsigned int scale = xu4.settings->scale;
    xu4.screenImage->fillRect(x * CHAR_WIDTH * scale, y * CHAR_HEIGHT * scale,
                              width * CHAR_WIDTH * scale,
                              height * CHAR_HEIGHT * scale,
                              0, 0, 0);
}

/**
 * Do the tremor spell effect where the screen shakes.
 */
void screenShake(int iterations) {
    if (xu4.settings->screenShakes) {
        int shakeOffset = SCALED(1);

        for (int i = 0; i < iterations; i++) {
            // shift the screen down and make the top row black
            screenVertOffset = shakeOffset;
            EventHandler::sleep(xu4.settings->shakeInterval);

            // shift the screen back up
            screenVertOffset = 0;
            EventHandler::sleep(xu4.settings->shakeInterval);
        }
    }
}

/**
 * Draw a tile graphic on the screen.
 */
static void screenShowGemTile(const Layout *layout, Map *map, MapTile &t, bool focus, int x, int y) {
    unsigned int scale = xu4.settings->scale;
    unsigned int tile = xu4.config->usaveIds()->ultimaId(t);

    if (map->type == Map::DUNGEON) {
        ASSERT(charsetInfo, "charset not initialized");
        std::map<string, int>::iterator charIndex = dungeonTileChars.find(t.getTileType()->nameStr());
        if (charIndex != dungeonTileChars.end()) {
            charsetInfo->image->drawSubRect((layout->viewport.x + (x * layout->tileshape.width)) * scale,
                                            (layout->viewport.y + (y * layout->tileshape.height)) * scale,
                                            0,
                                            charIndex->second * layout->tileshape.height * scale,
                                            layout->tileshape.width * scale,
                                            layout->tileshape.height * scale);
        }
    }
    else {
        if (gemTilesInfo == NULL) {
            gemTilesInfo = xu4.imageMgr->get(BKGD_GEMTILES);
            if (!gemTilesInfo)
                errorFatal("ERROR 1002: Unable to load the \"%s\" data file.\t\n\nIs %s installed?\n\nVisit the XU4 website for additional information.\n\thttp://xu4.sourceforge.net/", BKGD_GEMTILES, xu4.settings->game.c_str());
        }

        if (tile < 128) {
            gemTilesInfo->image->drawSubRect((layout->viewport.x + (x * layout->tileshape.width)) * scale,
                                             (layout->viewport.y + (y * layout->tileshape.height)) * scale,
                                             0,
                                             tile * layout->tileshape.height * scale,
                                             layout->tileshape.width * scale,
                                             layout->tileshape.height * scale);
        } else {
            xu4.screenImage->fillRect((layout->viewport.x + (x * layout->tileshape.width)) * scale,
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
        layout = dungeonGemLayout;
    else
        layout = gemLayout;


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
                    tile = c->location->map->getTileFromData(c->location->coords)->getId();
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

    while (filter && filterScaler && (scale % 2 == 0)) {
        dest = (*filterScaler)(src, 2, n);
        src = dest;
        scale /= 2;
    }
    if (scale == 3 && scaler3x(xu4.settings->filter)) {
        dest = (*filterScaler)(src, 3, n);
        src = dest;
        scale /= 3;
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

#ifdef IOS
//Unsure if implementation required in iOS.
void inline screenWait(int numberOfAnimationFrames){};
#endif
