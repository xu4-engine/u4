/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "shrine.h"
#include "u4.h"
#include "context.h"
#include "screen.h"
#include "event.h"
#include "names.h"
#include "annotation.h"
#include "map.h"
#include "music.h"
#include "game.h"
#include "player.h"

#define MEDITATION_MANTRAS_PER_CYCLE 16

const Shrine *shrine;
char virtueBuffer[20];
int cycles, completedCycles;
char mantraBuffer[20];
int reps;

int shrineHandleVirtue(const char *message);
int shrineHandleCycles(char choice);
void shrineMeditationCycle();
void shrineTimer(void *data);
int shrineHandleMantra(const char *message);
int shrineEjectOnKey(int key, void *data);
void shrineEject();

void shrineEnter(const Shrine *s) {
    ReadBufferActionInfo *info;

    shrine = s;
    screenMessage("You enter the ancient shrine and sit before the altar...\nUpon which virtue dost thou meditate?\n");

    virtueBuffer[0] = '\0';
    info = (ReadBufferActionInfo *) malloc(sizeof(ReadBufferActionInfo));
    info->buffer = virtueBuffer;
    info->bufferLen = sizeof(virtueBuffer);
    info->handleBuffer = &shrineHandleVirtue;
    info->screenX = TEXT_AREA_X + c->col;
    info->screenY = TEXT_AREA_Y + c->line;
    eventHandlerPushKeyHandlerData(&keyHandlerReadBuffer, info);
}

int shrineHandleVirtue(const char *message) {
    GetChoiceActionInfo *info;

    eventHandlerPopKeyHandler();

    screenMessage("\nFor how many Cycles (0-3)? ");

    info = (GetChoiceActionInfo *) malloc(sizeof(GetChoiceActionInfo));
    info->choices = "0123\033";
    info->handleChoice = &shrineHandleCycles;
    eventHandlerPushKeyHandlerData(&keyHandlerGetChoice, info);

    return 1;
}

int shrineHandleCycles(char choice) {
    eventHandlerPopKeyHandler();

    if (choice == '\033')
        cycles = 0;
    else
        cycles = choice - '0';
    completedCycles = 0;

    screenMessage("%c\n\n", cycles + '0');

    if (strcasecmp(virtueBuffer, getVirtueName(shrine->virtue)) != 0 || cycles == 0) {
        screenMessage("Thou art unable to focus thy thoughts on this subject!\n");
        shrineEject();
    } else {
        screenMessage("Begin Meditation\n");
        shrineMeditationCycle();
    }

    return 1;
}

void shrineMeditationCycle() {
    reps = 0;

    screenDisableCursor();
    eventHandlerPushKeyHandler(&keyHandlerIgnoreKeys);
    eventHandlerAddTimerCallback(&shrineTimer, 4);
}

void shrineTimer(void *data) {
    ReadBufferActionInfo *info;

    if (reps++ >= MEDITATION_MANTRAS_PER_CYCLE) {
        eventHandlerRemoveTimerCallback(&shrineTimer);
        eventHandlerPopKeyHandler();

        screenMessage("Mantra: ");

        mantraBuffer[0] = '\0';
        info = (ReadBufferActionInfo *) malloc(sizeof(ReadBufferActionInfo));
        info->buffer = mantraBuffer;
        info->bufferLen = sizeof(mantraBuffer);
        info->handleBuffer = &shrineHandleMantra;
        info->screenX = TEXT_AREA_X + c->col;
        info->screenY = TEXT_AREA_Y + c->line;
        eventHandlerPushKeyHandlerData(&keyHandlerReadBuffer, info);
        screenRedrawScreen();
    } 
    else {
        screenMessage(".");
        screenDisableCursor();
        screenRedrawScreen();
    }
}

int shrineHandleMantra(const char *message) {
    eventHandlerPopKeyHandler();

    screenMessage("\n");

    if (strcasecmp(mantraBuffer, shrine->mantra) != 0) {
        playerAdjustKarma(c->saveGame, KA_BAD_MANTRA);
        screenMessage("Thou art not able to focus thy thoughts with that Mantra!\n");
        shrineEject();
    }
    else if (--cycles > 0) {
        completedCycles++;
        playerAdjustKarma(c->saveGame, KA_MEDITATION);
        shrineMeditationCycle();
    }
    else {
        completedCycles++;
        playerAdjustKarma(c->saveGame, KA_MEDITATION);

        if (completedCycles == 3 &&
            playerAttemptElevation(c->saveGame, shrine->virtue)) {
            screenMessage("Thou hast achieved partial Avatarhood in the Virtue of %s\n\n"
                          "Thou art granted a vision!\n", 
                          getVirtueName(shrine->virtue));

            gameSetViewMode(VIEW_RUNE);
            screenDrawBackgroundInMapArea(BKGD_SHRINE_HON + shrine->virtue);

        } else {
            screenMessage("\nThy thoughts are pure.  "
                          "Thou art granted a vision!\n");
            /* FIXME: print advice string */
        }

        eventHandlerPushKeyHandler(&shrineEjectOnKey);
    }

    return 1;
}

int shrineEjectOnKey(int key, void *data) {
    gameSetViewMode(VIEW_NORMAL);
    eventHandlerPopKeyHandler();
    shrineEject();
    return 1;
}

void shrineEject() {
    if (c->parent != NULL) {
        Context *t = c;
        annotationClear(c->map->id);
        mapClearObjects(c->map);
        c->parent->saveGame->x = c->saveGame->dngx;
        c->parent->saveGame->y = c->saveGame->dngy;
        c->parent->annotation = c->annotation;
        c->parent->line = c->line;
        c->parent->moonPhase = c->moonPhase;
        c->parent->windDirection = c->windDirection;
        c->parent->windCounter = c->windCounter;
        c->parent->aura = c->aura;
        c->parent->auraDuration = c->auraDuration;
        c->parent->horseSpeed = c->horseSpeed;
        c = c->parent;
        c->col = 0;
        free(t);
                
        musicPlay();
    }

    gameFinishTurn();
}
