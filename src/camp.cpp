/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include "u4.h"

#include "camp.h"

#include "annotation.h"
#include "city.h"
#include "combat.h"
#include "context.h"
#include "conversation.h"
#include "event.h"
#include "game.h"
#include "location.h"
#include "map.h"
#include "mapmgr.h"
#include "creature.h"
#include "names.h"
#include "object.h"
#include "person.h"
#include "player.h"
#include "screen.h"
#include "settings.h"
#include "sound.h"
#include "stats.h"
#include "tileset.h"
#include "utils.h"
#include "xu4.h"


#define CAMP_FADE_OUT_TIME  1000
#define CAMP_FADE_IN_TIME   0
#define INN_FADE_OUT_TIME   1000
#define INN_FADE_IN_TIME    5000


void campTimer(void *data);
void campEnd(void);
int campHeal(HealType heal_type);
void innTimer(void *data);

CampController::CampController() {
    MapId id;

    /* setup camp (possible, but not for-sure combat situation */
    if (c->location->context & CTX_DUNGEON)
        id = MAP_CAMP_DNG;
    else
        id = MAP_CAMP_CON;

    map = getCombatMap(mapMgr->get(id));
    game->setMap(map, true, NULL, this);
}

void CampController::init(Creature *m) {
    CombatController::init(m);
    camping = true;
}

void CampController::begin() {
    // make sure everyone's asleep
    for (int i = 0; i < c->party->size(); i++)
        c->party->member(i)->putToSleep();

    CombatController::begin();

    musicFadeOut(1000);

    screenMessage("Resting...\n");
    screenDisableCursor();

    EventHandler::wait_msecs(xu4.settings->campTime * 1000);

    screenEnableCursor();

    /* Is the party ambushed during their rest? */
    if (xu4.settings->campingAlwaysCombat || (xu4_random(8) == 0)) {
        const Creature *m = creatureMgr->randomAmbushing();

        musicPlayLocale();
        screenMessage("Ambushed!\n");

        /* create an ambushing creature (so it leaves a chest) */
        setCreature(c->location->prev->map->addCreature(m, c->location->prev->coords));

        /* fill the creature table with creatures and place them */
        fillCreatureTable(m);
        placeCreatures();

        /* creatures go first! */
        finishTurn();
    }
    else {
        /* Wake everyone up! */
        for (int i = 0; i < c->party->size(); i++)
            c->party->member(i)->wakeUp();

        /* Make sure we've waited long enough for camping to be effective */
        bool healed = false;
        if (((c->saveGame->moves / CAMP_HEAL_INTERVAL) >= 0x10000) || (((c->saveGame->moves / CAMP_HEAL_INTERVAL) & 0xffff) != c->saveGame->lastcamp))
            healed = heal();

        screenMessage(healed ? "Party Healed!\n" : "No effect.\n");
        c->saveGame->lastcamp = (c->saveGame->moves / CAMP_HEAL_INTERVAL) & 0xffff;

        eventHandler->popController();
        game->exitToParentMap();
        musicFadeIn(CAMP_FADE_IN_TIME, true);
        delete this;
    }
}

void CampController::end(bool adjustKarma) {
    // wake everyone up!
    for (int i = 0; i < c->party->size(); i++)
        c->party->member(i)->wakeUp();
    CombatController::end(adjustKarma);
}

bool CampController::heal() {
    // restore each party member to max mp, and restore some hp
    bool healed = false;
    for (int i = 0; i < c->party->size(); i++) {
        PartyMember *m = c->party->member(i);
        m->setMp(m->getMaxMp());
        if ((m->getHp() < m->getMaxHp()) && m->heal(HT_CAMPHEAL))
            healed = true;
    }

    return healed;
}

InnController::InnController() {
    map = NULL;
    /*
     * Normally in cities, only one opponent per encounter; inn's
     * override this to get the regular encounter size.
     */
    forceStandardEncounterSize = true;
}

void InnController::begin() {
    /* first, show the avatar before sleeping */
    gameUpdateScreen();

    /* in the original, the vendor music plays straight through sleeping */
    if (xu4.settings->enhancements)
        musicFadeOut(INN_FADE_OUT_TIME); /* Fade volume out to ease into rest */

    EventHandler::wait_msecs(INN_FADE_OUT_TIME);

    /* show the sleeping avatar */
    c->party->setTransport(c->location->map->tileset->getByName("corpse")->getId());
    gameUpdateScreen();

    screenDisableCursor();

    EventHandler::wait_msecs(xu4.settings->innTime * 1000);

    screenEnableCursor();

    /* restore the avatar to normal */
    c->party->setTransport(c->location->map->tileset->getByName("avatar")->getId());
    gameUpdateScreen();

    /* the party is always healed */
    heal();

    /* Is there a special encounter during your stay? */
    // mwinterrowd suggested code, based on u4dos
    if (c->party->member(0)->isDead()) {
        maybeMeetIsaac();
    }
    else {
        if (xu4_random(8) != 0) {
            maybeMeetIsaac();
        }
        else {
            maybeAmbush();
        }
    }

    screenMessage("\nMorning!\n");
    screenPrompt();

    musicFadeIn(INN_FADE_IN_TIME, true);
}

bool InnController::heal() {
    // restore each party member to max mp, and restore some hp
    bool healed = false;
    for (int i = 0; i < c->party->size(); i++) {
        PartyMember *m = c->party->member(i);
        m->setMp(m->getMaxMp());
        if ((m->getHp() < m->getMaxHp()) && m->heal(HT_INNHEAL))
            healed = true;
    }

    return healed;
}


void InnController::maybeMeetIsaac()
{
    // Does Isaac the Ghost pay a visit to the Avatar?
    //  if ((location == skara_brae) && (random(4) = 0) {
    //          // create Isaac the Ghost
    //  }
    if ((c->location->map->id == 11) && (xu4_random(4) == 0)) {
        City *city = dynamic_cast<City*>(c->location->map);

        if (city->extraDialogues.size() == 1 &&
            city->extraDialogues[0]->getName() == "Isaac") {

            Coords coords(27, xu4_random(3) + 10, c->location->coords.z);

            // If Isaac is already around, just bring him back to the inn
            for (ObjectDeque::iterator i = c->location->map->objects.begin();
                 i != c->location->map->objects.end();
                 i++) {
                Person *p = dynamic_cast<Person*>(*i);
                if (p && p->getName() == "Isaac") {
                    p->setCoords(coords);
                    return;
                }
            }

            // Otherwise, we need to create Isaac
            Person *Isaac;
            Isaac = new Person(creatureMgr->getById(GHOST_ID)->getTile());

            Isaac->setMovementBehavior(MOVEMENT_WANDER);

            Isaac->setDialogue(city->extraDialogues[0]);
            Isaac->getStart() = coords;
            Isaac->setPrevTile(Isaac->getTile());

            // Add Isaac near the Avatar
            city->addPerson(Isaac);
        }
    }
}

void InnController::maybeAmbush()
{
    if (xu4.settings->innAlwaysCombat || (xu4_random(8) == 0)) {
        MapId mapid;
        Creature *creature;
        bool showMessage = true;

        /* Rats seem much more rare than meeting rogues in the streets */
        if (xu4_random(4) == 0) {
            /* Rats! */
            mapid = MAP_BRICK_CON;
            creature = c->location->map->addCreature(creatureMgr->getById(RAT_ID), c->location->coords);
        } else {
            /* While strolling down the street, attacked by rogues! */
            mapid = MAP_INN_CON;
            creature = c->location->map->addCreature(creatureMgr->getById(ROGUE_ID), c->location->coords);
            screenMessage("\nIn the middle of the night while out on a stroll...\n\n");
            showMessage = false;
        }

        map = getCombatMap(mapMgr->get(mapid));
        game->setMap(map, true, NULL, this);

        init(creature);
        showCombatMessage(showMessage);
        CombatController::begin();
    }
}

void InnController::awardLoot() {
    // never get a chest from inn combat
}
