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
#include "event.h"
#include "game.h"
#include "location.h"
#include "map.h"
#include "mapmgr.h"
#include "creature.h"
#include "music.h"
#include "names.h"
#include "object.h"
#include "person.h"
#include "player.h"
#include "screen.h"
#include "settings.h"
#include "stats.h"
#include "tile.h"
#include "utils.h"

void campTimer(void *data);
void campEnd(void);
int campHeal(HealType heal_type);
void innTimer(void *data);

void campBegin(void) {    
    musicCamp();
    MapId id;
    
    /* setup camp (possible, but not for-sure combat situation */
    if (c->location->context & CTX_DUNGEON)
        id = MAP_DUNGEON_CON;
    else id = MAP_CAMP_CON;

    delete c->combat;
    c->combat = new CombatController(id);
    c->combat->initCamping();
    c->combat->begin();    
    
    eventHandlerPushKeyHandler(&keyHandlerIgnoreKeys);
    eventHandlerAddTimerCallback(&campTimer, (eventTimerGranularity * settings.gameCyclesPerSecond) * settings.campTime);

    screenMessage("Resting...\n");
    screenDisableCursor();
}

void campTimer(void *data) {
    eventHandlerRemoveTimerCallback(&campTimer);
    eventHandlerPopKeyHandler();
    screenEnableCursor();

    /* Is the party ambushed during their rest? */
    if (settings.campingAlwaysCombat || (xu4_random(8) == 0)) {        
        const Creature *m = creatures.randomAmbushing();
                
        musicPlay();        
        screenMessage("Ambushed!\n");
        
        /* create an ambushing creature (so it leaves a chest) */
        c->combat->setCreature(c->location->prev->map->addCreature(m, c->location->prev->coords));
        
        /* fill the creature table with creatures and place them */
        c->combat->fillCreatureTable(m);
        c->combat->placeCreatures();

        /* creatures go first! */
        c->combat->finishTurn();        
    }    
    else campEnd();
}

void campEnd(void) {
    int i, healed = 0;    

    eventHandlerPopKeyHandler();
    gameExitToParentMap();
    musicFadeIn(CAMP_FADE_IN_TIME, 1);
    
    /* Wake everyone up! */    
    for (i = 0; i < c->party->size(); i++)
        c->party->member(i)->wakeUp();    

    /* Make sure we've waited long enough for camping to be effective */
    if (((c->saveGame->moves / CAMP_HEAL_INTERVAL) >= 0x10000) || (((c->saveGame->moves / CAMP_HEAL_INTERVAL) & 0xffff) != c->saveGame->lastcamp))
        healed = campHeal(HT_CAMPHEAL);

    screenMessage(healed ? "Party Healed!\n" : "No effect.\n");
    c->saveGame->lastcamp = (c->saveGame->moves / CAMP_HEAL_INTERVAL) & 0xffff;
    
    (*c->location->finishTurn)();
}

int campHeal(HealType heal_type) {
    int i;
    bool healed = false;

    healed = 0;

    /* restore each party member to max mp, and restore some hp */
    for (i = 0; i < c->party->size(); i++) {
        PartyMember *m = c->party->member(i);
        m->setMp(m->getMaxMp());
        if ((m->getHp() < m->getMaxHp()) && m->heal(heal_type))
            healed = true;
    }

    return healed;
}

void innBegin(void) {  

    /* first, show the avatar before sleeping */
    gameUpdateScreen();

    /* in the original, the vendor music plays straight through sleeping */
    if (settings.enhancements)
        musicFadeOut(INN_FADE_OUT_TIME); /* Fade volume out to ease into rest */

    eventHandlerSleep(INN_FADE_OUT_TIME);

    /* show the sleeping avatar */
    c->party->setTransport(Tile::findByName("corpse")->id);
    gameUpdateScreen();

    eventHandlerPushKeyHandler(&keyHandlerIgnoreKeys);
    eventHandlerAddTimerCallback(&innTimer, (eventTimerGranularity * settings.gameCyclesPerSecond) * settings.innTime);

    screenDisableCursor();
}

void innTimer(void *data) {
    eventHandlerRemoveTimerCallback(&innTimer);    

    /**
     * FIXME: normally we would "pop" the key handler
     * here, but for some reason the "ignore key"
     * handler doesn't pop every time.  Weird...
     **/
    eventHandlerSetKeyHandler(&gameBaseKeyHandler);
    screenEnableCursor();

    /* restore the avatar to normal */
    c->party->setTransport(Tile::findByName("avatar")->id);
    gameUpdateScreen();

    /* the party is always healed */
    campHeal(HT_INNHEAL);

    /* Is there a special encounter during your stay? */
    if (settings.innAlwaysCombat || (xu4_random(8) == 0)) {
        MapId mapid;
        Creature *creature;
        bool showMessage = true;
            
        /* Rats seem much more rare than meeting rogues in the streets */
        if (xu4_random(4) == 0) {
            /* Rats! */
            mapid = MAP_BRICK_CON;
            creature = c->location->map->addCreature(creatures.getById(RAT_ID), c->location->coords);
        } else {
            /* While strolling down the street, attacked by rogues! */
            mapid = MAP_INN_CON;
            creature = c->location->map->addCreature(creatures.getById(ROGUE_ID), c->location->coords);
            screenMessage("\nIn the middle of the night while out on a stroll...\n\n");
            showMessage = false;
        }

        delete c->combat;
        c->combat = new CombatController(mapid);        
        c->combat->init(creature);
        c->combat->setInn(true);
        c->combat->showCombatMessage(showMessage);  
        c->combat->begin();
    }
    
    else {
        screenMessage("\nMorning!\n");
        screenPrompt();
        screenRedrawScreen();

        /* Does Isaac the Ghost pay a visit to the Avatar? */
        if (c->location->map->id == 11) {// && (xu4_random(4) == 0)) {
            City *city = dynamic_cast<City*>(c->location->map);
            Person *Isaac;
            ObjectDeque::iterator i;
            int x = 27,
                y = xu4_random(3) + 10,
                z = c->location->coords.z;

            /* If Isaac is already around, just bring him back to the inn */
            for (i = c->location->map->objects.begin();
                 i != c->location->map->objects.end();
                 i++) {
                Person *p = dynamic_cast<Person*>(*i);
                if (p && p->name == "Isaac") {                
                    p->setCoords(Coords(x, y, z));
                    return;
                }
            }

            /**
             * FIXME: Isaac is currently broken
             */ 

            /* Otherwise, we need to create Isaac */
            Isaac = new Person;
            Isaac->name = city->persons[10]->name;
            Isaac->pronoun = city->persons[10]->pronoun;
            Isaac->description = city->persons[10]->description;
            Isaac->job = city->persons[10]->job;
            Isaac->health = city->persons[10]->health;
            Isaac->response1 = city->persons[10]->response1;
            Isaac->response2 = city->persons[10]->response2;
            Isaac->question = city->persons[10]->question;
            Isaac->yesresp = city->persons[10]->yesresp;
            Isaac->noresp = city->persons[10]->noresp;
            Isaac->keyword1 = city->persons[10]->keyword1;
            Isaac->keyword2 = city->persons[10]->keyword2;
            Isaac->questionTrigger = QTRIGGER_KEYWORD1;
            Isaac->setMovementBehavior(MOVEMENT_WANDER);
            Isaac->npcType = NPC_TALKER;            
            Isaac->questionType = QUESTION_NORMAL;
            Isaac->start = MapCoords(27, xu4_random(3) + 10, 0);
            Isaac->setTile(creatures.getById(GHOST_ID)->getTile());
            Isaac->setPrevTile(Isaac->getTile());            
            Isaac->turnAwayProb = 0;
            
            /* Add Isaac near the Avatar */
            city->addPerson(Isaac);
        }
    }    
    
    musicFadeIn(INN_FADE_IN_TIME, 1);
}
