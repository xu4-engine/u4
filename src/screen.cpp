/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <cstdio>
#include <cstdarg>
#include <cfloat>
#include "u4.h"

#include "screen.h"

#include "context.h"
#include "debug.h"
#include "dngview.h"
#include "location.h"
#include "names.h"
#include "object.h"
#include "savegame.h"
#include "settings.h"
#include "tileset.h"

int screenNeedPrompt = 1;
int screenCurrentCycle = 0;
int screenCursorX = 0;
int screenCursorY = 0;
int screenCursorStatus = 0;
int screenCursorEnabled = 1;
int screenLos[VIEWPORT_W][VIEWPORT_H];
int screen3dDungeonView = 1;

void screenTextAt(int x, int y, const char *fmt, ...) {
    char buffer[1024];
    unsigned int i;

    va_list args;
    va_start(args, fmt);
    vsprintf(buffer, fmt, args);
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
    char buffer[1024];
    unsigned int i;
    int wordlen;

    va_list args;
    va_start(args, fmt);
    vsprintf(buffer, fmt, args);
    va_end(args);

    screenHideCursor();

    /* scroll the message area, if necessary */
    if (c->line == 12) {
        screenScrollMessageArea();
        c->line--;
    }

    for (i = 0; i < strlen(buffer); i++) {
        wordlen = strcspn(buffer + i, " \b\t\n");

        /* backspace */
        if (buffer[i] == '\b') {
            c->col--;
            if (c->col < 0) {
                c->col += 16;
                c->line--;
            }
            continue;
        }

        /* check for word wrap */
        if ((c->col + wordlen > 16) || buffer[i] == '\n' || c->col == 16) {
            if (buffer[i] == '\n' || buffer[i] == ' ')
                i++;
            c->line++;
            c->col = 0;
            screenMessage(buffer + i);
            return;
        }

        /* code for move cursor right */
        if (buffer[i] == 0x12) {
            c->col++;
            continue;
        }
        
        screenShowChar(buffer[i], TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line);
        c->col++;
    }

    screenSetCursorPos(TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line);
    screenShowCursor();

    screenNeedPrompt = 1;
}

MapTile* screenViewportTile(unsigned int width, unsigned int height, int x, int y, int *focus) {
    MapCoords center = c->location->coords;    
    static MapTile grass = Tileset::findTileByName("grass")->id;
    
    if (c->location->map->width <= width &&
        c->location->map->height <= height) {
        center.x = c->location->map->width / 2;
        center.y = c->location->map->height / 2;
    }

    MapCoords tc = center;

    tc.x += x - (width / 2);
    tc.y += y - (height / 2);

    /* off the edge of the map: wrap or pad with grass tiles */
    if (MAP_IS_OOB(c->location->map, tc)) {
        if (c->location->map->border_behavior == BORDER_WRAP) 
            tc.wrap(c->location->map);
        else {
            *focus = 0;
            return &grass;
        }
    }

    return locationVisibleTileAt(c->location, tc, focus);
}

/**
 * Redraw the screen.  If showmap is set, the normal map is drawn in
 * the map area.  If blackout is set, the map area is blacked out. If
 * neither is set, the map area is left untouched.
 */
void screenUpdate(int showmap, int blackout) {
    MapTileList tiles;
    int focus, x, y;    

    static MapTile black = Tileset::findTileByName("black")->id;
    static MapTile avatar = Tileset::findTileByName("avatar")->id;

    ASSERT(c != NULL, "context has not yet been initialized");

    if (c->location->map->flags & FIRST_PERSON) {
        
        /* 1st-person perspective */
        if (screen3dDungeonView) {
            screenEraseMapArea();
            if (c->saveGame->torchduration > 0 && !blackout) {
                for (y = 3; y >= 0; y--) {
                    DungeonGraphicType type;

                    tiles = dungeonViewGetTiles(y, -1);
                    screenDungeonDrawWall(-1, y, (Direction)c->saveGame->orientation, dungeonViewTilesToGraphic(tiles));
                    delete tiles;

                    tiles = dungeonViewGetTiles(y, 1);
                    screenDungeonDrawWall(1, y, (Direction)c->saveGame->orientation, dungeonViewTilesToGraphic(tiles));
                    delete tiles;

                    tiles = dungeonViewGetTiles(y, 0);
                    type = dungeonViewTilesToGraphic(tiles);
                    if ((type == DNGGRAPHIC_DNGTILE) || (type == DNGGRAPHIC_BASETILE))
                        screenDungeonDrawTile(y, tiles->front());
                    else
                        screenDungeonDrawWall(0, y, (Direction)c->saveGame->orientation, dungeonViewTilesToGraphic(tiles));
                    delete tiles;
                }
            }
        }

        /* 3rd-person perspective */
        else {
            for (y = 0; y < VIEWPORT_H; y++) {
                for (x = 0; x < VIEWPORT_W; x++) {
                    if (x < 2 || y < 2 || x >= 10 || y >= 10)
                        screenShowTile(&black, 0, x, y);
                    else {
                        tiles = dungeonViewGetTiles((VIEWPORT_H / 2) - y, x - (VIEWPORT_W / 2));

                        /* Only show blackness if there is no light */
                        if (c->saveGame->torchduration <= 0)
                            screenShowTile(&black, 0, x, y);
                        else if (x == VIEWPORT_W/2 && y == VIEWPORT_H/2)
                            screenShowTile(&avatar, 0, x, y);
                        else
                            screenShowTile(tiles->front(), 0, x, y);                        
                        delete tiles;
                    }
                }
            }
        }
        screenRedrawMapArea();
    }

    else if (showmap) {
        if (!blackout)
            screenFindLineOfSight();

        for (y = 0; y < VIEWPORT_H; y++) {
            for (x = 0; x < VIEWPORT_W; x++) {
                MapTile *tile;

                if (!blackout && screenLos[x][y]) {
                    tile = screenViewportTile(VIEWPORT_W, VIEWPORT_H, x, y, &focus);
                    screenShowTile(tile, focus, x, y);
                } else
                    screenShowTile(&black, 0, x, y);
            }
        }
        screenRedrawMapArea();
    }

    screenUpdateCursor();
    screenUpdateMoons();
    screenUpdateWind();
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
        screenRedrawTextArea(screenCursorX, screenCursorY, 1, 1);
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

    screenRedrawTextArea(11, 0, 2, 1);
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
    screenRedrawTextArea(WIND_AREA_X, WIND_AREA_Y, WIND_AREA_W, WIND_AREA_H);
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
        screenRedrawTextArea(screenCursorX, screenCursorY, 1, 1);
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

void screenFindLineOfSight() {
    int focus, x, y;

    if (c == NULL)
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

    screenLos[VIEWPORT_W / 2][VIEWPORT_H / 2] = 1;

    for (x = VIEWPORT_W / 2 - 1; x >= 0; x--)
        if (screenLos[x + 1][VIEWPORT_H / 2] &&
            !screenViewportTile(VIEWPORT_W, VIEWPORT_H, x + 1, VIEWPORT_H / 2, &focus)->isOpaque())
            screenLos[x][VIEWPORT_H / 2] = 1;

    for (x = VIEWPORT_W / 2 + 1; x < VIEWPORT_W; x++)
        if (screenLos[x - 1][VIEWPORT_H / 2] &&
            !screenViewportTile(VIEWPORT_W, VIEWPORT_H, x - 1, VIEWPORT_H / 2, &focus)->isOpaque())
            screenLos[x][VIEWPORT_H / 2] = 1;

    for (y = VIEWPORT_H / 2 - 1; y >= 0; y--)
        if (screenLos[VIEWPORT_W / 2][y + 1] &&
            !screenViewportTile(VIEWPORT_W, VIEWPORT_H, VIEWPORT_W / 2, y + 1, &focus)->isOpaque())
            screenLos[VIEWPORT_W / 2][y] = 1;

    for (y = VIEWPORT_H / 2 + 1; y < VIEWPORT_H; y++)
        if (screenLos[VIEWPORT_W / 2][y - 1] &&
            !screenViewportTile(VIEWPORT_W, VIEWPORT_H, VIEWPORT_W / 2, y - 1, &focus)->isOpaque())
            screenLos[VIEWPORT_W / 2][y] = 1;

    for (y = VIEWPORT_H / 2 - 1; y >= 0; y--) {
        
        for (x = VIEWPORT_W / 2 - 1; x >= 0; x--) {
            if (screenLos[x][y + 1] &&
                !screenViewportTile(VIEWPORT_W, VIEWPORT_H, x, y + 1, &focus)->isOpaque())
                screenLos[x][y] = 1;
            else if (screenLos[x + 1][y] &&
                !screenViewportTile(VIEWPORT_W, VIEWPORT_H, x + 1, y, &focus)->isOpaque())
                screenLos[x][y] = 1;
            else if (screenLos[x + 1][y + 1] &&
                !screenViewportTile(VIEWPORT_W, VIEWPORT_H, x + 1, y + 1, &focus)->isOpaque())
                screenLos[x][y] = 1;
        }
                
        for (x = VIEWPORT_W / 2 + 1; x < VIEWPORT_W; x++) {
            if (screenLos[x][y + 1] &&
                !screenViewportTile(VIEWPORT_W, VIEWPORT_H, x, y + 1, &focus)->isOpaque())
                screenLos[x][y] = 1;
            else if (screenLos[x - 1][y] &&
                !screenViewportTile(VIEWPORT_W, VIEWPORT_H, x - 1, y, &focus)->isOpaque())
                screenLos[x][y] = 1;
            else if (screenLos[x - 1][y + 1] &&
                !screenViewportTile(VIEWPORT_W, VIEWPORT_H, x - 1, y + 1, &focus)->isOpaque())
                screenLos[x][y] = 1;
        }
    }

    for (y = VIEWPORT_H / 2 + 1; y < VIEWPORT_H; y++) {
        
        for (x = VIEWPORT_W / 2 - 1; x >= 0; x--) {
            if (screenLos[x][y - 1] &&
                !screenViewportTile(VIEWPORT_W, VIEWPORT_H, x, y - 1, &focus)->isOpaque())
                screenLos[x][y] = 1;
            else if (screenLos[x + 1][y] &&
                !screenViewportTile(VIEWPORT_W, VIEWPORT_H, x + 1, y, &focus)->isOpaque())
                screenLos[x][y] = 1;
            else if (screenLos[x + 1][y - 1] &&
                !screenViewportTile(VIEWPORT_W, VIEWPORT_H, x + 1, y - 1, &focus)->isOpaque())
                screenLos[x][y] = 1;
        }
                
        for (x = VIEWPORT_W / 2 + 1; x < VIEWPORT_W; x++) {
            if (screenLos[x][y - 1] &&
                !screenViewportTile(VIEWPORT_W, VIEWPORT_H, x, y - 1, &focus)->isOpaque())
                screenLos[x][y] = 1;
            else if (screenLos[x - 1][y] &&
                !screenViewportTile(VIEWPORT_W, VIEWPORT_H, x - 1, y, &focus)->isOpaque())
                screenLos[x][y] = 1;
            else if (screenLos[x - 1][y - 1] &&
                !screenViewportTile(VIEWPORT_W, VIEWPORT_H, x - 1, y - 1, &focus)->isOpaque())
                screenLos[x][y] = 1;
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

    /* two points define a rectangle */
    if (area->npoints == 2) {
        if (x >= (int)(area->point[0].x * settings.scale) &&
            y >= (int)(area->point[0].y * settings.scale) &&
            x < (int)(area->point[1].x * settings.scale) &&
            y < (int)(area->point[1].y * settings.scale)) {
            return 1;
        }
    }

    /* three points define a triangle */
    else if (area->npoints == 3) {
        return screenPointInTriangle(x, y, 
                                     area->point[0].x * settings.scale, area->point[0].y * settings.scale,
                                     area->point[1].x * settings.scale, area->point[1].y * settings.scale,
                                     area->point[2].x * settings.scale, area->point[2].y * settings.scale);
    }

    return 0;
}
