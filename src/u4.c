/*
 * $Id$
 */

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <SDL/SDL.h>

#include "u4.h"
#include "screen.h"
#include "event.h"
#include "map.h"
#include "person.h"
#include "ttype.h"
#include "context.h"

const extern Map world_map;
Context *c;

void move(int dx, int dy);

int main(int argc, char *argv[]) {
    screenInit();

    c = (Context *) malloc(sizeof(Context));
    c->parent = NULL;
    c->map = &world_map;
    c->x = c->map->startx;
    c->y = c->map->starty;
    c->state = STATE_NORMAL;
    c->line = 0;
    c->col = 0;

    screenDrawBorders();
    screenUpdate(c);
    SDL_UpdateRect(screen, 0, 0, 0, 0);

    eventHandlerMain();

    return 0;
}

void move(int dx, int dy) {
    int newx, newy;

    newx = c->x + dx;
    newy = c->y + dy;

    if (MAP_IS_OOB(c->map, newx, newy)) {
	switch (c->map->border_behavior) {
	case BORDER_WRAP:
	    if (newx < 0)
		newx += c->map->width;
	    if (newy < 0)
		newy += c->map->height;
	    if (newx >= c->map->width)
		newx -= c->map->width;
	    if (newy >= c->map->height)
		newy -= c->map->height;
	    break;

	case BORDER_EXIT2PARENT:
	    if (c->parent != NULL) {
		Context *t = c;
                c->parent->line = c->line;
		c = c->parent;
		free(t);
	    }
	    return;
	    
	case BORDER_FIXED:
	    if (newx < 0 || newx >= c->map->width)
		newx = c->x;
	    if (newy < 0 || newy >= c->map->height)
		newy = c->y;
	    break;
	}
    }

    if (/*iswalkable(MAP_TILE_AT(c->map, newx, newy)) &&*/
        !mapPersonAt(c->map, newx, newy)) {
	c->x = newx;
	c->y = newy;
    }
}
