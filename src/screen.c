/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "u4.h"
#include "screen.h"
#include "map.h"
#include "person.h"
#include "context.h"
#include "savegame.h"

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
}

void screenUpdate() {
    int y, x, tile;
    const Person *p;

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
}
