/*
 * $Id$
 */

#include "config.h"
#include "game.h"
#include "mapmgr.h"
#include "player.h"
#include "portal.h"
#include "screen.h"
#include "settings.h"
#include "stats.h"
#include "xu4.h"

#define REVIVE_WORLD_X 86
#define REVIVE_WORLD_Y 107
#define REVIVE_CASTLE_X 19
#define REVIVE_CASTLE_Y 8

int timerCount;
unsigned int timerMsg;
int deathSequenceRunning = 0;

void deathTimer(void *data);
void deathRevive(void);

const struct {
    int timeout;                /* pause in seconds */
    const char *text;           /* text of message */
} deathMsgs[] = {
    { 5, "\n\n\nAll is Dark...\n" },
    { 5, "\nBut wait...\n" },
    { 5, "Where am I?...\n" },
    { 5, "Am I dead?...\n" },
    { 5, "Afterlife?...\n" },
    { 5, "You hear:\n    %s\n" },
    { 5, "I feel motion...\n" },
    { 5, "\nLord British says: I have pulled thy spirit and some possessions from the void.  Be more careful in the future!\n\n\020" }
};

#define N_MSGS (sizeof(deathMsgs) / sizeof(deathMsgs[0]))

void deathStart(int delaySeconds) {
    if (deathSequenceRunning)
        return;

    // stop playing music
    musicFadeOut(1000);

    deathSequenceRunning = 1;
    timerCount = 0;
    timerMsg = 0;

    if (delaySeconds > 0) {
        if(EventHandler::wait_msecs(delaySeconds * 1000))
            return;
    }

    gameSetViewMode(VIEW_DEAD);
    screenDisableCursor();

    xu4.eventHandler->pushKeyHandler(&KeyHandler::ignoreKeys);
    xu4.eventHandler->getTimer()->add(&deathTimer, xu4.settings->gameCyclesPerSecond);
}

void deathTimer(void *data) {

    timerCount++;
    if ((timerMsg < N_MSGS) && (timerCount > deathMsgs[timerMsg].timeout)) {

        screenMessage(deathMsgs[timerMsg].text, c->party->member(0)->getName().c_str());
        screenHideCursor();

        timerCount = 0;
        timerMsg++;

        if (timerMsg >= N_MSGS) {
            xu4.eventHandler->getTimer()->remove(&deathTimer);
            deathRevive();
        }
    }
}

void deathRevive() {
    while(!c->location->map->isWorldMap() && c->location->prev != NULL) {
        xu4.game->exitToParentMap();
    }

    xu4.eventHandler->setController(xu4.game);

    deathSequenceRunning = 0;
    gameSetViewMode(VIEW_NORMAL);

    /* Move our world map location to Lord British's Castle */
    c->location->coords = c->location->map->portals[0]->coords;

    /* Now, move the avatar into the castle and put him
       in front of Lord British */
    xu4.game->setMap(xu4.config->map(MAP_CASTLE_LB2), 1, NULL);
    c->location->coords.x = REVIVE_CASTLE_X;
    c->location->coords.y = REVIVE_CASTLE_Y;
    c->location->coords.z = 0;

    c->aura.set(Aura::NONE, 0);
    c->horseSpeed = 0;
    gameStampCommandTime();
    musicPlayLocale();

    c->party->reviveParty();

    screenEnableCursor();
    screenShowCursor();
    c->stats->setView(STATS_PARTY_OVERVIEW);
}
