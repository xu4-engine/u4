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
#include "map.h"
#include "object.h"
#include "context.h"
#include "savegame.h"
#include "names.h"
#include "ttype.h"

int screenCurrentCycle = 0;
int screenCursorX = 0;
int screenCursorY = 0;
int screenCursorStatus = 0;
int screenLos[VIEWPORT_W][VIEWPORT_H];

void screenTextAt(int x, int y, char *fmt, ...) {
    char buffer[1024];
    int i;

    va_list args;
    va_start(args, fmt);
    vsprintf(buffer, fmt, args);
    va_end(args);

    for (i = 0; i < strlen(buffer); i++)
	screenShowChar(buffer[i], x + i, y);
}

void screenMessage(const char *fmt, ...) {
    char buffer[1024];
    int i;
    int wordlen;

    va_list args;
    va_start(args, fmt);
    vsprintf(buffer, fmt, args);
    va_end(args);

    screenDisableCursor();

    // scroll the message area, if necessary
    if (c->line == 12) {
        screenScrollMessageArea(); 
        c->line--;
    }

    for (i = 0; i < strlen(buffer); i++) {
        wordlen = strcspn(buffer + i, " \t\n");
        if ((c->col + wordlen > 16) || buffer[i] == '\n' || c->col == 16) {
            if (buffer[i] == '\n' || buffer[i] == ' ')
                i++;
            c->line++;
            c->col = 0;
            screenMessage(buffer + i);
            return;
        }
	screenShowChar(buffer[i], TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line);
        c->col++;
    }

    screenSetCursorPos(TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line);
    screenEnableCursor();
}

unsigned char screenViewportTile(int width, int height, int x, int y) {
    int centerx, centery, tx, ty;
    const Object *obj;

    centerx = c->saveGame->x;
    centery = c->saveGame->y;
    if (c->map->width == width &&
        c->map->height == height) {
        centerx = width / 2;
        centery = height / 2;
    }

    tx = x + centerx - (width / 2);
    ty = y + centery - (height / 2);

    /* off the edge of the map: wrap or pad with grass tiles */
    if (MAP_IS_OOB(c->map, tx, ty)) {
        if (c->map->border_behavior == BORDER_WRAP) {
	    if (tx < 0)
		tx += c->map->width;
	    if (ty < 0)
		ty += c->map->height;
	    if (tx >= c->map->width)
		tx -= c->map->width;
	    if (ty >= c->map->height)
		ty -= c->map->height;
        }
        else
            return GRASS_TILE;
    }

    if ((obj = mapObjectAt(c->map, tx, ty, 0)) &&
        (!obj->isAvatar || (c->map->flags & SHOW_AVATAR)))
        return obj->tile;

    else
        return mapTileAt(c->map, tx, ty);
}

void screenUpdate() {
    int y, x;

    if (c == NULL) {
        printf("screenUpdate called with NULL\n");
        return;
    }

    screenFindLineOfSight();

    for (y = 0; y < VIEWPORT_H; y++) {
	for (x = 0; x < VIEWPORT_W; x++) {
            if (screenLos[x][y])
                screenShowTile(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x, y), x, y);
            else
                screenShowTile(BLACK_TILE, x, y);
        }
    }
    screenRedrawMapArea();

    screenUpdateCursor();
    screenUpdateMoons();
    screenUpdateWind();
}

void screenCycle() {
    if (++screenCurrentCycle >= SCR_CYCLE_MAX)
        screenCurrentCycle = 0;
}

void screenUpdateCursor() {
    int phase = screenCurrentCycle * 4 / SCR_CYCLE_MAX;

    assert(phase >= 0 && phase < 4);

    if (screenCursorStatus) {
        screenShowChar(31 - phase, screenCursorX, screenCursorY);
        screenRedrawTextArea(screenCursorX, screenCursorY, 1, 1);
    }
}

void screenUpdateMoons() {
    screenShowChar(MOON_CHAR + c->saveGame->trammelphase, 11, 0);
    screenShowChar(MOON_CHAR + c->saveGame->feluccaphase, 12, 0);
    screenRedrawTextArea(11, 0, 2, 1);
}

void screenUpdateWind() {
    screenEraseTextArea(WIND_AREA_X, WIND_AREA_Y, WIND_AREA_W, WIND_AREA_H);
    screenTextAt(WIND_AREA_X, WIND_AREA_Y, "Wind %s", getDirectionName(c->windDirection));
    screenRedrawTextArea(WIND_AREA_X, WIND_AREA_Y, WIND_AREA_W, WIND_AREA_H);
}

void screenEnableCursor() {
    if (!screenCursorStatus)
        screenUpdateCursor();
    screenCursorStatus = 1;
}

void screenDisableCursor() {
    if (screenCursorStatus)
        screenShowChar(' ', screenCursorX, screenCursorY);
    screenCursorStatus = 0;
}

void screenSetCursorPos(int x, int y) {
    screenCursorX = x;
    screenCursorY = y;
}

void screenFindLineOfSight() {
    int y, x;

    if (c == NULL)
        return;

    for (y = 0; y < VIEWPORT_H; y++) {
        for (x = 0; x < VIEWPORT_W; x++) {
            screenLos[x][y] = 0;
        }
    }

    screenLos[VIEWPORT_W / 2][VIEWPORT_H / 2] = 1;

    for (x = VIEWPORT_W / 2 - 1; x >= 0; x--)
        if (screenLos[x + 1][VIEWPORT_H / 2] &&
            !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x + 1, VIEWPORT_H / 2)))
            screenLos[x][VIEWPORT_H / 2] = 1;

    for (x = VIEWPORT_W / 2 + 1; x < VIEWPORT_W; x++)
        if (screenLos[x - 1][VIEWPORT_H / 2] &&
            !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x - 1, VIEWPORT_H / 2)))
            screenLos[x][VIEWPORT_H / 2] = 1;

    for (y = VIEWPORT_H / 2 - 1; y >= 0; y--)
        if (screenLos[VIEWPORT_W / 2][y + 1] &&
            !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, VIEWPORT_W / 2, y + 1)))
            screenLos[VIEWPORT_W / 2][y] = 1;

    for (y = VIEWPORT_H / 2 + 1; y < VIEWPORT_H; y++)
        if (screenLos[VIEWPORT_W / 2][y - 1] &&
            !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, VIEWPORT_W / 2, y - 1)))
            screenLos[VIEWPORT_W / 2][y] = 1;

    for (y = VIEWPORT_H / 2 - 1; y >= 0; y--) {
        
        for (x = VIEWPORT_W / 2 - 1; x >= 0; x--) {
            if (screenLos[x][y + 1] &&
                !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x, y + 1)))
                screenLos[x][y] = 1;
            else if (screenLos[x + 1][y] &&
                !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x + 1, y)))
                screenLos[x][y] = 1;
            else if (screenLos[x + 1][y + 1] &&
                !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x + 1, y + 1)))
                screenLos[x][y] = 1;
        }
                
        for (x = VIEWPORT_W / 2 + 1; x < VIEWPORT_W; x++) {
            if (screenLos[x][y + 1] &&
                !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x, y + 1)))
                screenLos[x][y] = 1;
            else if (screenLos[x - 1][y] &&
                !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x - 1, y)))
                screenLos[x][y] = 1;
            else if (screenLos[x - 1][y + 1] &&
                !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x - 1, y + 1)))
                screenLos[x][y] = 1;
        }
    }

    for (y = VIEWPORT_H / 2 + 1; y < VIEWPORT_H; y++) {
        
        for (x = VIEWPORT_W / 2 - 1; x >= 0; x--) {
            if (screenLos[x][y - 1] &&
                !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x, y - 1)))
                screenLos[x][y] = 1;
            else if (screenLos[x + 1][y] &&
                !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x + 1, y)))
                screenLos[x][y] = 1;
            else if (screenLos[x + 1][y - 1] &&
                !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x + 1, y - 1)))
                screenLos[x][y] = 1;
        }
                
        for (x = VIEWPORT_W / 2 + 1; x < VIEWPORT_W; x++) {
            if (screenLos[x][y - 1] &&
                !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x, y - 1)))
                screenLos[x][y] = 1;
            else if (screenLos[x - 1][y] &&
                !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x - 1, y)))
                screenLos[x][y] = 1;
            else if (screenLos[x - 1][y - 1] &&
                !tileIsOpaque(screenViewportTile(VIEWPORT_W, VIEWPORT_H, x - 1, y - 1)))
                screenLos[x][y] = 1;
        }
    }
}

