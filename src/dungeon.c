/*
 * $Id$
 */

#include <stdlib.h>

#include "dungeon.h"

#include "annotation.h"
#include "context.h"
#include "debug.h"
#include "game.h"
#include "item.h"
#include "location.h"
#include "mapmgr.h"
#include "player.h"
#include "screen.h"
#include "stats.h"
#include "utils.h"

/**
 * Returns the dungeon token associated with the given dungeon tile
 */
DungeonToken dungeonTokenForTile(unsigned char tile) {
    return (DungeonToken)(tile & 0xF0);
}

/**
 * Return the dungeon sub-token associated with the given dungeon tile.
 * For instance, for tile 0x91, returns FOUNTAIN_HEALING
 * NOTE: This function will always need type-casting to the token type necessary
 */
unsigned char dungeonSubTokenForTile(unsigned char tile) {
    return (tile & 0xF);
}

/**
 * Returns the dungeon token for the current location
 */
DungeonToken dungeonCurrentToken() {
    return dungeonTokenAt(c->location->map, c->location->x, c->location->y, c->location->z);
}

/**
 * Returns the dungeon sub-token for the current location
 */
unsigned char dungeonCurrentSubToken() {
    return dungeonSubTokenAt(c->location->map, c->location->x, c->location->y, c->location->z);    
}

/**
 * Returns the dungeon token for the given coordinates
 */
DungeonToken dungeonTokenAt(Map *map, int x, int y, int z) {
    return dungeonTokenForTile(mapGetTileFromData(map, x, y, z));
}

/**
 * Returns the dungeon sub-token for the given coordinates
 */
unsigned char dungeonSubTokenAt(Map *map, int x, int y, int z) {
    return dungeonSubTokenForTile(mapGetTileFromData(map, x, y, z));
}

/**
 * Loads a dungeon room into map->dungeon->room
 */
int dungeonLoadRoom(Dungeon *dng, int room) {
    if (dng->room != NULL)
        free(dng->room);    
    
    dng->room = mapMgrInitMap();

    dng->room->id = 0;
    dng->room->border_behavior = BORDER_FIXED;
    dng->room->width = dng->room->height = 11;
    dng->room->data = dng->rooms[room].map_data;
    dng->room->music = MUSIC_COMBAT;
    dng->room->type = MAPTYPE_COMBAT;
    dng->room->flags |= NO_LINE_OF_SIGHT;

    dng->currentRoom = (dng->rooms + room);
    return 1;
}

/**
 * Handles 's'earching while in dungeons
 */
void dungeonSearch(void) {
    DungeonToken token = dungeonCurrentToken();    
    const Annotation *a = annotationAt(c->location->x, c->location->y, c->location->z, c->location->map->id);
    const ItemLocation *item;
    if (a) 
        token = DUNGEON_CORRIDOR;

    screenMessage("Search...\n");

    switch (token) {
    case DUNGEON_MAGIC_ORB: /* magic orb */
        screenMessage("You find a Magical Ball...\nWho touches? ");
        gameGetPlayerForCommand(&dungeonTouchOrb, 0);
        break;

    case DUNGEON_FOUNTAIN: /* fountains */
        screenMessage("You find a Fountain.\nWho drinks? ");
        gameGetPlayerForCommand(&dungeonDrinkFountain, 0);
        break;

    default: 
        {
            /* see if there is an item at the current location (stones on altars, etc.) */
            item = itemAtLocation(c->location->map, c->location->x, c->location->y, c->location->z);
            if (item) {
                if (*item->isItemInInventory != NULL && (*item->isItemInInventory)(item->data))
                    screenMessage("Nothing Here!\n");
                else {                
                    if (item->name)
                        screenMessage("You find...\n%s!\n", item->name);
                    (*item->putItemInInventory)(item->data);
                }
            } else
                screenMessage("\nYou find Nothing!\n");
        }
        
        break;
    }
}

/**
 * Drink from the fountain at the current location
 */
int dungeonDrinkFountain(int player) {
    int retval = 1;
    FountainType type = (FountainType)dungeonCurrentSubToken();    

    switch(type) {
    /* plain fountain */
    case FOUNTAIN_NORMAL: 
        screenMessage("\nHmmm--No Effect!\n");
        break;

    /* healing fountain */
    case FOUNTAIN_HEALING: 
        if (playerHeal(c->saveGame, HT_FULLHEAL, player))
            screenMessage("\nAhh-Refreshing!\n");
        else screenMessage("\nHmmm--No Effect!\n");
        break;
    
    /* acid fountain */
    case FOUNTAIN_ACID:
        playerApplyDamage(&c->saveGame->players[player], 100); /* 100 damage to drinker */
        screenMessage("\nBleck--Nasty!\n");
        break;

    /* cure fountain */
    case FOUNTAIN_CURE:
        if (playerHeal(c->saveGame, HT_CURE, player))
            screenMessage("\nHmmm--Delicious!\n");
        else screenMessage("\nHmmm--No Effect!\n");
        break;

    /* poison fountain */
    case FOUNTAIN_POISON: 
        if (c->saveGame->players[player].status != STAT_POISONED) {
            playerApplyEffect(c->saveGame, EFFECT_POISON, player);
            playerApplyDamage(&c->saveGame->players[player], 100); /* 100 damage to drinker also */
            screenMessage("\nArgh-Choke-Gasp!\n");
        }
        else screenMessage("\nHmm--No Effect!\n");
        break;

    default:
        ASSERT(0, "Invalid call to dungeonDrinkFountain: no fountain at current location");
        retval = 0;
    }

    statsUpdate();
    return retval;
}

/**
 * Touch the magical ball at the current location
 */
int dungeonTouchOrb(int player) {
    int stats = 0;
    int damage = 0;
    int x, y, z;
    unsigned char replacementTile;
    
    /* Get current position and find a replacement tile for it */
    locationGetCurrentPosition(c->location, &x, &y, &z);
    replacementTile = locationGetReplacementTile(c->location, x, y, z);

    switch(c->location->map->id) {
    case MAP_DECEIT:    stats = STATSBONUS_INT; break;
    case MAP_DESPISE:   stats = STATSBONUS_DEX; break;
    case MAP_DESTARD:   stats = STATSBONUS_STR; break;
    case MAP_WRONG:     stats = STATSBONUS_INT | STATSBONUS_DEX; break;
    case MAP_COVETOUS:  stats = STATSBONUS_DEX | STATSBONUS_STR; break;
    case MAP_SHAME:     stats = STATSBONUS_INT | STATSBONUS_STR; break;
    case MAP_HYTHLOTH:  stats = STATSBONUS_INT | STATSBONUS_DEX | STATSBONUS_STR; break;
    default: break;
    }

    /* give stats bonuses */
    if (stats & STATSBONUS_STR) {
        screenMessage("Strength + 5\n");
        AdjustValue(c->saveGame->players[player].str, 5, 50);
        damage += 200;
    }
    if (stats & STATSBONUS_DEX) {
        screenMessage("Dexterity + 5\n");
        AdjustValue(c->saveGame->players[player].dex, 5, 50);        
        damage += 200;
    }
    if (stats & STATSBONUS_INT) {
        screenMessage("Intelligence + 5\n");
        AdjustValue(c->saveGame->players[player].intel, 5, 50);        
        damage += 200;
    }   
    
    /* deal damage to the party member who touched the orb */
    playerApplyDamage(&c->saveGame->players[player], damage);
    /* remove the orb from the map */
    annotationAdd(x, y, z, c->location->map->id, replacementTile);

    statsUpdate();
    return 1;
}

/**
 * Returns true if a ladder-up is found at the given coordinates
 */
int dungeonLadderUpAt(struct _Map *map, int x, int y, int z) {
    if (dungeonTokenAt(map, x, y, z) == DUNGEON_LADDER_UP ||
        dungeonTokenAt(map, x, y, z) == DUNGEON_LADDER_UPDOWN)
        return 1;
    return 0;
}

/**
 * Returns true if a ladder-down is found at the given coordinates
 */
int dungeonLadderDownAt(struct _Map *map, int x, int y, int z) {
    if (dungeonTokenAt(map, x, y, z) == DUNGEON_LADDER_DOWN ||
        dungeonTokenAt(map, x, y, z) == DUNGEON_LADDER_UPDOWN)
        return 1;
    return 0;
}
