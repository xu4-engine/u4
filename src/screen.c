/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "u4.h"

#include "screen.h"

#include "context.h"
#include "dngview.h"
#include "list.h"
#include "location.h"
#include "names.h"
#include "object.h"
#include "savegame.h"
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

unsigned char screenViewportTile(unsigned int width, unsigned int height, int x, int y, int *focus) {
    int centerx, centery, tx, ty, z;

    z = c->location->z;
    centerx = c->location->x;
    centery = c->location->y;
    if (c->location->map->width == width &&
        c->location->map->height == height) {
        centerx = width / 2;
        centery = height / 2;
    }

    tx = x + centerx - (width / 2);
    ty = y + centery - (height / 2);

    /* off the edge of the map: wrap or pad with grass tiles */
    if (MAP_IS_OOB(c->location->map, tx, ty)) {
        if (c->location->map->border_behavior == BORDER_WRAP) {
            if (tx < 0)
                tx += c->location->map->width;
            if (ty < 0)
                ty += c->location->map->height;
            if (tx >= (int)c->location->map->width)
                tx -= c->location->map->width;
            if (ty >= (int)c->location->map->height)
                ty -= c->location->map->height;
        }
        else {
            *focus = 0;
            return GRASS_TILE;
        }
    }

    return locationVisibleTileAt(c->location, tx, ty, z, focus);
}

/**
 * Redraw the screen.  If showmap is set, the normal map is drawn in
 * the map area.  If blackout is set, the map area is blacked out. If
 * neither is set, the map area is left untouched.
 */
void screenUpdate(int showmap, int blackout) {
    ListNode *tiles;
    int focus, x, y;
    Tileset *base = tilesetGetByType(TILESET_BASE);

    assert(c != NULL);

    if (c->location->map->flags & FIRST_PERSON) {
        
        /* 1st-person perspective */
        if (screen3dDungeonView) {
            screenEraseMapArea();
            if (c->saveGame->torchduration > 0 && !blackout) {
                for (y = 3; y >= 0; y--) {
                    DungeonGraphicType type;

                    tiles = dungeonViewGetTiles(y, -1);
                    screenDungeonDrawWall(-1, y, c->saveGame->orientation, dungeonViewTilesToGraphic(tiles));
                    listDelete(tiles);

                    tiles = dungeonViewGetTiles(y, 1);
                    screenDungeonDrawWall(1, y, c->saveGame->orientation, dungeonViewTilesToGraphic(tiles));
                    listDelete(tiles);

                    tiles = dungeonViewGetTiles(y, 0);
                    type = dungeonViewTilesToGraphic(tiles);
                    if (type == DNGGRAPHIC_DNGTILE)
                        screenDungeonDrawTile(y, c->location->tileset->tiles[(unsigned char) (unsigned) tiles->data].displayTile);
                    else if (type == DNGGRAPHIC_BASETILE)
                        screenDungeonDrawTile(y, base->tiles[(unsigned char) (unsigned) tiles->data].displayTile);
                    else
                        screenDungeonDrawWall(0, y, c->saveGame->orientation, dungeonViewTilesToGraphic(tiles));
                    listDelete(tiles);
                }
            }
        }

        /* 3rd-person perspective */
        else {
            for (y = 0; y < VIEWPORT_H; y++) {
                for (x = 0; x < VIEWPORT_W; x++) {
                    if (x < 2 || y < 2 || x >= 10 || y >= 10)
                        screenShowTile(base, BLACK_TILE, 0, x, y);
                    else {
                        tiles = dungeonViewGetTiles((VIEWPORT_H / 2) - y, x - (VIEWPORT_W / 2));

                        /* Only show blackness if there is no light */
                        if (c->saveGame->torchduration <= 0)
                            screenShowTile(base, BLACK_TILE, 0, x, y);
                        else if (x == VIEWPORT_W/2 && y == VIEWPORT_H/2)
                            screenShowTile(base, AVATAR_TILE, 0, x, y);
                        else
                            screenShowTile(base, c->location->tileset->tiles[(unsigned char) (unsigned) tiles->data].displayTile, 0, x, y);
                        listDelete(tiles);
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
                unsigned char tile;

                if (!blackout && screenLos[x][y]) {
                    tile = screenViewportTile(VIEWPORT_W, VIEWPORT_H, x, y, &focus);
                    screenShowTile(base, tile, focus, x, y);
                } else
                    screenShowTile(base, BLACK_TILE, 0, x, y);
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

    assert(phase >= 0 && phase < 4);

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
        screenShowChar('1'+c->location->z, 12, 0);        
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
        screenTextAt(WIND_AREA_X, WIND_AREA_Y, "Dir: %5s", getDirectionName(c->saveGame->orientation));        
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
            !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x + 1, VIEWPORT_H / 2, &focus)))
            screenLos[x][VIEWPORT_H / 2] = 1;

    for (x = VIEWPORT_W / 2 + 1; x < VIEWPORT_W; x++)
        if (screenLos[x - 1][VIEWPORT_H / 2] &&
            !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x - 1, VIEWPORT_H / 2, &focus)))
            screenLos[x][VIEWPORT_H / 2] = 1;

    for (y = VIEWPORT_H / 2 - 1; y >= 0; y--)
        if (screenLos[VIEWPORT_W / 2][y + 1] &&
            !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, VIEWPORT_W / 2, y + 1, &focus)))
            screenLos[VIEWPORT_W / 2][y] = 1;

    for (y = VIEWPORT_H / 2 + 1; y < VIEWPORT_H; y++)
        if (screenLos[VIEWPORT_W / 2][y - 1] &&
            !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, VIEWPORT_W / 2, y - 1, &focus)))
            screenLos[VIEWPORT_W / 2][y] = 1;

    for (y = VIEWPORT_H / 2 - 1; y >= 0; y--) {
        
        for (x = VIEWPORT_W / 2 - 1; x >= 0; x--) {
            if (screenLos[x][y + 1] &&
                !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x, y + 1, &focus)))
                screenLos[x][y] = 1;
            else if (screenLos[x + 1][y] &&
                !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x + 1, y, &focus)))
                screenLos[x][y] = 1;
            else if (screenLos[x + 1][y + 1] &&
                !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x + 1, y + 1, &focus)))
                screenLos[x][y] = 1;
        }
                
        for (x = VIEWPORT_W / 2 + 1; x < VIEWPORT_W; x++) {
            if (screenLos[x][y + 1] &&
                !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x, y + 1, &focus)))
                screenLos[x][y] = 1;
            else if (screenLos[x - 1][y] &&
                !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x - 1, y, &focus)))
                screenLos[x][y] = 1;
            else if (screenLos[x - 1][y + 1] &&
                !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x - 1, y + 1, &focus)))
                screenLos[x][y] = 1;
        }
    }

    for (y = VIEWPORT_H / 2 + 1; y < VIEWPORT_H; y++) {
        
        for (x = VIEWPORT_W / 2 - 1; x >= 0; x--) {
            if (screenLos[x][y - 1] &&
                !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x, y - 1, &focus)))
                screenLos[x][y] = 1;
            else if (screenLos[x + 1][y] &&
                !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x + 1, y, &focus)))
                screenLos[x][y] = 1;
            else if (screenLos[x + 1][y - 1] &&
                !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x + 1, y - 1, &focus)))
                screenLos[x][y] = 1;
        }
                
        for (x = VIEWPORT_W / 2 + 1; x < VIEWPORT_W; x++) {
            if (screenLos[x][y - 1] &&
                !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x, y - 1, &focus)))
                screenLos[x][y] = 1;
            else if (screenLos[x - 1][y] &&
                !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x - 1, y, &focus)))
                screenLos[x][y] = 1;
            else if (screenLos[x - 1][y - 1] &&
                !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x - 1, y - 1, &focus)))
                screenLos[x][y] = 1;
        }
    }
}

