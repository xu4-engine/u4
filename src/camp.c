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

void campTimer(void *data);
void campEnd(void);

void campBegin() {
    int i;

    c = gameCloneContext(c);

    gameSetMap(c, &camp_map, 1, NULL);
    c->saveGame->dnglevel = 0;

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
    gameExitToParentMap(c);
    campHeal();   

    gameFinishTurn();
}

void campHeal() {
    int i;

    for (i = 0; i < c->saveGame->members; i++) {
        c->saveGame->players[i].mp = playerGetMaxMp(&c->saveGame->players[i]);
        playerHeal(c->saveGame, HT_HEAL, i);
    }
    statsUpdate();
    screenMessage("Party Healed!\n");
}


