/*
 * $Id$
 */

#include <stdlib.h>
#include <string.h>

#include "codex.h"

#include "context.h"
#include "event.h"
#include "game.h"
#include "item.h"
#include "names.h"
#include "savegame.h"
#include "screen.h"
#include "stats.h"
#include "u4.h"
#include "u4file.h"
#include "utils.h"

int codexInit();
void codexDelete();
void codexEject(CodexEjectCode code);
int codexHandleWOP(const char *word);
int codexHandleVirtues(const char *virtue);
int codexHandleInfinity(const char *answer);
void codexImpureThoughts();

int codexHandleInfinityAnyKey(int key, void *data);
int codexHandleEndgameAnyKey(int key, void *data);

char codexInputBuffer[32];
char **codexVirtueQuestions;
char **codexEndgameText1;
char **codexEndgameText2;

/**
 * Initializes the Chamber of the Codex sequence (runs from codexStart())
 */
int codexInit() {
    U4FILE *avatar;
    
    avatar = u4fopen("avatar.exe");
    if (!avatar)
        return 0;
        
    codexVirtueQuestions = u4read_stringtable(avatar, 0x0fc7b, 11);
    codexEndgameText1 = u4read_stringtable(avatar, 0x0fee4, 7);
    codexEndgameText2 = u4read_stringtable(avatar, 0x10187, 5);

    u4fclose(avatar);

    return 1;
}

/**
 * Frees all memory associated with the Codex sequence
 */
void codexDelete() {
    free(codexVirtueQuestions);
    free(codexEndgameText1);
    free(codexEndgameText2);
}

/**
 * Begins the Chamber of the Codex sequence
 */
void codexStart() { 
    codexInit();    

    /**
     * disable the whirlpool cursor and black out the screen
     */
    screenDisableCursor();
    screenUpdate(0, 1);
    
    /**
     * make the avatar alone
     */
    statsAreaClear();
    statsShowPartyView(0); /* show just the avatar */
    screenRedrawScreen();

    /**
     * change the view mode so the dungeon doesn't get shown
     */
    gameSetViewMode(VIEW_CODEX);

    screenMessage("\n\n\n\nThere is a sudden darkness, and you find yourself alone in an empty chamber.\n");    
    eventHandlerSleep(4000);

    /**
     * check to see if you have the 3-part key
     */
    if ((c->saveGame->items & (ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T)) != (ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T)) {
        codexEject(CODEX_EJECT_NO_3_PART_KEY);
        return;
    }

    screenDrawBackgroundInMapArea(BKGD_KEY);
    screenRedrawMapArea();

    screenMessage("\nYou use your key of Three Parts.\n");
    eventHandlerSleep(3000);

    screenMessage("\nA voice rings out:\n\"What is the Word of Passage?\"\n\n");    
    
    /**
     * Get the Word of Passage
     */
    gameGetInput(&codexHandleWOP, codexInputBuffer, sizeof(codexInputBuffer), 0, 0);
}

/**
 * Ejects you from the chamber of the codex (and the Abyss, for that matter)
 * with the correct message.
 */
void codexEject(CodexEjectCode code) {
    struct {
        int x, y;
    } startLocations[] = {
        { 231, 136 },
        { 83, 105 },
        { 35, 221 },
        { 59, 44 },
        { 158, 21 },
        { 105, 183 },
        { 23, 129 },
        { 186, 171 }        
    };

    switch(code) {
    case CODEX_EJECT_NO_3_PART_KEY:
        screenMessage("\nThou dost not have the Key of Three Parts.\n\n");
        break;    
    case CODEX_EJECT_NO_FULL_AVATAR:
        screenMessage("\nThou art not ready.\n");
        eventHandlerSleep(2000);
        screenMessage("\nPassage is not granted.\n\n");
        break;
    case CODEX_EJECT_BAD_WOP:        
        screenMessage("\nPassage is not granted.\n\n");
        break;
    case CODEX_EJECT_HONESTY:
    case CODEX_EJECT_COMPASSION:
    case CODEX_EJECT_VALOR:
    case CODEX_EJECT_JUSTICE:
    case CODEX_EJECT_SACRIFICE:
    case CODEX_EJECT_HONOR:
    case CODEX_EJECT_SPIRITUALITY:
    case CODEX_EJECT_HUMILITY:
    case CODEX_EJECT_TRUTH:
    case CODEX_EJECT_LOVE:
    case CODEX_EJECT_COURAGE:
        screenMessage("\nThy quest is not yet complete.\n\n");
        break;
    case CODEX_EJECT_BAD_INFINITY:
        screenMessage("\nThou dost not know the true nature of the Universe.\n\n");
        break;
    default: 
        screenMessage("\nOops, you just got too close to beating the game.\nBAD AVATAR!\n");
        break;
    }

    eventHandlerSleep(2000);

    /* free memory associated with the Codex */
    codexDelete();

    /* re-enable the cursor and show it */
    screenEnableCursor();
    screenShowCursor();
        
    /* return view to normal and exit the Abyss */
    gameSetViewMode(VIEW_DUNGEON);
    gameExitToParentMap(c);    
    
    /**
     * if being ejected because of a missed virtue question, 
     * then teleport the party to the starting location for
     * that virtue.
     */
    if (code >= CODEX_EJECT_HONESTY && code <= CODEX_EJECT_HUMILITY) {
        int virtue = code - CODEX_EJECT_HONESTY;
        c->location->x = startLocations[virtue].x;
        c->location->y = startLocations[virtue].y;        
    }

    /* finally, finish the turn */
    (*c->location->finishTurn)();
}

/**
 * Handles entering the Word of Passage
 */
int codexHandleWOP(const char *word) {
    static int tries = 1;
    int i;

    eventHandlerPopKeyHandler();    

    /* slight pause before continuing */
    screenMessage("\n");    
    screenDisableCursor();
    eventHandlerSleep(1000);    
        
    /* entered correctly */
    if (strcasecmp(word, "veramocor") == 0) {        
        tries = 1; /* reset 'tries' in case we need to enter this again later */

        /* eject them if they're not a full avatar at this point */
        for (i = 0; i < VIRT_MAX; i++) {
            if (c->saveGame->karma[i] != 0) {
                codexEject(CODEX_EJECT_NO_FULL_AVATAR);
                return 0;
            }
        }        
        
        screenEraseMapArea();
        screenRedrawMapArea();

        screenMessage("\nPassage is granted.\n");
        eventHandlerSleep(4000);

        /* Ask the Virtue questions */
        screenMessage("\n\nThe voice asks:\n");
        eventHandlerSleep(2000);
        screenMessage("\n%s\n\n", codexVirtueQuestions[0]);

        gameGetInput(&codexHandleVirtues, codexInputBuffer, sizeof(codexInputBuffer), 0, 0);
        
        return 1;
    }
    
    /* entered incorrectly - give 3 tries before ejecting */
    else if (tries++ < 3) {
        codexImpureThoughts();
        screenMessage("\"What is the Word of Passage?\"\n\n");

        gameGetInput(&codexHandleWOP, codexInputBuffer, sizeof(codexInputBuffer), 0, 0);        
    }
    
    /* 3 tries are up... eject! */
    else {
        tries = 1;
        codexEject(CODEX_EJECT_BAD_WOP);
    }

    return 0;
}

/**
 * Handles naming of virtues in the Chamber of the Codex
 */
int codexHandleVirtues(const char *virtue) {    
    static int current = 0;
    static int tries = 1;

    eventHandlerPopKeyHandler();

    /* slight pause before continuing */    
    screenMessage("\n");
    screenDisableCursor();    
    eventHandlerSleep(1000);    
        
    /* answered with the correct one of eight virtues */
    if ((current < VIRT_MAX) && 
        (strcasecmp(virtue, getVirtueName((Virtue)current)) == 0)) {

        screenDrawBackgroundInMapArea(BKGD_HONESTY + current);
        screenRedrawMapArea();

        current++;
        tries = 1;

        /* FIXME: draw graphic here */
        //eventHandlerSleep(2000);

        if (current == VIRT_MAX) {
            screenMessage("\nThou art well versed in the virtues of the Avatar.\n");
            eventHandlerSleep(5000);
        }

        screenMessage("\n\nThe voice asks:\n");
        eventHandlerSleep(2000);
        screenMessage("\n%s\n\n", codexVirtueQuestions[current]);

        gameGetInput(&codexHandleVirtues, codexInputBuffer, sizeof(codexInputBuffer), 0, 0);
    }

    /* answered with the correct base virtue (truth, love, courage) */
    else if ((current >= VIRT_MAX) &&
             (strcasecmp(virtue, getBaseVirtueName((BaseVirtue)(1 << (current - VIRT_MAX)))) == 0)) {

        screenDrawBackgroundInMapArea(BKGD_HONESTY + current);
        screenRedrawMapArea();

        current++;
        tries = 1;

        if (current < VIRT_MAX+3) {
            screenMessage("\n\nThe voice asks:\n");
            eventHandlerSleep(2000);
            screenMessage("\n%s\n\n", codexVirtueQuestions[current]);
    
            gameGetInput(&codexHandleVirtues, codexInputBuffer, sizeof(codexInputBuffer), 0, 0);
        }
        else {
            screenMessage("\nThe ground rumbles beneath your feet.\n");
            eventHandlerSleep(1000);
            screenShake(10);

            eventHandlerSleep(3000);
            screenEnableCursor();
            screenMessage("\nAbove the din, the voice asks:\n\nIf all eight virtues of the Avatar combine into and are derived from the Three Principles of Truth, Love and Courage...");
            eventHandlerPushKeyHandler(&codexHandleInfinityAnyKey);
        }
    }
    
    /* give them 3 tries to enter the correct virtue, then eject them! */
    else if (tries++ < 3) {
        codexImpureThoughts();
        screenMessage("%s\n\n", codexVirtueQuestions[current]);
    
        gameGetInput(&codexHandleVirtues, codexInputBuffer, sizeof(codexInputBuffer), 0, 0);
    }
    
    /* failed 3 times... eject! */
    else {        
        codexEject(CODEX_EJECT_HONESTY + current);

        tries = 1;        
        current = 0;
    }

    return 1;
}

int codexHandleInfinityAnyKey(int key, void *data) {
    eventHandlerPopKeyHandler();

    screenMessage("\n\nThen what is the one thing which encompasses and is the whole of all undeniable Truth, unending Love, and unyielding Courage?\n\n");
    gameGetInput(&codexHandleInfinity, codexInputBuffer, sizeof(codexInputBuffer), 0, 0);
    return 1;
}

int codexHandleInfinity(const char *answer) {
    static int tries = 1;

    eventHandlerPopKeyHandler();

    /* slight pause before continuing */    
    screenMessage("\n");
    screenDisableCursor();
    eventHandlerSleep(1000);

    if (strcasecmp(answer, "infinity") == 0) {        
        eventHandlerSleep(2000);
        screenShake(10);
        
        screenEnableCursor();
        screenMessage("\n%s", codexEndgameText1[0]);
        eventHandlerPushKeyHandler(&codexHandleEndgameAnyKey); 
    }
    else if (tries++ < 3) {
        codexImpureThoughts();
        screenMessage("\nAbove the din, the voice asks:\n\nIf all eight virtues of the Avatar combine into and are derived from the Three Principles of Truth, Love and Courage...");
        eventHandlerPushKeyHandler(&codexHandleInfinityAnyKey);
    }
    else codexEject(CODEX_EJECT_BAD_INFINITY);

    return 1;
}

int codexHandleEndgameAnyKey(int key, void *data) {
    static int index = 1;

    eventHandlerPopKeyHandler();

    if (index < 10) {
        if (index < 7)
            screenMessage(codexEndgameText1[index]);
        else if (index == 7)
            screenMessage("\n\n%s", codexEndgameText2[index-7]);
        else if (index > 7)
            screenMessage(codexEndgameText2[index-7]);        
    
        index++;       
        eventHandlerPushKeyHandler(&codexHandleEndgameAnyKey);
    }
    else {
        /* CONGRATULATIONS!... you have completed the game in x turns */    
        screenDisableCursor();
        screenMessage("%s%d%s", codexEndgameText2[index-7], c->saveGame->moves, codexEndgameText2[index-6]);        
        eventHandlerPushKeyHandler(&keyHandlerIgnoreKeys);        
    }

    return 1;
}

/**
 * Pretty self-explanatory
 */
void codexImpureThoughts() {
    screenMessage("\nThy thoughts are not pure.\nI ask again.\n");
    eventHandlerSleep(2000);
}
