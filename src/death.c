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

int timerCount;
int timerMsg;

void deathTimer(void);
void deathRevive(void);

const struct {
    int timeout;
    const char *text;
} deathMsgs[] = {
    { 10, "\n\n\nAll is Dark...\n" },
    { 10, "\nBut wait...\n" },
    { 10, "Where am I?...\n" },
    { 10, "Am I dead?...\n" },
    { 10, "Afterlife?...\n" },
    { 10, "You hear:\n    %s\n" },
    { 10, "I feel motion...\n" },
    { 10, "\nLord British says: I have pulled thy spirit and some possessions from the void.  Be more careful in the future!\n\n\020" }
};
    
extern City lcb_2_city;

#define N_MSGS (sizeof(deathMsgs) / sizeof(deathMsgs[0]))

void deathStart() {
    timerCount = 0;
    timerMsg = 0;

    eventHandlerPushKeyHandler(&keyHandlerIgnoreKeys);
    eventHandlerAddTimerCallback(&deathTimer);
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

    c->saveGame->x = 86;
    c->saveGame->y = 107;

    c = gameCloneContext(c);
    gameSetMap(c, lcb_2_city.map, 0);
    c->saveGame->x = 19;
    c->saveGame->y = 8;
    mapAddAvatarObject(c->map, c->saveGame->transport, c->saveGame->x, c->saveGame->y);
    musicPlay();

    playerRevive(c->saveGame);

    screenEnableCursor();
    c->statsItem = STATS_PARTY_OVERVIEW;
    statsUpdate();
    screenRedrawScreen();
}
