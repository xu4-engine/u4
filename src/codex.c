/*
 * $Id$
 */

#include "codex.h"

#include "context.h"
#include "event.h"
#include "game.h"
#include "item.h"
#include "names.h"
#include "savegame.h"
#include "screen.h"
#include "u4.h"
#include "u4file.h"
#include "utils.h"

int codexInit();
void codexDelete();
void codexEject(CodexEjectCode code);
int getWordOfPassage(const char *word);
int getVirtue(const char *virtue);
void codexImpureThoughts();

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

    screenDisableCursor();
    screenUpdate(0, 1); /* black out the screen */
    gameSetViewMode(VIEW_CODEX);

    screenMessage("\n\n\n\nThere is a sudden darkness, and you find yourself alone in an empty chamber.\n");    
    eventHandlerSleep(4000);

    /* check to see if you have the 3-part key */
    if ((c->saveGame->items & (ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T)) != (ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T)) {
        codexEject(CODEX_EJECT_NO_3_PART_KEY);
        return;
    }

    /* FIXME: draw graphic here */

    screenMessage("\nYou use your key of Three Parts.\n");
    eventHandlerSleep(3000);

    screenMessage("\nA voice rings out:\n\"What is the Word of Passage?\"\n\n");    
    
    /* Prompt for the Word of Passage */
    gameGetInput(&getWordOfPassage, codexInputBuffer, sizeof(codexInputBuffer), 0, 0);
}

/**
 * Ejects you from the chamber of the codex (and the Abyss, for that matter)
 * with the correct message.
 */
void codexEject(CodexEjectCode code) {
    switch(code) {
    case CODEX_EJECT_NO_3_PART_KEY:
        screenMessage("\nThou dost not have the Key of Three Parts.\n\n");
        break;    
    case CODEX_EJECT_NO_FULL_AVATAR:
        screenMessage("\n\nThou art not ready.\n");
        eventHandlerSleep(2000);
        screenMessage("\nPassage is not granted.\n\n");
        break;
    case CODEX_EJECT_BAD_WOP:        
        screenMessage("\n\nPassage is not granted.\n\n");
        break;
    default: 
        screenMessage("\nOops, you just got too close to beating the game.\nBAD AVATAR!\n");
        break;
    }

    codexDelete();
        
    gameSetViewMode(VIEW_DUNGEON);
    gameExitToParentMap(c);
    (*c->location->finishTurn)();    
}

/**
 * Handles entering the Word of Passage
 */
int getWordOfPassage(const char *word) {
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
        
        screenMessage("\nPassage is granted.\n");
        eventHandlerSleep(4000);

        /* Ask the Virtue questions */
        screenMessage("\n\nThe voice asks:\n");
        eventHandlerSleep(1000);
        screenMessage("\n%s\n\n", codexVirtueQuestions[0]);

        gameGetInput(&getVirtue, codexInputBuffer, sizeof(codexInputBuffer), 0, 0);
        
        return 1;
    }
    
    /* entered incorrectly - give 3 tries before ejecting */
    else if (tries++ < 3) {
        codexImpureThoughts();
        screenMessage("\"What is the Word of Passage?\"\n\n");

        gameGetInput(&getWordOfPassage, codexInputBuffer, sizeof(codexInputBuffer), 0, 0);        
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
int getVirtue(const char *virtue) {    
    static int current = 0;
    static int tries = 1;

    eventHandlerPopKeyHandler();

    /* slight pause before continuing */    
    screenMessage("\n");    
    screenDisableCursor();    
    eventHandlerSleep(1000);    
        
    /* answered with the correct one of eight virtues */
    if ((current < VIRT_MAX) && 
        (strncasecmp(virtue, getVirtueName((Virtue)current), 6) == 0)) {
        current++;
        tries = 1;

        /* FIXME: draw graphic here */
        eventHandlerSleep(2000);

        screenMessage("\n\nThe voice asks:\n");
        eventHandlerSleep(1000);
        screenMessage("\n%s\n\n", codexVirtueQuestions[current]);

        gameGetInput(&getVirtue, codexInputBuffer, sizeof(codexInputBuffer), 0, 0);
    }

    /* answered with the correct base virtue (truth, love, courage) */
    else if ((current >= VIRT_MAX) &&
             (strcasecmp(virtue, getBaseVirtueName((BaseVirtue)(1 << (current - VIRT_MAX)))) == 0)) {
        current++;
        tries = 1;

        if (current < VIRT_MAX+3) {
            screenMessage("\n\nThe voice asks:\n");
            eventHandlerSleep(1000);
            screenMessage("\n%s\n\n", codexVirtueQuestions[current]);
    
            gameGetInput(&getVirtue, codexInputBuffer, sizeof(codexInputBuffer), 0, 0);
        }
        else {
            /* FIXME */
            codexEject(-1);
        }
    }
    
    /* give them 3 tries to enter the correct virtue, then eject them! */
    else if (tries++ < 3) {
        codexImpureThoughts();
        screenMessage("%s\n\n", codexVirtueQuestions[current]);
    
        gameGetInput(&getVirtue, codexInputBuffer, sizeof(codexInputBuffer), 0, 0);
    }
    
    /* failed 3 times... eject! */
    else {        
        tries = 1;
        current = 0;
        codexEject(CODEX_EJECT_NO_3_PART_KEY);
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
