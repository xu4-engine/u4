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
#include "object.h"
#include "location.h"
#include "music.h"
#include "game.h"
#include "player.h"
#include "area.h"
#include "ttype.h"
#include "stats.h"
#include "combat.h"
#include "monster.h"

extern Map camp_map;

void campTimer(void *data);
void campEnd(void);
int campHeal();
void innTimer(void *data);
void innEnd(void);

void campBegin() {
    /* setup camp (possible, but not for-sure combat situation */
    combatBegin(CORPSE_TILE, AVATAR_TILE, NULL);
    
    musicFadeOut(2000); /* Fade volume out to ease into camp */    

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
    
        int i, j,
            numAmbushingMonsters = 0,
            randMonster;
        extern Object *monsterObj;
        extern Object *combat_monsters[AREA_MONSTERS];
        extern int numMonsters;
        extern Monster monsters[MAX_MONSTERS];

        /* first, find out how many ambushing monsters we're dealing with */
        for (i = 0; i < numMonsters; i++) {
            if (monsterAmbushes(&monsters[i]))
                numAmbushingMonsters++;
        }
        
        if (numAmbushingMonsters > 0) {
            /* now, randomely select one of them */
            randMonster = rand() % numAmbushingMonsters;
            numAmbushingMonsters = 0;

            /* now, find the one we selected and create monsters */
            for (i = 0; i < numMonsters; i++) {
                if (monsterAmbushes(&monsters[i])) {
                    if (numAmbushingMonsters == randMonster) {

                        musicPlay();
                        screenMessage("Ambushed!\n");
                        
                        /* assign the monster object for combat */
                        monsterObj = mapAddMonsterObject(c->location->prev->map, &monsters[i], c->location->prev->x, c->location->prev->y, c->location->prev->z);
                        
                        /* numAmbushingMonsters now is the number of creatures we will have in combat */
                        numAmbushingMonsters = combatInitialNumberOfMonsters(&monsters[i]);

                        for (i = 0; i < numAmbushingMonsters; i++) {
                            /* find a random free slot in the monster table */
                            do {j = rand() % AREA_MONSTERS;} while (combat_monsters[j] != NULL);
                            combatCreateMonster(j, i != (numAmbushingMonsters - 1));
                        }

                        /* ok, we're done creating monsters, let's destroy this monster object
                           so it won't leave a treasure chest behind */
                        mapRemoveObject(c->location->prev->map, monsterObj);

                        /* monsters go first! */
                        combatFinishTurn();

                        eventHandlerPushKeyHandler(&combatBaseKeyHandler);

                        break;
                    }
                    else numAmbushingMonsters++;                    
                }
            }
        }        
    }    
    else campEnd();    
}

void campEnd() {
    int i, healed = 0;
    musicFadeIn(0); /* Return volume to normal */
    
    gameExitToParentMap(c);

    /* Wake everyone up! */
    for (i = 0; i < c->saveGame->members; i++) {
        if (c->saveGame->players[i].status == STAT_SLEEPING)
            c->saveGame->players[i].status = STAT_GOOD;
    }
    
    /* Make sure we've waited long enough for camping to be effective */
    if (((c->saveGame->moves / CAMP_HEAL_INTERVAL) & 0xffff) != c->saveGame->lastcamp)    
        healed = campHeal();

    screenMessage(healed ? "Party Healed!\n" : "No effect.\n");
    c->saveGame->lastcamp = (c->saveGame->moves / CAMP_HEAL_INTERVAL) & 0xffff;

    (*c->location->finishTurn)();
}

int campHeal() {
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

void innBegin() {
    int i;

    /* first, show the avatar before sleeping */
    gameUpdateScreen();
    eventHandlerSleep(500);

    /* show the sleeping avatar */
    c->saveGame->transport = CORPSE_TILE;
    gameUpdateScreen();

    eventHandlerPushKeyHandler(&keyHandlerIgnoreKeys);
    eventHandlerAddTimerCallback(&innTimer, 5 * 4);    

    screenDisableCursor();
}

void innTimer(void *data) {
    eventHandlerRemoveTimerCallback(&innTimer);
    eventHandlerPopKeyHandler();
    screenEnableCursor();

    /* FIXME: add encounter and special case stuff here */

    innEnd();
}

void innEnd() {
    campHeal();
    
    /* restore avatar to normal */
    c->saveGame->transport = AVATAR_TILE;
    gameUpdateScreen();    
    screenMessage("\nMorning!\n");
}


