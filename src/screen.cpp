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

#include "context.h"
#include "debug.h"
#include "dungeonview.h"
#include "error.h"
#include "event.h"
#include "game.h"
#include "imagemgr.h"
#include "settings.h"
#include "stats.h"
#include "textview.h"
#include "tileanim.h"
#include "tileset.h"
#include "xu4.h"
#include "gpu.h"

using std::vector;

static const int MsgBufferSize = 1024;

struct RenderLayer {
    void (*func)(ScreenState*, void*);
    void* data;
};

static const float colorBlack[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

static const char* fontFiles[] = {
    "cfont-comfortaa.txf",
    "cfont-avatar.txf",
    "cfont-symbols.txf"
};

/*
 * Load textured font metrics.
 *
 * Return zero if any files failed to load.
 */
static int loadFonts(const char** files, int txfCount, TxfHeader** txfArr)
{
    int i;
    for (i = 0; i < txfCount; ++i) {
        txfArr[i] = (TxfHeader*) xu4.config->loadFile(*files++);
        if (! txfArr[i]) {
            int fn;
            for (fn = 0; fn < i; ++fn) {
                free(txfArr[fn]);
                txfArr[fn] = NULL;
            }
            return 0;
        }
    }
    return txfCount;
}

struct Screen {
    RenderLayer* layers;
    vector<string> gemLayoutNames;
    const Layout* gemLayout;
    const Layout* dungeonGemLayout;
    DungeonView* dungeonView;
    std::map<string, int> dungeonTileChars;
    ImageInfo* charsetInfo;
    ImageInfo* gemTilesInfo;
    char* msgBuffer;
    ScreenState state;
    TxfHeader* txf[3];
    short needPrompt;
    short colorFG;
    uint8_t uploadScreen;
    uint8_t layersAvail;
#ifdef GPU_RENDER
    ImageInfo* textureInfo;
    TileView* renderMapView;
    VisualId focusReticle;
    int mapId;          // Tracks map changes.
    int blockX;         // Tracks changes to view point.
    int blockY;
    BlockingGroups* blockingUpdate;
    BlockingGroups blockingGroups;
#else
    uint8_t blockingGrid[VIEWPORT_W * VIEWPORT_H];
    uint8_t screenLos[VIEWPORT_W * VIEWPORT_H];
#endif

    Screen(uint8_t layerCount) {
        layers = new RenderLayer[layerCount];
        memset(layers, 0, sizeof(RenderLayer) * layerCount);
        uploadScreen = 0;
        layersAvail = layerCount;

        gemLayout = NULL;
        dungeonGemLayout = NULL;
        dungeonView = NULL;
        charsetInfo = NULL;
        gemTilesInfo = NULL;
        msgBuffer = new char[MsgBufferSize];

        state.tileanims = NULL;
        state.fontTable = txf;
        state.currentCycle = 0;
        state.vertOffset = 0;
        state.displayW = state.displayH = 0;
        state.aspectW = state.aspectH = 0;
        state.cursorX = state.cursorY = 0;
        state.cursorVisible = false;

        needPrompt = 1;
        colorFG = FONT_COLOR_INDEX(FG_WHITE);
#ifdef GPU_RENDER
        textureInfo = NULL;
        renderMapView = NULL;
#endif
        txf[0] = NULL;
        loadFonts(fontFiles, 3, txf);
    }

    ~Screen() {
        if (txf[0]) {
            free(txf[0]);
            free(txf[1]);
            free(txf[2]);
        }
        delete dungeonView;
        delete[] msgBuffer;
        delete[] layers;
    }
};

#define XU4_SCREEN  ((Screen*) xu4.screen)

static void screenLoadLayoutsFromConf(Screen*);
#ifndef GPU_RENDER
static void screenFindLineOfSight();
#endif

// Just extern the system functions here. That way people aren't tempted to call them as part of the public API.
extern void screenInit_sys(const Settings*, ScreenState*, int reset);
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
#ifdef GPU_RENDER
    scr->mapId = -1;
    scr->blockX = scr->blockY = -1;
    scr->blockingUpdate = NULL;
#endif

    // Create a special purpose image that represents the whole screen.
    xu4.screenImage = Image::create(320, 200);
    xu4.screenImage->fill(Image::black);

    xu4.imageMgr = new ImageMgr;

    scr->charsetInfo = xu4.imageMgr->get(BKGD_CHARSET);
    if (! scr->charsetInfo)
        errorLoadImage(BKGD_CHARSET);

#ifdef GPU_RENDER
    {
    ImageInfo* tinfo;
    ImageInfo* minfo;
    Symbol symbol[3];
    uint32_t matId = 0;

    xu4.config->internSymbols(symbol, 3, "texture material reticle");
    scr->textureInfo = tinfo = xu4.imageMgr->get(symbol[0]);
    if (tinfo) {
        minfo = xu4.imageMgr->get(symbol[1]);
        if (minfo) {
            if (! minfo->tex)
                minfo->tex = gpu_makeTexture(minfo->image);
            matId = minfo->tex;
        }

        gpu_setTilesTexture(xu4.gpu, tinfo->tex, matId, tinfo->tileTexCoord[3]);
        scr->focusReticle = tinfo->subImageIndex.find(symbol[2])->second;
    }
    }
#endif

    assert(scr->state.tileanims == NULL);
    scr->state.tileanims = xu4.config->newTileAnims();
    if (! scr->state.tileanims)
        errorFatal("Unable to find tile animations");

    scr->gemTilesInfo = NULL;
    screenLoadLayoutsFromConf(scr);

    if (xu4.verbose)
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
void screenInit(int layerCount) {
    Screen* screen = new Screen(layerCount);
    xu4.screen = screen;
    screenInit_sys(xu4.settings, &screen->state, SYS_CLEAN);
    screenInit_data(screen, *xu4.settings);
}

void screenDelete() {
    if (xu4.screen) {
        screenDelete_data(XU4_SCREEN);
        screenDelete_sys();
        delete XU4_SCREEN;
        xu4.screen = NULL;
    }
}

/**
 * Re-initializes the screen and implements any changes made in settings
 */
void screenReInit() {
    Screen* screen = XU4_SCREEN;

    gs_emitMessage(SENDER_DISPLAY, NULL);

    screenDelete_data(XU4_SCREEN);
    screenInit_sys(xu4.settings, &screen->state, SYS_RESET);
    gpu_clear(xu4.gpu, colorBlack);
    screenInit_data(screen, *xu4.settings); // Load new backgrounds, etc.

    gs_emitMessage(SENDER_DISPLAY, &screen->state);
}

void screenSetLayer(int layer, void (*renderFunc)(ScreenState*, void*),
                    void* data) {
    Screen* screen = XU4_SCREEN;
    assert(layer < screen->layersAvail);

    RenderLayer* rl = screen->layers + layer;
    rl->func = renderFunc;
    rl->data = data;
}

bool screenLayerUsed(int layer) {
    return XU4_SCREEN->layers[layer].func ? true : false;
}

void screenTextAt(int x, int y, const char *fmt, ...) {
    char* buffer = XU4_SCREEN->msgBuffer;
    int i, buflen;

    va_list args;
    va_start(args, fmt);
    buflen = vsnprintf(buffer, MsgBufferSize, fmt, args);
    va_end(args);

    for (i = 0; i < buflen; i++)
        screenShowChar(buffer[i], x + i, y);
}

void screenPrompt() {
    Screen* scr = XU4_SCREEN;
    if (scr->needPrompt && scr->state.cursorVisible && c->col == 0) {
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
        screenScrollMessageArea();
    }
    screenSetCursorPos(TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line);
}

// whitespace & color codes: " \b\t\n\r\023\024\025\026\027\030\031"
static const uint8_t nonWordChars[32] = {
    0x01, 0x27, 0xF8, 0x03, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void screenMessage(const char *fmt, ...) {
    char* buffer = XU4_SCREEN->msgBuffer;
    int buflen;

    if (! c)
        return; // Some cases (like the intro) don't have the context initiated.

    va_list args;
    va_start(args, fmt);
    buflen = vsnprintf(buffer, MsgBufferSize, fmt, args);
    va_end(args);
    if (buflen < 1)
        return;

    screenMessageN(buffer, buflen);
}

/*
 * Center first line of text in message area.
 */
void screenMessageCenter(const char* text, int newlines) {
    const int half = TEXT_AREA_W / 2;
    char* buffer = XU4_SCREEN->msgBuffer;
    char* start = buffer + half;
    char* cp = start;
    char* endOfLine = NULL;
    int ch, lineLen;

    memset(buffer, ' ', half);
    while ((ch = *text++)) {
        if (ch == '\n' && ! endOfLine)
            endOfLine = cp;
        *cp++ = ch;
    }

    if (! endOfLine)
        endOfLine = cp;
    lineLen = endOfLine - start;
    if (lineLen > TEXT_AREA_W)
        lineLen = TEXT_AREA_W;
    start -= half - (lineLen+1) / 2;    // Indent

    while (newlines) {
        --newlines;
        *cp++ = '\n';
    }

    screenMessageN(start, cp - start);
}

void screenMessageN(const char* buffer, int buflen) {
    bool colorize = xu4.settings->enhancements &&
                    xu4.settings->enhancementsOptions.textColorization;
    const int colCount = TEXT_AREA_W;
    int i, w;

    /* scroll the message area, if necessary */
    if (c->line == TEXT_AREA_H) {
        c->line--;
        screenScrollMessageArea();
    }

    for (i = 0; i < buflen; i++) {
        switch (buffer[i])
        {
            case '\b':          // backspace
                c->col--;
                if (c->col < 0) {
                    c->col += colCount;
                    c->line--;
                }
                continue;

            case '\n':          // new line
newline:
                c->col = 0;
                c->line++;
                if (c->line == TEXT_AREA_H) {
                    c->line--;
                    screenScrollMessageArea();
                }
                continue;

            case '\r':          // carriage return
                c->col = 0;
                continue;

            case 0x12:          // DC2 - code for move cursor right
                c->col++;
                continue;

            case FG_GREY:       // color-change codes
            case FG_BLUE:
            case FG_PURPLE:
            case FG_GREEN:
            case FG_RED:
            case FG_YELLOW:
            case FG_WHITE:
                if (colorize)
                    XU4_SCREEN->colorFG = FONT_COLOR_INDEX(buffer[i]);
                continue;

            case '\t':          // tab
            case ' ':
                if (c->col == colCount)
                    goto newline;

                /* don't show a space in column 1.  Helps with Hawkwind, but
                 * disables centering of endgame message. */
                if (c->col == 0 && c->hawkwindHack)
                    continue;

                screenShowChar(' ', TEXT_AREA_X+c->col, TEXT_AREA_Y+c->line);
                c->col++;
                continue;
        }

        if (c->col == colCount) {
            --i;            // Undo loop increment.
            goto newline;
        }

        // Check for word wrap.
        for (w = i; w < buflen; ++w) {
            unsigned int c = ((uint8_t*) buffer)[w];
            if (nonWordChars[c >> 3] & (1 << (c & 7)))
                break;
        }
        if (w == i)
            continue;
        if (c->col + (w - i) > colCount) {
            if (c->col > 0) {
                --i;            // Undo loop increment.
                goto newline;
            }
            // Word doesn't fit on one line so break it up.
            w = i + colCount;
        }

        for (; i < w; ++i) {
            screenShowChar(buffer[i], TEXT_AREA_X+c->col, TEXT_AREA_Y+c->line);
            c->col++;
        }
        --i;
    }

    screenSetCursorPos(TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line);

    XU4_SCREEN->needPrompt = 1;
}

const vector<string>& screenGetGemLayoutNames() {
    return XU4_SCREEN->gemLayoutNames;
}

const char** screenGetFilterNames() {
    static const char* filterNames[] = {
        "point", "HQX", "xBR-lv2", "xBRZ", NULL
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

    vector<MapTile> tiles;
    c->location->getTilesAt(tiles, tc, focus);
    return tiles;
}

/*
 * Return true if coords is visible and tiles were drawn.
 */
bool screenTileUpdate(TileView *view, const Coords &coords)
{
    Location* loc = c->location;
    if (loc->map->flags & FIRST_PERSON)
        return false;

    // Get the screen coordinates
    int x = coords.x;
    int y = coords.y;

    if (loc->map->width > VIEWPORT_W || loc->map->height > VIEWPORT_H)
    {
        //Center the coordinates to the viewport if you're on centered-view map.
        x = x - loc->coords.x + VIEWPORT_W / 2;
        y = y - loc->coords.y + VIEWPORT_H / 2;
    }

#ifdef GPU_RENDER
    if (x >= 0 && y >= 0 && x < VIEWPORT_W && y < VIEWPORT_H)
        return true;
#else
    // Draw if it is on screen
    if (x >= 0 && y >= 0 && x < VIEWPORT_W && y < VIEWPORT_H &&
        XU4_SCREEN->screenLos[y*VIEWPORT_W + x])
    {
        // Get the tiles
        bool focus;
        Coords mc(coords);
        map_wrap(mc, loc->map);
        vector<MapTile> tiles;
        loc->getTilesAt(tiles, mc, focus);

        view->drawTile(tiles, x, y);
        if (focus)
            view->drawFocus(x, y);
        return true;
    }
#endif
    return false;
}

#ifdef GPU_RENDER
struct SpriteRenderData {
    float* attr;
    const float* uvTable;
    float rect[4];
    int cx, cy;
};

static void emitSprite(const Coords* loc, VisualId vid, void* user) {
    SpriteRenderData* rd = (SpriteRenderData*) user;
    float* rect = rd->rect;
    int uvIndex = VID_INDEX(vid);

    // Make a quad with the view center as the origin.
    rect[0] = (float) (loc->x - rd->cx) - 0.5f;
    rect[1] = (float) (rd->cy - loc->y) - 0.5f;
    rd->attr = gpu_emitQuad(rd->attr, rect, rd->uvTable + uvIndex*4);
#if 0
    printf("KR emitSprite %d,%d vid:%d:%d\n",
            loc->x, loc->y, VID_BANK(vid), VID_INDEX(vid));
#endif
}

void screenDisableMap() {
    XU4_SCREEN->renderMapView = NULL;
}

/*
 * \param center    Center of view.
 */
void screenUpdateMap(TileView* view, const Map* map, const Coords& center) {
    Screen* sp = XU4_SCREEN;

    sp->renderMapView = view;

    // Reset map rendering data when the location changes.
    if (sp->mapId != map->id) {
        sp->mapId = map->id;
        sp->blockX = -1;
        gpu_resetMap(xu4.gpu, map);
    }

    // Update the map render position & remake the blocking groups if
    // the view has moved.
    if (sp->blockX != center.x || sp->blockY != center.y) {
        sp->blockX = center.x;
        sp->blockY = center.y;

        if ((map->flags & NO_LINE_OF_SIGHT) == 0) {
            BlockingGroups* blocks = &sp->blockingGroups;
            map->queryBlocking(blocks,
                               center.x - view->columns / 2,
                               center.y - view->rows / 2,
                               view->columns, view->rows);
            sp->blockingUpdate = blocks;
            //printf("KR groups %d,%d,%d\n",
            //       blocks->left, blocks->center, blocks->right);
        }
    }

    {
    SpriteRenderData rd;
    const Object* focusObj;
    const float VIEW_TILE_SIZE = 1.0f;

    rd.uvTable = sp->textureInfo->tileTexCoord;
    rd.rect[2] = rd.rect[3] = VIEW_TILE_SIZE;
    rd.cx = center.x;
    rd.cy = center.y;
    rd.attr = gpu_beginTris(xu4.gpu, GPU_DLIST_VIEW_OBJ);

    map->queryVisible(center, view->columns / 2, emitSprite, &rd, &focusObj);

    if (focusObj) {
        if ((screenState()->currentCycle * 4 / SCR_CYCLE_PER_SECOND) % 2) {
            const Coords& floc = focusObj->coords;
            emitSprite(&floc, sp->focusReticle, &rd);
        }
    }

    gpu_endTris(xu4.gpu, GPU_DLIST_VIEW_OBJ, rd.attr);
    }
}
#endif

/**
 * Redraw the map & stats views.
 *
 * If showmap is set, the normal map is drawn in the map area.
 * If blackout is set, the map area is blacked out.
 * If neither is set, the map area is left untouched.
 */
void screenUpdate(TileView *view, bool showmap, bool blackout) {
    ASSERT(c != NULL, "context has not yet been initialized");

    c->stats->redraw();

    if (blackout)
    {
        screenEraseMapArea();
#ifdef GPU_RENDER
        XU4_SCREEN->renderMapView = NULL;
#endif
    }
    else if (c->location->map->flags & FIRST_PERSON) {
        XU4_SCREEN->dungeonView->display(c, view);
        screenRedrawMapArea();
#ifdef GPU_RENDER
        XU4_SCREEN->renderMapView = NULL;
#endif
    }
    else if (showmap) {
#ifdef GPU_RENDER
        screenUpdateMap(view, c->location->map, c->location->coords);
#else
        MapTile black = c->location->map->tileset->getByName(Tile::sym.black)->getId();
        vector<MapTile> viewTiles[VIEWPORT_W][VIEWPORT_H];
        uint8_t* blocked = XU4_SCREEN->blockingGrid;
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

        const uint8_t* lineOfSight = XU4_SCREEN->screenLos;
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
#endif
    }

    screenUpdateMoons();
    screenUpdateWind();
    screenUploadToGPU();
}

/**
 * Transfer the contents of the screenImage to the GPU.
 * This function will be removed after GPU rendering is fully implemented.
 */
void screenUploadToGPU() {
    XU4_SCREEN->uploadScreen = 1;
}

static void screenUploadCursor(Screen* sp, uint32_t stex) {
    int phase =                             // Rotations per second
    //  (sp->state.currentCycle >> 1) & 3;  // 0.5 DOS (normal)
        sp->state.currentCycle & 3;         // 1.0 C64, DOS (combat)

    ASSERT(phase >= 0 && phase < 4, "derived an invalid cursor phase: %d", phase);
    int cursorChar = 31 - phase;
    const Image* charset = sp->charsetInfo->image;

    // NOTE: This code requires the charset image to be one character wide.
    int cdim = charset->width();
    Image32 cimg;
    cimg.pixels = charset->pixels + (cdim * cdim * cursorChar);
    cimg.w = cimg.h = cdim;
    gpu_blitTexture(stex, sp->state.cursorX * cdim, sp->state.cursorY * cdim,
                    &cimg);
}

void screenRender() {
    Screen* sp = XU4_SCREEN;
    void* gpu = xu4.gpu;
    ScreenState* ss = &sp->state;
    int offsetY = ss->aspectY;

    if (sp->uploadScreen) {
        sp->uploadScreen = 0;

        uint32_t stex = gpu_screenTexture(xu4.gpu);
        gpu_blitTexture(stex, 0, 0, xu4.screenImage);

        if (sp->state.cursorVisible)
            screenUploadCursor(sp, stex);
    }

    if (ss->vertOffset) {
        offsetY -= ss->vertOffset;
        gpu_clear(gpu, colorBlack);     // Clear the top rows of pixels.
    }
    gpu_viewport(ss->aspectX, offsetY, ss->aspectW, ss->aspectH);
    gpu_drawTextureScaled(gpu, gpu_screenTexture(gpu));

#ifdef GPU_RENDER
    // TODO: Make this a RenderLayer.
    TileView* view = sp->renderMapView;
    if (view) {
        if (view->scissor)
            gpu_setScissor(view->scissor);

        gpu_drawMap(gpu, view, sp->textureInfo->tileTexCoord,
                    sp->blockingUpdate, sp->blockX, sp->blockY, view->scale);
        sp->blockingUpdate = NULL;

        gpu_drawTris(gpu, GPU_DLIST_VIEW_OBJ);

        anim_advance(&xu4.eventHandler->fxAnim, 1.0f / 24.0f);
        view->updateEffects((float) sp->blockX,
                            (float) sp->blockY,
                            sp->textureInfo->tileTexCoord);
        gpu_drawTris(gpu, GPU_DLIST_VIEW_FX);

        if (view->highlightActive())
            gpu_invertColors(gpu);

        if (view->scissor)
            gpu_setScissor(NULL);

        gpu_viewport(ss->aspectX, offsetY, ss->aspectW, ss->aspectH);
    }
#endif

    {
    RenderLayer* rl  = sp->layers;
    RenderLayer* end = rl + sp->layersAvail;
    for (; rl != end; ++rl) {
        if (rl->func)
            rl->func(ss, rl->data);
    }
    }
}

void screenDrawImageInMapArea(Symbol name) {
    ImageInfo *info;

    info = xu4.imageMgr->get(name);
    if (!info)
        errorLoadImage(name);

    info->image->drawSubRect(BORDER_WIDTH, BORDER_HEIGHT,
                             BORDER_WIDTH, BORDER_HEIGHT,
                             VIEWPORT_W * TILE_WIDTH,
                             VIEWPORT_H * TILE_HEIGHT);
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
            XU4_SCREEN->colorFG = FONT_COLOR_INDEX(color);
            break;
    }
}

/**
 * Draw a character from the charset onto the screen.
 */
void screenShowChar(int chr, int x, int y) {
    Image* charset = XU4_SCREEN->charsetInfo->image;
    int charW = charset->width();
    int charH = CHAR_HEIGHT;
    int colorFG = XU4_SCREEN->colorFG;

    if (colorFG == FONT_COLOR_INDEX(FG_WHITE)) {
        charset->drawSubRect(x * charW, y * charH,
                             0, chr * charH, charW, charH);
    } else {
        charset->drawLetter(x * charW, y * charH,
                            0, chr * charH, charW, charH,
                            (chr < ' ') ? NULL : fontColor + colorFG,
                            fontColor + FONT_COLOR_INDEX(BG_NORMAL));
    }
}

/**
 * Scroll the text in the message area up one position.
 */
static void screenScrollMessageArea() {
    ImageInfo* charset = XU4_SCREEN->charsetInfo;
    Image* screen = xu4.screenImage;
    int charW = charset->image->width();
    int charH = CHAR_HEIGHT;

    screen->drawSubRectOn(screen,
                          TEXT_AREA_X * charW, TEXT_AREA_Y * charH,
                          TEXT_AREA_X * charW, (TEXT_AREA_Y + 1) * charH,
                          TEXT_AREA_W * charW, (TEXT_AREA_H - 1) * charH);

    screen->fillRect(TEXT_AREA_X * charW,
                     TEXT_AREA_Y * charH + (TEXT_AREA_H - 1) * charH,
                     TEXT_AREA_W * charW, charH, 0, 0, 0);
}

void screenCycle() {
    Screen* screen = XU4_SCREEN;
    int cycle = screen->state.currentCycle + 1;
    if (cycle >= SCR_CYCLE_MAX)
        cycle = 0;
    screen->state.currentCycle = cycle;

    xu4.eventHandler->advanceFlourishAnim();
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

/*
void screenDumpCursor() {
    Screen* scr = XU4_SCREEN;
    printf("cursor %d %d,%d\n",
            scr->state.cursorVisible, scr->state.cursorX, scr->state.cursorY);
}
*/

void screenShowCursor(bool on) {
    XU4_SCREEN->state.cursorVisible = on;
}

void screenSetCursorPos(int x, int y) {
    Screen* scr = XU4_SCREEN;
    scr->state.cursorVisible = true;
    scr->state.cursorX = x;
    scr->state.cursorY = y;
}

bool screenToggle3DDungeonView() {
    DungeonView* view = XU4_SCREEN->dungeonView;
    if (view)
        return view->toggle3DDungeonView();
    return false;
}

void screenMakeDungeonView() {
    if (XU4_SCREEN->dungeonView)
        return;
    XU4_SCREEN->dungeonView = new DungeonView(BORDER_WIDTH, BORDER_HEIGHT,
                                              VIEWPORT_W, VIEWPORT_H);
}

void screenDetectDungeonTraps() {
    XU4_SCREEN->dungeonView->detectTraps();
}

#ifndef GPU_RENDER
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
 *   https://fadden.com/tech/fast-los.html
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
    static const int8_t octantSign[8 * 3] = {
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
    const int8_t* osign = octantSign;

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
        memset(XU4_SCREEN->screenLos, 1, VIEWPORT_W * VIEWPORT_H);
    } else {
        // otherwise calculate it from the map data
        memset(XU4_SCREEN->screenLos, 0, VIEWPORT_W * VIEWPORT_H);

        CPU_START()
        if (xu4.settings->lineOfSight == 0)
            screenFindLineOfSightDOS(XU4_SCREEN->blockingGrid,
                                     XU4_SCREEN->screenLos);
        else
            screenFindLineOfSightEnhanced(XU4_SCREEN->blockingGrid,
                                          XU4_SCREEN->screenLos);
        CPU_END()
    }
}
#endif

/**
 * Generates terms a and b for equation "ax + b = y" that defines the
 * line containing the two given points.  Vertical lines are special
 * cased to return DBL_MAX for a and the x coordinate as b since they
 * cannot be represented with the above formula.
 */
static void getLineTerms(int x1, int y1, int x2, int y2, double *a, double *b) {
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
static int pointsOnSameSideOfLine(int x1, int y1, int x2, int y2, double a, double b) {
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

static int pointInTriangle(int x, int y, int tx1, int ty1, int tx2, int ty2, int tx3, int ty3) {
    double a[3], b[3];

    getLineTerms(tx1, ty1, tx2, ty2, &(a[0]), &(b[0]));
    getLineTerms(tx2, ty2, tx3, ty3, &(a[1]), &(b[1]));
    getLineTerms(tx3, ty3, tx1, ty1, &(a[2]), &(b[2]));

    if (! pointsOnSameSideOfLine(x, y, tx3, ty3, a[0], b[0]))
        return 0;
    if (! pointsOnSameSideOfLine(x, y, tx1, ty1, a[1], b[1]))
        return 0;
    if (! pointsOnSameSideOfLine(x, y, tx2, ty2, a[2], b[2]))
        return 0;

    return 1;
}

/*
 * Transform window x,y to MouseArea coordinate system.
 */
void screenPointToMouseArea(int* x, int* y) {
    const Screen* sp = XU4_SCREEN;
    unsigned int scale = sp->state.aspectH / U4_SCREEN_H;
    *x = (*x - sp->state.aspectX) / scale;
    *y = (*y - sp->state.aspectY) / scale;
}

/**
 * Determine if the given point is within a mouse area.
 * The point is in MouseArea coordinates so use screenPointToMouseArea() to
 * map window coordinates.
 */
int pointInMouseArea(int x, int y, const MouseArea *area) {
    ASSERT(area->npoints == 2 || area->npoints == 3, "unsupported number of points in area: %d", area->npoints);

    /* two points define a rectangle */
    if (area->npoints == 2) {
        if (x >= (int)(area->point[0].x) && y >= (int)(area->point[0].y) &&
            x <  (int)(area->point[1].x) && y <  (int)(area->point[1].y)) {
            return 1;
        }
    }

    /* three points define a triangle */
    else if (area->npoints == 3) {
        return pointInTriangle(x, y, area->point[0].x, area->point[0].y,
                                     area->point[1].x, area->point[1].y,
                                     area->point[2].x, area->point[2].y);
    }

    return 0;
}

void screenRedrawMapArea() {
    xu4.game->mapArea.update();
}

void screenEraseMapArea() {
    xu4.screenImage->fillRect(BORDER_WIDTH, BORDER_HEIGHT,
                              VIEWPORT_W * TILE_WIDTH,
                              VIEWPORT_H * TILE_HEIGHT,
                              0, 0, 0);
}

void screenEraseTextArea(int x, int y, int width, int height) {
    int charW = CHAR_WIDTH;
    int charH = CHAR_HEIGHT;
    xu4.screenImage->fillRect(x * charW, y * charH,
                              width * charW, height * charH, 0, 0, 0);
}

/**
 * Do the tremor spell effect where the screen shakes.
 */
void screenShake(int iterations) {
    if (xu4.settings->screenShakes) {
        Screen* scr = XU4_SCREEN;
        int shakeOffset = scr->state.aspectH / U4_SCREEN_H;

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
static void screenShowGemTile(const Layout *layout, const Map *map,
                              MapTile &t, bool focus, int x, int y) {
    Screen* scr = XU4_SCREEN;
    unsigned int tile = xu4.config->usaveIds()->ultimaId(t);

    if (map->type == Map::DUNGEON) {
        ImageInfo* charsetInfo = scr->charsetInfo;

        std::map<string, int>::iterator charIndex = scr->dungeonTileChars.find(t.getTileType()->nameStr());
        if (charIndex != scr->dungeonTileChars.end()) {
            charsetInfo->image->drawSubRect(
                layout->viewport.x + (x * layout->tileshape.width),
                layout->viewport.y + (y * layout->tileshape.height),
                0,
                charIndex->second * layout->tileshape.height,
                layout->tileshape.width,
                layout->tileshape.height);
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
                layout->viewport.x + (x * layout->tileshape.width),
                layout->viewport.y + (y * layout->tileshape.height),
                0,
                tile * layout->tileshape.height,
                layout->tileshape.width,
                layout->tileshape.height);
        } else {
            xu4.screenImage->fillRect(
                layout->viewport.x + (x * layout->tileshape.width),
                layout->viewport.y + (y * layout->tileshape.height),
                layout->tileshape.width,
                layout->tileshape.height,
                0, 0, 0);
        }
    }
}

void screenGemUpdate() {
    MapTile tile;
    int x, y;
    const Layout* layout;
    const Map* map = c->location->map;
    bool focus;

    xu4.screenImage->fillRect(BORDER_WIDTH,
                              BORDER_HEIGHT,
                              VIEWPORT_W * TILE_WIDTH,
                              VIEWPORT_H * TILE_HEIGHT,
                              0, 0, 0);

    if (map->type == Map::DUNGEON) {
        //DO THE SPECIAL DUNGEON MAP TRAVERSAL
        layout = XU4_SCREEN->dungeonGemLayout;

        vector<vector<int> > drawnTiles(layout->viewport.width, vector<int>(layout->viewport.height, 0));
        vector<std::pair<int,int> > coordStack;
        vector<MapTile> tiles;
        const Coords& coords = c->location->coords;

        //Put the avatar's position on the stack
        int center_x = layout->viewport.width / 2 - 1;
        int center_y = layout->viewport.height / 2 - 1;
        int avt_x = coords.x - 1;
        int avt_y = coords.y - 1;

        coordStack.push_back(std::pair<int,int>(center_x, center_y));
        bool weAreDrawingTheAvatarTile = true;

        //And draw each tile on the growing stack until it is empty
        while (coordStack.size() > 0) {
            const std::pair<int,int>& currentXY = coordStack.back();
            x = currentXY.first;
            y = currentXY.second;
            coordStack.pop_back();

            if (x < 0 || x >= layout->viewport.width ||
                y < 0 || y >= layout->viewport.height)
                continue;   //Skip out of range tiles

            if (drawnTiles[x][y])
                continue;   //Skip already considered tiles

            drawnTiles[x][y] = 1;

            // DRAW THE ACTUAL TILE
            tiles = screenViewportTile(layout->viewport.width,
                                       layout->viewport.height,
                                       x - center_x + avt_x,
                                       y - center_y + avt_y, focus);
            tile = tiles.front();

            if (! weAreDrawingTheAvatarTile) {
                // Hack to avoid showing the avatar tile multiple times in
                // repeating dungeon maps
                TileId avatarTileId =
                    map->tileset->getByName(Tile::sym.avatar)->getId();
                if (tile.getId() == avatarTileId)
                    tile = map->getTileFromData(coords);
            }

            screenShowGemTile(layout, map, tile, focus, x, y);

            if (! tile.getTileType()->isOpaque() ||
                tile.getTileType()->isWalkable() || weAreDrawingTheAvatarTile)
            {
                // Continue the search so we can see through all walkable
                // objects, non-opaque objects (like creatures) or the avatar
                // position in those rare circumstances where he is stuck in a
                // wall by adding all relative adjacency combinations to the
                // stack for drawing

                coordStack.push_back(std::pair<int,int>(x + 1,  y - 1));
                coordStack.push_back(std::pair<int,int>(x + 1,  y    ));
                coordStack.push_back(std::pair<int,int>(x + 1,  y + 1));

                coordStack.push_back(std::pair<int,int>(x    ,  y - 1));
                coordStack.push_back(std::pair<int,int>(x    ,  y + 1));

                coordStack.push_back(std::pair<int,int>(x - 1,  y - 1));
                coordStack.push_back(std::pair<int,int>(x - 1,  y    ));
                coordStack.push_back(std::pair<int,int>(x - 1,  y + 1));

                // We only draw the avatar tile once, it is the first tile drawn
                weAreDrawingTheAvatarTile = false;
            }
        }
    } else {
        //DO THE REGULAR EVERYTHING-IS-VISIBLE MAP TRAVERSAL
        layout = XU4_SCREEN->gemLayout;

        for (x = 0; x < layout->viewport.width; x++) {
            for (y = 0; y < layout->viewport.height; y++) {
                tile = screenViewportTile(layout->viewport.width,
                                          layout->viewport.height, x, y, focus).front();
                screenShowGemTile(layout, map, tile, focus, x, y);
            }
        }
    }

    screenRedrawMapArea();

    screenUpdateMoons();
    screenUpdateWind();
    screenUploadToGPU();
}

const ScreenState* screenState() {
    return &XU4_SCREEN->state;
}
