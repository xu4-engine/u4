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

int screenCurrentCycle = 0;
int screenCursorX = 0;
int screenCursorY = 0;
int screenCursorStatus = 0;

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

void screenUpdate() {
    int y, x, tile;
    const Object *obj;

    if (c == NULL)
        return;

    for (y = 0; y < VIEWPORT_H; y++) {
	for (x = 0; x < VIEWPORT_W; x++) {

	    if (MAP_IS_OOB(c->map, x + c->saveGame->x - (VIEWPORT_W / 2), y + c->saveGame->y - (VIEWPORT_H / 2)))
		tile = 4;

	    else if ((c->map->flags & SHOW_AVATAR) &&
                     x == (VIEWPORT_W / 2) &&
		     y == (VIEWPORT_H / 2))
		tile = c->saveGame->transport;
	  
	    else if ((obj = mapObjectAt(c->map, x + c->saveGame->x - (VIEWPORT_W / 2), y + c->saveGame->y - (VIEWPORT_H / 2)))) 
		tile = obj->tile;

	    else
		tile = mapTileAt(c->map, x + c->saveGame->x - (VIEWPORT_W / 2), y + c->saveGame->y - (VIEWPORT_H / 2));

	    screenShowTile(tile, x, y);
	}
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
    int phase = screenCurrentCycle * 4 / SCR_CYCLE_MAX;

    assert(phase >= 0 && phase < 4);

    if (screenCursorStatus)
        screenShowChar(31 - phase, screenCursorX, screenCursorY);
}

void screenUpdateMoons() {
    int trammelPhase, feluccaPhase;

    trammelPhase = c->moonPhase / (MOON_SECONDS_PER_PHASE * 4) / 3;
    feluccaPhase = c->moonPhase / (MOON_SECONDS_PER_PHASE * 4) % 8;

    if (--trammelPhase < 0)
        trammelPhase = 7;
    if (--feluccaPhase < 0)
        feluccaPhase = 7;

    assert(trammelPhase >= 0 && trammelPhase < 8);
    assert(feluccaPhase >= 0 && feluccaPhase < 8);

    screenShowChar(MOON_CHAR + trammelPhase, 11, 0);
    screenShowChar(MOON_CHAR + feluccaPhase, 12, 0);
}

void screenUpdateWind() {
    screenEraseTextArea(WIND_AREA_X, WIND_AREA_Y, WIND_AREA_W, WIND_AREA_H);
    screenTextAt(WIND_AREA_X, WIND_AREA_Y, "Wind %s", getDirectionName(c->windDirection));
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
