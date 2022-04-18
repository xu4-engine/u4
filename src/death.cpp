/*
 * death.cpp
 */

#include "config.h"
#include "game.h"
#include "mapmgr.h"
#include "party.h"
#include "portal.h"
#include "screen.h"
#include "settings.h"
#include "stats.h"
#include "u4.h"
#include "xu4.h"

//#define REVIVE_WORLD_X 86
//#define REVIVE_WORLD_Y 107
#define REVIVE_CASTLE_X 19
#define REVIVE_CASTLE_Y 8

#define PAUSE_SEC   5

class DeathController : public Controller {
public:
    DeathController()
        : Controller(xu4.settings->gameCyclesPerSecond * PAUSE_SEC)
    {
        setDeleteOnPop();
        msg = 0;
    }

    void timerFired();

    uint16_t msg;
};

void deathStart(int delaySeconds) {
    EventHandler* eh = xu4.eventHandler;
    if (dynamic_cast<DeathController *>(eh->getController()))
        return;

    // stop playing music
    musicFadeOut(1000);
    screenDisableCursor();

    if (delaySeconds > 0) {
        if(EventHandler::wait_msecs(delaySeconds * 1000))
            return;
    }

    eh->pushController(new DeathController);
}

static void deathRevive() {
    while(! c->location->map->isWorldMap() && c->location->prev != NULL)
        xu4.game->exitToParentMap();

    xu4.eventHandler->popController();      // Deletes the DeathController

    gameSetViewMode(VIEW_NORMAL);

    /* Move our world map location to Lord British's Castle */
    c->location->coords = c->location->map->portals[0]->coords;

    /* Now, move the avatar into the castle and put him
       in front of Lord British */
    xu4.game->setMap(xu4.config->map(MAP_CASTLE_LB2), 1, NULL);
    Coords& coord = c->location->coords;
    coord.x = REVIVE_CASTLE_X;
    coord.y = REVIVE_CASTLE_Y;
    coord.z = 0;

    c->aura.set(Aura::NONE, 0);
    c->horseSpeed = 0;
    gameStampCommandTime();
    musicPlayLocale();

    c->party->reviveParty();

    screenEnableCursor();
    screenShowCursor();
    c->stats->setView(STATS_PARTY_OVERVIEW);
}

static const char* deathMsgs[] = {
    "\n\n\nAll is Dark...\n",
    "\nBut wait...\n",
    "Where am I?...\n",
    "Am I dead?...\n",
    "Afterlife?...\n",
    "You hear:\n%s\n",
    "I feel motion...\n",
    "\nLord British says: I have pulled thy spirit and some possessions from the void.  Be more careful in the future!\n\020"
};

#define N_MSGS  (sizeof(deathMsgs) / sizeof(char*))

void DeathController::timerFired() {
    if (msg < N_MSGS) {
        if (msg == 0) {
            screenEraseMapArea();
            gameSetViewMode(VIEW_CUTSCENE);
        }

        if (msg == 5) {
            string name = c->party->member(0)->getName();
            int spaces = (TEXT_AREA_W - name.size()) / 2;
            name.insert(0, spaces, ' ');
            screenMessage(deathMsgs[msg], name.c_str());
        } else
            screenMessage(deathMsgs[msg]);

        screenHideCursor();

        if (++msg >= N_MSGS)
            deathRevive();
    }
}
