/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "u4.h"
#include "death.h"
#include "game.h"
#include "screen.h"
#include "event.h"
#include "player.h"
#include "context.h"
#include "annotation.h"
#include "map.h"
#include "city.h"
#include "music.h"
#include "stats.h"

#define REVIVE_WORLD_X 86
#define REVIVE_WORLD_Y 107
#define REVIVE_CASTLE_X 19
#define REVIVE_CASTLE_Y 8

int timerCount;
int timerMsg;

void deathTimer(void);
void deathRevive(void);

const struct {
    int timeout;                /* pause in seconds */
    const char *text;           /* text of message */
} deathMsgs[] = {
    { 3, "\n\n\nAll is Dark...\n" },
    { 3, "\nBut wait...\n" },
    { 3, "Where am I?...\n" },
    { 3, "Am I dead?...\n" },
    { 3, "Afterlife?...\n" },
    { 3, "You hear:\n    %s\n" },
    { 3, "I feel motion...\n" },
    { 3, "\nLord British says: I have pulled thy spirit and some possessions from the void.  Be more careful in the future!\n\n\020" }
};
    
extern City lcb_2_city;

#define N_MSGS (sizeof(deathMsgs) / sizeof(deathMsgs[0]))

void deathStart() {
    timerCount = 0;
    timerMsg = 0;

    gameSetViewMode(VIEW_DEAD);

    eventHandlerPushKeyHandler(&keyHandlerIgnoreKeys);
    eventHandlerAddTimerCallback(&deathTimer, 4);
    screenDisableCursor();
}

void deathTimer() {

    timerCount++;
    if (timerCount > deathMsgs[timerMsg].timeout) {

        screenMessage(deathMsgs[timerMsg].text, c->saveGame->players[0].name);
        screenDisableCursor();

        timerCount = 0;
        timerMsg++;

        if (timerMsg >= N_MSGS) {
            eventHandlerRemoveTimerCallback(&deathTimer);
            eventHandlerPopKeyHandler();
            deathRevive();
        }
    }
}

void deathRevive() {
    while(!mapIsWorldMap(c->map)) {
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
                
        }
    }

    c->saveGame->x = REVIVE_WORLD_X;
    c->saveGame->y = REVIVE_WORLD_Y;

    c = gameCloneContext(c);
    gameSetMap(c, lcb_2_city.map, 0);
    c->saveGame->x = REVIVE_CASTLE_X;
    c->saveGame->y = REVIVE_CASTLE_Y;
    c->saveGame->dngx = REVIVE_WORLD_X;
    c->saveGame->dngy = REVIVE_WORLD_Y;
    gameSetViewMode(VIEW_NORMAL);
    musicPlay();

    playerRevive(c->saveGame);

    screenEnableCursor();
    c->statsItem = STATS_PARTY_OVERVIEW;
    statsUpdate();
    screenRedrawScreen();
}
