/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <string>
#include <vector>

#include "codex.h"

#include "context.h"
#include "event.h"
#include "game.h"
#include "item.h"
#include "imagemgr.h"
#include "names.h"
#include "savegame.h"
#include "screen.h"
#include "stats.h"
#include "u4.h"
#include "u4file.h"
#include "utils.h"

using namespace std;

int codexInit();
void codexDelete();
void codexEject(CodexEjectCode code);
int codexHandleWOP(string *word);
int codexHandleVirtues(string *virtue);
int codexHandleInfinity(string *answer);
void codexImpureThoughts();

/**
 * Key handlers
 */ 
bool codexHandleInfinityAnyKey(int key, void *data);
bool codexHandleEndgameAnyKey(int key, void *data);

string codexInputBuffer;
vector<string> codexVirtueQuestions;
vector<string> codexEndgameText1;
vector<string> codexEndgameText2;

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
    codexVirtueQuestions.clear();
    codexEndgameText1.clear();
    codexEndgameText2.clear();
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
    screenUpdate(&game->mapArea, false, true);
    
    /**
     * make the avatar alone
     */
    c->stats->setView(STATS_PARTY_OVERVIEW);
    c->stats->update(true);     /* show just the avatar */
    screenRedrawScreen();

    /**
     * change the view mode so the dungeon doesn't get shown
     */
    gameSetViewMode(VIEW_CODEX);

    screenMessage("\n\n\n\nThere is a sudden darkness, and you find yourself alone in an empty chamber.\n");    
    EventHandler::sleep(4000);

    /**
     * check to see if you have the 3-part key
     */
    if ((c->saveGame->items & (ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T)) != (ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T)) {
        codexEject(CODEX_EJECT_NO_3_PART_KEY);
        return;
    }

    screenDrawImageInMapArea(BKGD_KEY);
    screenRedrawMapArea();

    screenMessage("\nYou use your key of Three Parts.\n");
    EventHandler::sleep(3000);

    screenMessage("\nA voice rings out:\n\"What is the Word of Passage?\"\n\n");    
    
    /**
     * Get the Word of Passage
     */
    gameGetInput(&codexHandleWOP, &codexInputBuffer);
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
    case CODEX_EJECT_NO_FULL_PARTY:
      screenMessage("\nThou hast not proved thy leadership in all eight virtues.\n\n");
      EventHandler::sleep(2000);
      screenMessage("\nPassage is not granted.\n\n");      
      break;
    case CODEX_EJECT_NO_FULL_AVATAR:
        screenMessage("\nThou art not ready.\n");
        EventHandler::sleep(2000);
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

    EventHandler::sleep(2000);

    /* free memory associated with the Codex */
    codexDelete();

    /* re-enable the cursor and show it */
    screenEnableCursor();
    screenShowCursor();
        
    /* return view to normal and exit the Abyss */
    gameSetViewMode(VIEW_DUNGEON);
    game->exitToParentMap();    
    
    /**
     * if being ejected because of a missed virtue question, 
     * then teleport the party to the starting location for
     * that virtue.
     */
    if (code >= CODEX_EJECT_HONESTY && code <= CODEX_EJECT_HUMILITY) {
        int virtue = code - CODEX_EJECT_HONESTY;
        c->location->coords.x = startLocations[virtue].x;
        c->location->coords.y = startLocations[virtue].y;        
    }

    /* finally, finish the turn */
    (*c->location->finishTurn)();
}

/**
 * Handles entering the Word of Passage
 */
int codexHandleWOP(string *word) {
    static int tries = 1;
    int i;

    eventHandler->popKeyHandler();    

    /* slight pause before continuing */
    screenMessage("\n");    
    screenDisableCursor();
    EventHandler::sleep(1000);    
        
    /* entered correctly */
    if (strcasecmp(word->c_str(), "veramocor") == 0) {        
        tries = 1; /* reset 'tries' in case we need to enter this again later */

        /* eject them if they don't have all 8 party members */
        if (c->saveGame->members != 8) {
            codexEject(CODEX_EJECT_NO_FULL_PARTY);
            return 0;
        }
        
        /* eject them if they're not a full avatar at this point */
        for (i = 0; i < VIRT_MAX; i++) {
            if (c->saveGame->karma[i] != 0) {
                codexEject(CODEX_EJECT_NO_FULL_AVATAR);
                return 0;
            }
        }        
        
        screenMessage("\nPassage is granted.\n");
        EventHandler::sleep(4000);

        screenEraseMapArea();
        screenRedrawMapArea();

        /* Ask the Virtue questions */
        screenMessage("\n\nThe voice asks:\n");
        EventHandler::sleep(2000);
        screenMessage("\n%s\n\n", codexVirtueQuestions[0].c_str());

        gameGetInput(&codexHandleVirtues, &codexInputBuffer);
        
        return 1;
    }
    
    /* entered incorrectly - give 3 tries before ejecting */
    else if (tries++ < 3) {
        codexImpureThoughts();
        screenMessage("\"What is the Word of Passage?\"\n\n");

        gameGetInput(&codexHandleWOP, &codexInputBuffer);
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
int codexHandleVirtues(string *virtue) {    
    static const char *codexImageNames[] = {
        BKGD_HONESTY, BKGD_COMPASSN, BKGD_VALOR, BKGD_JUSTICE, 
        BKGD_SACRIFIC, BKGD_HONOR, BKGD_SPIRIT, BKGD_HUMILITY,
        BKGD_TRUTH, BKGD_LOVE, BKGD_COURAGE
    };

    static int current = 0;
    static int tries = 1;

    eventHandler->popKeyHandler();

    /* slight pause before continuing */    
    screenMessage("\n");
    screenDisableCursor();    
    EventHandler::sleep(1000);    
        
    /* answered with the correct one of eight virtues */
    if ((current < VIRT_MAX) && 
        (strcasecmp(virtue->c_str(), getVirtueName(static_cast<Virtue>(current))) == 0)) {

        screenDrawImageInMapArea(codexImageNames[current]);
        screenRedrawMapArea();

        current++;
        tries = 1;

        EventHandler::sleep(2000);

        if (current == VIRT_MAX) {
            screenMessage("\nThou art well versed in the virtues of the Avatar.\n");
            EventHandler::sleep(5000);
        }

        screenMessage("\n\nThe voice asks:\n");
        EventHandler::sleep(2000);
        screenMessage("\n%s\n\n", codexVirtueQuestions[current].c_str());

        gameGetInput(&codexHandleVirtues, &codexInputBuffer);
    }

    /* answered with the correct base virtue (truth, love, courage) */
    else if ((current >= VIRT_MAX) &&
             (strcasecmp(virtue->c_str(), getBaseVirtueName(static_cast<BaseVirtue>(1 << (current - VIRT_MAX)))) == 0)) {

        screenDrawImageInMapArea(codexImageNames[current]);
        screenRedrawMapArea();

        current++;
        tries = 1;

        if (current < VIRT_MAX+3) {
            screenMessage("\n\nThe voice asks:\n");
            EventHandler::sleep(2000);
            screenMessage("\n%s\n\n", codexVirtueQuestions[current].c_str());
    
            gameGetInput(&codexHandleVirtues, &codexInputBuffer);
        }
        else {
            screenMessage("\nThe ground rumbles beneath your feet.\n");
            EventHandler::sleep(1000);
            screenShake(10);

            EventHandler::sleep(3000);
            screenEnableCursor();
            screenMessage("\nAbove the din, the voice asks:\n\nIf all eight virtues of the Avatar combine into and are derived from the Three Principles of Truth, Love and Courage...");
            eventHandler->pushKeyHandler(&codexHandleInfinityAnyKey);
        }
    }
    
    /* give them 3 tries to enter the correct virtue, then eject them! */
    else if (tries++ < 3) {
        codexImpureThoughts();
        screenMessage("%s\n\n", codexVirtueQuestions[current].c_str());
    
        gameGetInput(&codexHandleVirtues, &codexInputBuffer);
    }
    
    /* failed 3 times... eject! */
    else {        
        codexEject(static_cast<CodexEjectCode>(CODEX_EJECT_HONESTY + current));

        tries = 1;        
        current = 0;
    }

    return 1;
}

bool codexHandleInfinityAnyKey(int key, void *data) {
    eventHandler->popKeyHandler();

    screenMessage("\n\nThen what is the one thing which encompasses and is the whole of all undeniable Truth, unending Love, and unyielding Courage?\n\n");
    gameGetInput(&codexHandleInfinity, &codexInputBuffer);
    return true;
}

int codexHandleInfinity(string *answer) {
    static int tries = 1;

    eventHandler->popKeyHandler();

    /* slight pause before continuing */    
    screenMessage("\n");
    screenDisableCursor();
    EventHandler::sleep(1000);

    if (strcasecmp(answer->c_str(), "infinity") == 0) {        
        EventHandler::sleep(2000);
        screenShake(10);
        
        screenEnableCursor();
        screenMessage("\n%s", codexEndgameText1[0].c_str());
        eventHandler->pushKeyHandler(&codexHandleEndgameAnyKey); 
    }
    else if (tries++ < 3) {
        codexImpureThoughts();
        screenMessage("\nAbove the din, the voice asks:\n\nIf all eight virtues of the Avatar combine into and are derived from the Three Principles of Truth, Love and Courage...");
        eventHandler->pushKeyHandler(&codexHandleInfinityAnyKey);
    }
    else codexEject(CODEX_EJECT_BAD_INFINITY);

    return 1;
}

bool codexHandleEndgameAnyKey(int key, void *data) {
    static int index = 1;

    eventHandler->popKeyHandler();

    if (index < 10) {

        if (index < 7) {
            if (index == 6) {
                screenEraseMapArea();
                screenRedrawMapArea();
            }
            screenMessage(codexEndgameText1[index].c_str());
        } 
        else if (index == 7) {
            screenDrawImageInMapArea(BKGD_STONCRCL);
            screenRedrawMapArea();
            screenMessage("\n\n%s", codexEndgameText2[index-7].c_str());
        }
        else if (index > 7)
            screenMessage(codexEndgameText2[index-7].c_str());
    
        index++;
        eventHandler->pushKeyHandler(&codexHandleEndgameAnyKey);
    }
    else {
        /* CONGRATULATIONS!... you have completed the game in x turns */    
        screenDisableCursor();
        screenMessage("%s%d%s", codexEndgameText2[index-7].c_str(), c->saveGame->moves, codexEndgameText2[index-6].c_str());
        eventHandler->pushKeyHandler(&KeyHandler::ignoreKeys);        
    }

    return true;
}

/**
 * Pretty self-explanatory
 */
void codexImpureThoughts() {
    screenMessage("\nThy thoughts are not pure.\nI ask again.\n");
    EventHandler::sleep(2000);
}
