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
#include "person.h"
#include "context.h"
#include "savegame.h"

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

void screenMessage(char *fmt, ...) {
    char buffer[1024];
    int i;

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
        if (buffer[i] == '\n' || c->col == 16) {
            if (buffer[i] == '\n')
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
    const Person *p;

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
	  
	    else if ((p = mapPersonAt(c->map, x + c->saveGame->x - (VIEWPORT_W / 2), y + c->saveGame->y - (VIEWPORT_H / 2)))) 
		tile = p->tile0;

	    else
		tile = mapTileAt(c->map, x + c->saveGame->x - (VIEWPORT_W / 2), y + c->saveGame->y - (VIEWPORT_H / 2));

	    screenShowTile(tile, x, y);
	}
    }

    screenUpdateCursor();
    screenUpdateMoons();
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

    screenShowChar(20 + trammelPhase, 11, 0);
    screenShowChar(20 + feluccaPhase, 12, 0);
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
