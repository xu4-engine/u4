/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "u4.h"

#include "camp.h"

#include "map.h"
#include "area.h"
#include "annotation.h"
#include "city.h"
#include "combat.h"
#include "context.h"
#include "event.h"
#include "game.h"
#include "location.h"
#include "mapmgr.h"
#include "monster.h"
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
    
    /* setup camp (possible, but not for-sure combat situation */
    combatInitCamping();
    combatBegin();
    
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
        const Monster *m;

        m = monsterGetAmbushingMonster();
                
        musicPlay();        
        screenMessage("Ambushed!\n");
        
        /* create an ambushing monster (so it leaves a chest) */
        combatInfo.monsterObj = mapAddMonsterObject(c->location->prev->map, m, c->location->prev->coords);
        
        /* fill the monster table with monsters and place them */
        combatFillMonsterTable(m);
        combatPlaceMonsters();

        /* monsters go first! */
        combatFinishTurn();
    }    
    else campEnd();
}

void campEnd(void) {
    int i, healed = 0;

    eventHandlerPopKeyHandler();
    gameExitToParentMap();
    musicFadeIn(CAMP_FADE_IN_TIME, 1);    
    
    /* Wake everyone up! */
    for (i = 0; i < c->saveGame->members; i++) {
        if (c->players[i].status == STAT_SLEEPING)
            c->players[i].status = combatInfo.party[i].status;
    }    

    /* Make sure we've waited long enough for camping to be effective */
    if (((c->saveGame->moves / CAMP_HEAL_INTERVAL) >= 0x10000) || (((c->saveGame->moves / CAMP_HEAL_INTERVAL) & 0xffff) != c->saveGame->lastcamp))
        healed = campHeal(HT_CAMPHEAL);

    screenMessage(healed ? "Party Healed!\n" : "No effect.\n");
    c->saveGame->lastcamp = (c->saveGame->moves / CAMP_HEAL_INTERVAL) & 0xffff;

    combatInfo.camping = 0;
    (*c->location->finishTurn)();
}

int campHeal(HealType heal_type) {
    int i;
    bool healed = false;

    healed = 0;

    /* restore each party member to max mp, and restore some hp */
    for (i = 0; i < c->saveGame->members; i++) {
        c->players[i].mp = playerGetMaxMp(&c->players[i]);
        if ((c->players[i].hp < c->players[i].hpMax) &&
            (playerHeal(heal_type, i)))
            healed = true;
    }
    statsUpdate();

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
    gameSetTransport(CORPSE_TILE);    
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
    gameSetTransport(AVATAR_TILE);
    gameUpdateScreen();

    /* the party is always healed */
    campHeal(HT_INNHEAL);

    /* Is there a special encounter during your stay? */
    if (settings.innAlwaysCombat || (xu4_random(8) == 0)) {
        MapId mapid;
        Object *monsterObj;        
        bool showMessage = true;
            
        /* Rats seem much more rare than meeting rogues in the streets */
        if (xu4_random(4) == 0) {
            /* Rats! */
            mapid = MAP_BRICK_CON;
            monsterObj = mapAddMonsterObject(c->location->map, monsterById(RAT_ID), c->location->coords);
        } else {
            /* While strolling down the street, attacked by rogues! */
            mapid = MAP_INN_CON;
            monsterObj = mapAddMonsterObject(c->location->map, monsterById(ROGUE_ID), c->location->coords);
            screenMessage("\nIn the middle of the night while out on a stroll...\n\n");
            showMessage = false;
        }
        
        combatInfo.inn = 1;
        combatInit(monsterObj->monster, monsterObj, mapid);
        combatInfo.showCombatMessage = showMessage;
        combatBegin();        
    }
    
    else {
        screenMessage("\nMorning!\n");
        screenPrompt();
        screenRedrawScreen();

        /* Does Isaac the Ghost pay a visit to the Avatar? */
        if (c->location->map->id == 11) {// && (xu4_random(4) == 0)) {
            City *city = c->location->map->city;
            Person *Isaac;
            Object *obj;
            int x = 27,
                y = xu4_random(3) + 10,
                z = c->location->coords.z;

            /* If Isaac is already around, just bring him back to the inn */
            for (obj = c->location->map->objects; obj; obj = obj->next) {
                if (obj->getType() == OBJECT_PERSON && obj->person->name && 
                    strcmp(obj->person->name, "Isaac") == 0) {
                    obj->setCoords(MapCoords(x, y, z));
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
            Isaac->movement_behavior = MOVEMENT_WANDER;
            Isaac->npcType = NPC_TALKER;
            Isaac->permanent = 0;
            Isaac->questionType = QUESTION_NORMAL;
            Isaac->startx = 27;
            Isaac->starty = xu4_random(3) + 10;
            Isaac->startz = 0;
            Isaac->tile0 = monsterById(GHOST_ID)->tile;
            Isaac->tile1 = Isaac->tile0 + 1;
            Isaac->turnAwayProb = 0;
            
            /* Add Isaac near the Avatar */
            mapAddPersonObject(c->location->map, Isaac);
        }
    }    
    
    musicFadeIn(INN_FADE_IN_TIME, 1);
}
