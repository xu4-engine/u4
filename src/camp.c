/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "camp.h"
#include "u4.h"
#include "context.h"
#include "screen.h"
#include "event.h"
#include "names.h"
#include "annotation.h"
#include "map.h"
#include "music.h"
#include "game.h"
#include "player.h"
#include "area.h"
#include "ttype.h"

extern Map camp_map;
Map *oldmap;

void campTimer(void *data);
void campEnd(void);

void campBegin() {
    int i;

    oldmap = c->map;
    gameSetMap(c, &camp_map, 1);

    musicPlay();

    for (i = 0; i < c->saveGame->members; i++)
        mapAddObject(c->map, CORPSE_TILE, CORPSE_TILE, c->map->area->player_start[i].x, c->map->area->player_start[i].y, c->saveGame->dnglevel);

    eventHandlerPushKeyHandler(&keyHandlerIgnoreKeys);
    eventHandlerAddTimerCallback(&campTimer, 4 * 10);

    screenMessage("Resting...\n");
    screenDisableCursor();
}

void campTimer(void *data) {
    eventHandlerRemoveTimerCallback(&campTimer);
    eventHandlerPopKeyHandler();
    screenEnableCursor();

    campEnd();
}

void campEnd() {
    annotationClear(c->map->id);
    mapClearObjects(c->map);

    c->map = oldmap;
    c->saveGame->x = c->saveGame->dngx;
    c->saveGame->y = c->saveGame->dngy;
    c->col = 0;
                
    musicPlay();

    gameFinishTurn();
}

