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
#include "location.h"
#include "music.h"
#include "game.h"
#include "player.h"
#include "area.h"
#include "ttype.h"
#include "stats.h"

extern Map camp_map;

void campTimer(void *data);
void campEnd(void);

void campBegin() {
    int i;

    gameSetMap(c, &camp_map, 1, NULL);
    musicFadeOut(2000); /* Fade volume out to ease into camp */

    /* FIXME: when random encounters occur, make sure to call musicPlay() function for battle */

    for (i = 0; i < c->saveGame->members; i++)
        mapAddObject(c->location->map, CORPSE_TILE, CORPSE_TILE, c->location->map->area->player_start[i].x, c->location->map->area->player_start[i].y, c->location->z);

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
    int healed = 0;
    musicFadeIn(0); /* Return volume to normal */
    
    gameExitToParentMap(c);    
    
    if (c->saveGame->moves - c->saveGame->lastcamp > CAMP_HEAL_INTERVAL)
        healed = campHeal();

    screenMessage(healed ? "Party Healed!\n" : "No effect.\n");
    c->saveGame->lastcamp = c->saveGame->moves;

    gameFinishTurn();
}

int campHeal() {
    int i, healed;

    healed = 0;

    for (i = 0; i < c->saveGame->members; i++) {
        c->saveGame->players[i].mp = playerGetMaxMp(&c->saveGame->players[i]);
        if (c->saveGame->players[i].hp < c->saveGame->players[i].hpMax)
        {
            playerHeal(c->saveGame, HT_HEAL, i);
            healed = 1;
        }
    }
    statsUpdate();

    return healed;    
}


