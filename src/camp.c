/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "u4.h"

#include "camp.h"

#include "area.h"
#include "annotation.h"
#include "city.h"
#include "combat.h"
#include "context.h"
#include "event.h"
#include "game.h"
#include "location.h"
#include "monster.h"
#include "music.h"
#include "names.h"
#include "object.h"
#include "person.h"
#include "player.h"
#include "screen.h"
#include "stats.h"
#include "ttype.h"

extern Map camp_map;
extern Map brick_map;
extern Map inn_map;

void campTimer(void *data);
void campEnd(void);
int campHeal(void);
void innTimer(void *data);

void campBegin(void) {
    
    /* go to camp music and wait a bit to change to camp view */
    musicCamp();
    eventHandlerSleep(1000);
    
    /* setup camp (possible, but not for-sure combat situation */
    combatBegin(&camp_map, NULL, 0);
    
    eventHandlerPushKeyHandler(&keyHandlerIgnoreKeys);
    eventHandlerAddTimerCallback(&campTimer, 4 * 10);    

    screenMessage("Resting...\n");
    screenDisableCursor();
}

void campTimer(void *data) {
    eventHandlerRemoveTimerCallback(&campTimer);
    eventHandlerPopKeyHandler();
    screenEnableCursor();

    /* Is the party ambushed during their rest? */
    if (rand() % 8 == 0) {
        
        int i, j, numAmbushingMonsters;
        extern CombatInfo combatInfo;
        const Monster *m;

        m = monsterGetAmbushingMonster();
                
        musicPlay();
        musicFadeIn(0);
        screenMessage("Ambushed!\n");
        
        /* assign the monster object for combat */
        combatInfo.monsterObj = mapAddMonsterObject(c->location->prev->map, m, c->location->prev->x, c->location->prev->y, c->location->prev->z);
        
        /* numAmbushingMonsters is the number of creatures we will have in combat */
        numAmbushingMonsters = combatInitialNumberOfMonsters(m);

        /* create the monsters */
        for (i = 0; i < numAmbushingMonsters; i++) {
            /* find a random free slot in the monster table */
            do {j = rand() % AREA_MONSTERS;} while (combatInfo.monsters[j] != NULL);
            combatCreateMonster(j, i != (numAmbushingMonsters - 1));
        }

        /* ok, we're done creating monsters, let's destroy this monster object
           so it won't leave a treasure chest behind */
        mapRemoveObject(c->location->prev->map, combatInfo.monsterObj);

        /* monsters go first! */
        combatFinishTurn();          
    }    
    else campEnd();
}

void campEnd(void) {
    int i, healed = 0;
    extern CombatInfo combatInfo;
    musicFadeIn(0); /* Return volume to normal */

    eventHandlerPopKeyHandler();
    gameExitToParentMap(c);
    musicPlay();
    
    /* Wake everyone up! */
    for (i = 0; i < c->saveGame->members; i++) {
        if (c->saveGame->players[i].status == STAT_SLEEPING)
            c->saveGame->players[i].status = combatInfo.party_status[i];
    }    

    /* Make sure we've waited long enough for camping to be effective */
    if (((c->saveGame->moves / CAMP_HEAL_INTERVAL) & 0xffff) != c->saveGame->lastcamp)    
        healed = campHeal();

    screenMessage(healed ? "Party Healed!\n" : "No effect.\n");
    c->saveGame->lastcamp = (c->saveGame->moves / CAMP_HEAL_INTERVAL) & 0xffff;

    (*c->location->finishTurn)();
}

int campHeal(void) {
    int i, healed;

    healed = 0;

    /* restore each party member to max mp, and heal between 75 and 225 hp */
    for (i = 0; i < c->saveGame->members; i++) {
        c->saveGame->players[i].mp = playerGetMaxMp(&c->saveGame->players[i]);
        if ((c->saveGame->players[i].hp < c->saveGame->players[i].hpMax) &&
            (playerHeal(c->saveGame, HT_RESTHEAL, i)))            
            healed = 1;
    }
    statsUpdate();

    return healed;
}

void innBegin(void) {  

    /* first, show the avatar before sleeping */
    gameUpdateScreen();
    musicFadeOut(1000); /* Fade volume out to ease into rest */

    eventHandlerSleep(1000);

    /* show the sleeping avatar */
    gameSetTransport(CORPSE_TILE);    
    gameUpdateScreen();

    eventHandlerPushKeyHandler(&keyHandlerIgnoreKeys);
    eventHandlerAddTimerCallback(&innTimer, 8 * 4);

    screenDisableCursor();
}

void innTimer(void *data) {
    eventHandlerRemoveTimerCallback(&innTimer);
    eventHandlerPopKeyHandler();
    screenEnableCursor();

    /* restore the avatar to normal */
    gameSetTransport(AVATAR_TILE);
    gameUpdateScreen();

    /* the party is always healed */
    campHeal();

    /* Is there a special encounter during your stay? */
    if (rand() % 8 == 0) {
        
        Map *map;
        Object *monsterObj;        
                
        /* Rats seem much more rare than meeting rogues in the streets */
        if (rand() % 4 == 0) {
            /* Rats! */
            map = &brick_map;
            monsterObj = mapAddMonsterObject(c->location->map, monsterById(RAT_ID), c->location->x, c->location->y, c->location->z);
        } else {
            /* While strolling down the street, attacked by rogues! */
            map = &inn_map;
            monsterObj = mapAddMonsterObject(c->location->map, monsterById(ROGUE_ID), c->location->x, c->location->y, c->location->z);
            screenMessage("\nIn the middle of the night while out on a stroll...\n\n");
        }        

        /* begin combat! */
        combatBegin(map, monsterObj, 0);        
    }
    
    else {
        screenMessage("\nMorning!\n");        

        /* Does Isaac the Ghost pay a visit to the Avatar? */
        if (c->location->map->id == 11 && (rand() % 4 == 0)) {
            City *city = c->location->map->city;
            Person *Isaac;
            Object *obj;
            int x = 27,
                y = (rand() % 3) + 10,
                z = c->location->z;

            /* If Isaac is already around, just bring him back to the inn */
            for (obj = c->location->map->objects; obj; obj = obj->next) {
                if (obj->objType == OBJECT_PERSON && obj->person->name && 
                    strcmp(obj->person->name, "Isaac") == 0) {
                    obj->x = x;
                    obj->y = y;
                    obj->z = z;
                    return;
                }
            }

            /* Otherwise, we need to create Isaac */
            Isaac = (Person *)malloc(sizeof(Person));
            Isaac->name = city->persons[10].name;
            Isaac->pronoun = city->persons[10].pronoun;
            Isaac->description = city->persons[10].description;
            Isaac->job = city->persons[10].job;
            Isaac->health = city->persons[10].health;
            Isaac->response1 = city->persons[10].response1;
            Isaac->response2 = city->persons[10].response2;
            Isaac->question = city->persons[10].question;
            Isaac->yesresp = city->persons[10].yesresp;
            Isaac->noresp = city->persons[10].noresp;
            Isaac->keyword1 = city->persons[10].keyword1;
            Isaac->keyword2 = city->persons[10].keyword2;
            Isaac->questionTrigger = QTRIGGER_KEYWORD1;
            Isaac->movement_behavior = MOVEMENT_WANDER;
            Isaac->npcType = NPC_TALKER;
            Isaac->permanent = 0;
            Isaac->questionType = QUESTION_NORMAL;
            Isaac->startx = 27;
            Isaac->starty = (rand() % 3) + 10;
            Isaac->startz = 0;
            Isaac->tile0 = GHOST_TILE;
            Isaac->tile1 = GHOST_TILE+1;
            Isaac->turnAwayProb = 0;
            Isaac->vendorIndex = 0; 
            
            /* Add Isaac near the Avatar */
            mapAddPersonObject(c->location->map, Isaac);
        }
    }

    musicFadeIn(0);
    musicPlay();
}
