/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <time.h>

#include "u4.h"
#include "death.h"
#include "game.h"
#include "screen.h"
#include "event.h"
#include "player.h"
#include "context.h"
#include "annotation.h"
#include "location.h"
#include "city.h"
#include "music.h"
#include "stats.h"

#define REVIVE_WORLD_X 86
#define REVIVE_WORLD_Y 107
#define REVIVE_CASTLE_X 19
#define REVIVE_CASTLE_Y 8

int timerCount;
int timerMsg;

void deathTimer(void *data);
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

void deathTimer(void *data) {

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
    while(!mapIsWorldMap(c->location->map) && c->location->prev != NULL) {
        gameExitToParentMap(c);
    }
    
    gameSetMap(c, lcb_2_city.map, 1, NULL);
    c->location->x = REVIVE_CASTLE_X;
    c->location->y = REVIVE_CASTLE_Y;
    c->location->z = 1;    

    c->aura = AURA_NONE;
    c->auraDuration = 0;
    c->horseSpeed = 0;
    c->lastCommandTime = time(NULL);
    gameSetViewMode(VIEW_NORMAL);
    musicPlay();

    playerReviveParty(c->saveGame);

    screenEnableCursor();
    c->statsItem = STATS_PARTY_OVERVIEW;
    statsUpdate();
    screenRedrawScreen();
}
