/*
 * $Id$
 */

#include "codex.h"

#include "context.h"
#include "event.h"
#include "game.h"
#include "savegame.h"
#include "screen.h"

void codexEject(CodexEjectCode code);

void codexStart() {    
    screenUpdate(0, 1); /* black out the screen */
    gameSetViewMode(VIEW_CODEX);

    screenMessage("\n\n\n\nThere is a sudden darkness, and you find yourself alone in an empty chamber.\n");    
    eventHandlerSleep(2000);

    /* check to see if you have the 3-part key */
    if ((c->saveGame->items & (ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T)) != (ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T)) {
        codexEject(CODEX_EJECT_NO_3_PART_KEY);
        return;
    }

    screenMessage("\nYou use your key of Three Parts.\n");
    eventHandlerSleep(3000);

    screenMessage("\nA voice rings out:\n\"What is the Word of Passage?\"\n\n");
    /* FIXME */
}

void codexEject(CodexEjectCode code) {
    switch(code) {
    case CODEX_EJECT_NO_3_PART_KEY:
        screenMessage("\nThou dost not have the Key of Three Parts.\n\n");
        break;
    case CODEX_EJECT_NO_FULL_AVATAR:
        break;
    default: break;
    }
        
    gameSetViewMode(VIEW_DUNGEON);
    gameExitToParentMap(c);
    (*c->location->finishTurn)();    
}
