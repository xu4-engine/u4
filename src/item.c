/*
 * $Id$
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "item.h"

#include "context.h"
#include "event.h"
#include "game.h"
#include "location.h"
#include "player.h"
#include "portal.h"
#include "savegame.h"
#include "screen.h"
#include "mapmgr.h"

DestroyAllMonstersCallback destroyAllMonstersCallback;

void itemSetDestroyAllMonstersCallback(DestroyAllMonstersCallback callback) {
    destroyAllMonstersCallback = callback;
}

int isRuneInInventory(void *virt);
void putRuneInInventory(void *virt);
int isStoneInInventory(void *virt);
void putStoneInInventory(void *virt);
int isItemInInventory(void *item);
void putItemInInventory(void *item);
void useBBC(void *item);
void useHorn(void *item);
void useWheel(void *item);
void useSkull(void *item);
void useStone(void *item);
void useKey(void *item);
int isMysticInInventory(void *mystic);
void putMysticInInventory(void *mystic);
void useTelescope(void *notused);
int isReagentInInventory(void *reag);
void putReagentInInventory(void *reag);
int isAbyssOpened(const Portal *p);

static const ItemLocation items[] = {
    { "Mandrake Root", NULL, 182, 54, -1, MAP_WORLD,
      &isReagentInInventory, &putReagentInInventory, NULL, (void *) REAG_MANDRAKE, SC_NEWMOONS | SC_REAGENTDELAY },
    { "Mandrake Root", NULL, 100, 165, -1, MAP_WORLD, 
      &isReagentInInventory, &putReagentInInventory, NULL, (void *) REAG_MANDRAKE, SC_NEWMOONS | SC_REAGENTDELAY },
    { "Nightshade", NULL, 46, 149, -1, MAP_WORLD, 
      &isReagentInInventory, &putReagentInInventory, NULL, (void *) REAG_NIGHTSHADE, SC_NEWMOONS | SC_REAGENTDELAY},
    { "Nightshade", NULL, 205, 44, -1, MAP_WORLD, 
      &isReagentInInventory, &putReagentInInventory, NULL, (void *) REAG_NIGHTSHADE, SC_NEWMOONS | SC_REAGENTDELAY },    
    { "the Bell of Courage", "bell", 176, 208, -1, MAP_WORLD, 
      &isItemInInventory, &putItemInInventory, &useBBC, (void *) ITEM_BELL, 0 },
    { "the Book of Truth", "book", 6, 6, 0, MAP_LYCAEUM, 
      &isItemInInventory, &putItemInInventory, &useBBC, (void *) ITEM_BOOK, 0 },
    { "the Candle of Love", "candle", 22, 1, 0, MAP_COVE, 
      &isItemInInventory, &putItemInInventory, &useBBC, (void *) ITEM_CANDLE, 0 },    
    { "A Silver Horn", "horn", 45, 173, -1, MAP_WORLD, 
      &isItemInInventory, &putItemInInventory, &useHorn, (void *) ITEM_HORN, 0 },
    { "the Wheel from the H.M.S. Cape", "wheel", 96, 215, -1, MAP_WORLD, 
      &isItemInInventory, &putItemInInventory, &useWheel, (void *) ITEM_WHEEL, 0 },
    { "the Skull of Modain the Wizard", "skull", 197, 245, -1, MAP_WORLD, 
      &isItemInInventory, &putItemInInventory, &useSkull, (void *) ITEM_SKULL, SC_NEWMOONS },
    { "the Black Stone", NULL, 224, 133, -1, MAP_WORLD, 
      &isStoneInInventory, &putStoneInInventory, NULL, (void *) STONE_BLACK, SC_NEWMOONS },
    { "the White Stone", NULL, 64, 80, -1, MAP_WORLD, 
      &isStoneInInventory, &putStoneInInventory, NULL, (void *) STONE_WHITE, 0 },

    /* handlers for using generic objects */
    { NULL, "stone", -1, -1, 0, MAP_NONE, &isStoneInInventory, NULL, &useStone, NULL, 0 },
    { NULL, "stones", -1, -1, 0, MAP_NONE,&isStoneInInventory, NULL, &useStone, NULL, 0 },
    { NULL, "key", -1, -1, 0, MAP_NONE, &isItemInInventory, NULL, &useKey, (void *)(ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T), 0 },
    { NULL, "keys", -1, -1, 0, MAP_NONE, &isItemInInventory, NULL, &useKey, (void *)(ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T), 0 },    
    
    /* Lycaeum telescope */
    { NULL, NULL, 22, 3, 0, MAP_LYCAEUM, NULL, &useTelescope, NULL, NULL, 0 },

    { "Mystic Armor", NULL, 22, 4, 0, MAP_EMPATH_ABBEY, 
      &isMysticInInventory, &putMysticInInventory, NULL, (void *) ARMR_MYSTICROBES, SC_FULLAVATAR },
    { "Mystic Swords", NULL, 8, 15, 0, MAP_SERPENTS_HOLD, 
      &isMysticInInventory, &putMysticInInventory, NULL, (void *) WEAP_MYSTICSWORD, SC_FULLAVATAR },
    { "the rune of Honesty", NULL, 8, 6, 0, MAP_MOONGLOW, 
      &isRuneInInventory, &putRuneInInventory, NULL, (void *) RUNE_HONESTY, 0 },
    { "the rune of Compassion", NULL, 25, 1, 0, MAP_BRITAIN, 
      &isRuneInInventory, &putRuneInInventory, NULL, (void *) RUNE_COMPASSION, 0 },
    { "the rune of Valor", NULL, 30, 30, 0, MAP_JHELOM, 
      &isRuneInInventory, &putRuneInInventory, NULL, (void *) RUNE_VALOR, 0 },
    { "the rune of Justice", NULL, 13, 6, 0, MAP_YEW, 
      &isRuneInInventory, &putRuneInInventory, NULL, (void *) RUNE_JUSTICE, 0 },
    { "the rune of Sacrifice", NULL, 28, 30, 0, MAP_MINOC, 
      &isRuneInInventory, &putRuneInInventory, NULL, (void *) RUNE_SACRIFICE, 0 },
    { "the rune of Honor", NULL, 2, 29, 0, MAP_TRINSIC, 
      &isRuneInInventory, &putRuneInInventory, NULL, (void *) RUNE_HONOR, 0 },
    { "the rune of Spirituality", NULL, 17, 8, 0, MAP_CASTLE_OF_LORD_BRITISH, 
      &isRuneInInventory, &putRuneInInventory, NULL, (void *) RUNE_SPIRITUALITY, 0 },
    { "the rune of Humility", NULL, 29, 29, 0, MAP_PAWS, 
      &isRuneInInventory, &putRuneInInventory, NULL, (void *) RUNE_HUMILITY, 0 }
};

#define N_ITEMS (sizeof(items) / sizeof(items[0]))

int isRuneInInventory(void *virt) {
    return c->saveGame->runes & (int)virt;
}

void putRuneInInventory(void *virt) {
    playerAwardXp(&c->saveGame->players[0], 100);
    playerAdjustKarma(c->saveGame, KA_FOUND_ITEM);
    c->saveGame->runes |= (int)virt;
    c->saveGame->lastreagent = c->saveGame->moves & 0xF0;
}

int isStoneInInventory(void *virt) {
    /* generic test: does the party have any stones yet? */
    if (virt == NULL) 
        return (c->saveGame->stones > 0);
    /* specific test: does the party have a specific stone? */
    else return c->saveGame->stones & (int)virt;
}

void putStoneInInventory(void *virt) {
    playerAwardXp(&c->saveGame->players[0], 200);
    playerAdjustKarma(c->saveGame, KA_FOUND_ITEM);
    c->saveGame->stones |= (int)virt;
    c->saveGame->lastreagent = c->saveGame->moves & 0xF0;
}

int isItemInInventory(void *item) {
    /* you can't find the skull again once it's destroyed */
    if (((int)item == ITEM_SKULL) && (c->saveGame->items & ITEM_SKULL_DESTROYED))
        return 1;
    else return c->saveGame->items & (int)item;
}

void putItemInInventory(void *item) {
    playerAwardXp(&c->saveGame->players[0], 400);
    playerAdjustKarma(c->saveGame, KA_FOUND_ITEM);
    c->saveGame->items |= (int)item;
    c->saveGame->lastreagent = c->saveGame->moves & 0xF0;
}

void useBBC(void *item) {
    /* on top of the Abyss entrance */
    if (c->location->x == 0xe9 && c->location->y == 0xe9) {
        /* must use bell first */
        if ((int)item == ITEM_BELL) {
            screenMessage("\nThe Bell rings on and on!\n");
            c->saveGame->items |= ITEM_BELL_USED;
        }
        /* then the book */
        else if (((int)item == ITEM_BOOK) && (c->saveGame->items & ITEM_BELL_USED)) {
            screenMessage("\nThe words resonate with the ringing!\n");
            c->saveGame->items |= ITEM_BOOK_USED;
        }
        /* then the candle */
        else if (((int)item == ITEM_CANDLE) && (c->saveGame->items & ITEM_BOOK_USED)) {
            screenMessage("\nAs you light the Candle the Earth Trembles!\n");    
            c->saveGame->items |= ITEM_CANDLE_USED;
        }
        else screenMessage("\nHmm...No effect!\n");
    }
    /* somewhere else */
    else screenMessage("\nHmm...No effect!\n");
}

void useHorn(void *item) {
    screenMessage("\nThe Horn sounds an eerie tone!\n");
    c->aura = AURA_HORN;
    c->auraDuration = 10;
}

void useWheel(void *item) {
    if ((c->transportContext == TRANSPORT_SHIP) && (c->saveGame->shiphull == 50)) {
        screenMessage("\nOnce mounted, the Wheel glows with a blue light!\n");
        c->saveGame->shiphull = 99;        
    }
    else screenMessage("\nHmm...No effect!\n");    
}

void useSkull(void *item) {
    
    /* destroy the skull! pat yourself on the back */
    if (c->location->x == 0xe9 && c->location->y == 0xe9) {
        screenMessage("\n\nYou cast the Skull of Mondain into the Abyss!\n");

        c->saveGame->items = (c->saveGame->items & ~ITEM_SKULL) | ITEM_SKULL_DESTROYED;
        playerAdjustKarma(c->saveGame, KA_DESTROYED_SKULL);
    }

    /* use the skull... bad, very bad */
    else {
        screenMessage("\n\nYou hold the evil Skull of Mondain the Wizard aloft....\n");
    
        /* destroy all monsters */    
        (*destroyAllMonstersCallback)();
    
        /* destroy the skull */
        c->saveGame->items = (c->saveGame->items & ~ITEM_SKULL);
        playerAdjustKarma(c->saveGame, KA_USED_SKULL);
    }
}

void useStone(void *item) {
    screenMessage("\nNo place to Use them!\nHmm...No effect!\n");
}

void useKey(void *item) {
    screenMessage("\nNo place to Use them!\n");
}

int isMysticInInventory(void *mystic) {
    if (((int)mystic) == WEAP_MYSTICSWORD)
        return c->saveGame->weapons[WEAP_MYSTICSWORD] > 0;
    else if (((int)mystic) == ARMR_MYSTICROBES)
        return c->saveGame->armor[ARMR_MYSTICROBES] > 0;
    else {
        assert(0);
        return 0;
    }
}

void putMysticInInventory(void *mystic) {
    playerAwardXp(&c->saveGame->players[0], 400);
    playerAdjustKarma(c->saveGame, KA_FOUND_ITEM);
    if (((int)mystic) == WEAP_MYSTICSWORD)
        c->saveGame->weapons[WEAP_MYSTICSWORD] += 8;
    else if (((int)mystic) == ARMR_MYSTICROBES)
        c->saveGame->armor[ARMR_MYSTICROBES] += 8;
    else
        assert(0);
    c->saveGame->lastreagent = c->saveGame->moves & 0xF0;
}

void useTelescope(void *notused) {
    AlphaActionInfo *alphaInfo;

    alphaInfo = (AlphaActionInfo *) malloc(sizeof(AlphaActionInfo));
    alphaInfo->lastValidLetter = 'p';
    alphaInfo->handleAlpha = gamePeerCity;
    alphaInfo->prompt = "You Select:";
    alphaInfo->data = NULL;

    screenMessage("You see a knob\non the telescope\nmarked A-P\n%s", alphaInfo->prompt);
    eventHandlerPushKeyHandlerData(&gameGetAlphaChoiceKeyHandler, alphaInfo); 
}

int isReagentInInventory(void *reag) {
    return 0;
}

void putReagentInInventory(void *reag) {
    playerAdjustKarma(c->saveGame, KA_FOUND_ITEM);
    c->saveGame->reagents[(int)reag] += rand() % 8 + 2;
    c->saveGame->lastreagent = c->saveGame->moves & 0xF0;

    if (c->saveGame->reagents[(int)reag] > 99) {
        c->saveGame->reagents[(int)reag] = 99;
        screenMessage("Dropped some!\n");
    }
}

int itemConditionsMet(unsigned char conditions) {
    int i;

    if ((conditions & SC_NEWMOONS) &&
        !(c->saveGame->trammelphase == 0 && c->saveGame->feluccaphase == 0))
        return 0;

    if (conditions & SC_FULLAVATAR) {
        for (i = 0; i < VIRT_MAX; i++) {
            if (c->saveGame->karma[i] != 0)
                return 0;
        }
    }

    if ((conditions & SC_REAGENTDELAY) &&
        (c->saveGame->moves & 0xF0) == c->saveGame->lastreagent)
        return 0;

    return 1;
}

/**
 * Returns an item location record if a searchable object exists at
 * the given location. NULL is returned if nothing is there.
 */
const ItemLocation *itemAtLocation(const Map *map, int x, int y, int z) {
    int i;
    for (i = 0; i < N_ITEMS; i++) {
        if (items[i].mapid == map->id && 
            items[i].x == x && 
            items[i].y == y &&
            items[i].z == (unsigned short) z &&
            itemConditionsMet(items[i].conditions))
            return &(items[i]);
    }
    return NULL;
}

void itemUse(const char *shortname) {
    int i;
    const ItemLocation *item = NULL;

    for (i = 0; i < N_ITEMS; i++) {
        if (items[i].shortname &&
            strcasecmp(items[i].shortname, shortname) == 0) {
            
            item = &items[i];

            /* item name found, see if we have that item in our inventory */
            if (!(*items[i].isItemInInventory) || (*items[i].isItemInInventory)(items[i].data)) {       

                /* use the item, if we can! */
                if (!item || !item->useItem)
                    screenMessage("\nNot a Usable item!\n");
                else
                    (*item->useItem)(items[i].data);
            }
            else
                screenMessage("\nNone owned!\n");

            /* we found the item, no need to keep searching */
            break;
        }
    }

    /* item was not found */
    if (!item)
        screenMessage("\nNot a Usable item!\n");
}

int isAbyssOpened(const Portal *p) {
    /* make sure the bell, book and candle have all been used */
    int items = c->saveGame->items;
    int isopened = (items & ITEM_BELL_USED) && (items & ITEM_BOOK_USED) && (items & ITEM_CANDLE_USED);
    
    if (!isopened)
        screenMessage("Enter Can't!\n");
    return isopened;
}
