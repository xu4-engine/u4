/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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

void campTimer(void);
void campEnd(void);

void campBegin() {
    int i;

    annotationClear();

    c = gameCloneContext(c);

    gameSetMap(c, &camp_map, 1);
    musicPlay();

    for (i = 0; i < c->saveGame->members; i++)
        mapAddObject(c->map, CORPSE_TILE, CORPSE_TILE, c->map->area->player_start[i].x, c->map->area->player_start[i].y);

    eventHandlerPushKeyHandler(&keyHandlerIgnoreKeys);
    eventHandlerAddTimerCallback(&campTimer, 4 * 10);

    screenMessage("Resting...\n");
    screenDisableCursor();
}

void campTimer() {
    eventHandlerRemoveTimerCallback(&campTimer);
    eventHandlerPopKeyHandler();
    screenEnableCursor();

    campEnd();
}

void campEnd() {
    if (c->parent != NULL) {
        Context *t = c;
        annotationClear();
        mapClearObjects(c->map);
        c->parent->saveGame->x = c->saveGame->dngx;
        c->parent->saveGame->y = c->saveGame->dngy;
        c->parent->line = c->line;
        c->parent->moonPhase = c->moonPhase;
        c->parent->windDirection = c->windDirection;
        c->parent->windCounter = c->windCounter;
        c = c->parent;
        c->col = 0;
        free(t);
                
        musicPlay();
    }

    gameFinishTurn();
}

