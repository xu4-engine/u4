/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

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
#include "tileset.h"
#include "utils.h"

/**
 * Returns true if 'map' points to a dungeon map
 */ 
bool isDungeon(Map *punknown) {
    Dungeon *pd;
    if ((pd = dynamic_cast<Dungeon*>(punknown)) != NULL)
        return true;
    else
        return false;
}

/**
 * Returns the name of the dungeon
 */ 
string Dungeon::getName() {
    return name;
}

/**
 * Returns the dungeon token associated with the given dungeon tile
 */
DungeonToken dungeonTokenForTile(MapTile tile) {
    return (DungeonToken)(tile & 0xF0);
}

/**
 * Return the dungeon sub-token associated with the given dungeon tile.
 * For instance, for tile 0x91, returns FOUNTAIN_HEALING
 * NOTE: This function will always need type-casting to the token type necessary
 */
unsigned char dungeonSubTokenForTile(MapTile tile) {
    return (tile & 0xF);
}

/**
 * Returns the dungeon token for the current location
 */
DungeonToken dungeonCurrentToken() {
    return dungeonTokenAt(c->location->map, c->location->coords);
}

/**
 * Returns the dungeon sub-token for the current location
 */
unsigned char dungeonCurrentSubToken() {
    return dungeonSubTokenAt(c->location->map, c->location->coords);
}

/**
 * Returns the dungeon token for the given coordinates
 */
DungeonToken dungeonTokenAt(Map *map, MapCoords coords) {
    return dungeonTokenForTile(map->getTileFromData(coords));
}

/**
 * Returns the dungeon sub-token for the given coordinates
 */
unsigned char dungeonSubTokenAt(Map *map, MapCoords coords) {
    return dungeonSubTokenForTile(map->getTileFromData(coords));
}

/**
 * Loads a dungeon room into map->dungeon->room
 */
bool dungeonLoadRoom(Dungeon *dng, int room) {
    if (dng->room != NULL)
        delete dng->room;    
    
    dng->room = getCombatMap(mapMgrInitMap(MAPTYPE_COMBAT));

    dng->room->id = 0;
    dng->room->border_behavior = BORDER_FIXED;
    dng->room->width = dng->room->height = 11;

    for (unsigned int y = 0; y < dng->room->height; y++) {
        for (unsigned int x = 0; x < dng->room->width; x++) {
            dng->room->data.push_back(MapTile(dng->rooms[room].map_data[x + (y * dng->room->width)]));
        }
    }
    
    dng->room->music = MUSIC_COMBAT;
    dng->room->type = MAPTYPE_COMBAT;
    dng->room->flags |= NO_LINE_OF_SIGHT;

    dng->currentRoom = (dng->rooms + room);
    return true;
}

/**
 * Handles 's'earching while in dungeons
 */
void dungeonSearch(void) {
    DungeonToken token = dungeonCurrentToken(); 
    AnnotationList a = c->location->map->annotations->allAt(c->location->coords);
    const ItemLocation *item;
    if (a.size() > 0)
        token = DUNGEON_CORRIDOR;

    screenMessage("Search...\n");

    switch (token) {
    case DUNGEON_MAGIC_ORB: /* magic orb */
        screenMessage("You find a Magical Ball...\nWho touches? ");
        gameGetPlayerForCommand(&dungeonTouchOrb, 0, 0);
        break;

    case DUNGEON_FOUNTAIN: /* fountains */
        screenMessage("You find a Fountain.\nWho drinks? ");
        gameGetPlayerForCommand(&dungeonDrinkFountain, 0, 0);
        break;

    default: 
        {
            /* see if there is an item at the current location (stones on altars, etc.) */
            item = itemAtLocation(c->location->map, c->location->coords);
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
bool dungeonDrinkFountain(int player) {
    bool retval = true;
    FountainType type = (FountainType)dungeonCurrentSubToken();    

    switch(type) {
    /* plain fountain */
    case FOUNTAIN_NORMAL: 
        screenMessage("\nHmmm--No Effect!\n");
        break;

    /* healing fountain */
    case FOUNTAIN_HEALING: 
        if (c->party->member(player)->heal(HT_FULLHEAL))
            screenMessage("\nAhh-Refreshing!\n");
        else screenMessage("\nHmmm--No Effect!\n");
        break;
    
    /* acid fountain */
    case FOUNTAIN_ACID:
        c->party->member(player)->applyDamage(100); /* 100 damage to drinker */        
        screenMessage("\nBleck--Nasty!\n");
        break;

    /* cure fountain */
    case FOUNTAIN_CURE:
        if (c->party->member(player)->heal(HT_CURE))        
            screenMessage("\nHmmm--Delicious!\n");
        else screenMessage("\nHmmm--No Effect!\n");
        break;

    /* poison fountain */
    case FOUNTAIN_POISON: 
        if (c->party->member(player)->getStatus() != STAT_POISONED) {
            c->party->member(player)->applyEffect(EFFECT_POISON);
            c->party->member(player)->applyDamage(100); /* 100 damage to drinker also */            
            screenMessage("\nArgh-Choke-Gasp!\n");
        }
        else screenMessage("\nHmm--No Effect!\n");
        break;

    default:
        ASSERT(0, "Invalid call to dungeonDrinkFountain: no fountain at current location");
        retval = false;
    }

    return retval;
}

/**
 * Touch the magical ball at the current location
 */
bool dungeonTouchOrb(int player) {
    int stats = 0;
    int damage = 0;    
    MapTile replacementTile;
    
    /* Get current position and find a replacement tile for it */   
    replacementTile = locationGetReplacementTile(c->location, c->location->coords);

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
        AdjustValueMax(c->saveGame->players[player].str, 5, 50);
        damage += 200;
    }
    if (stats & STATSBONUS_DEX) {
        screenMessage("Dexterity + 5\n");
        AdjustValueMax(c->saveGame->players[player].dex, 5, 50);        
        damage += 200;
    }
    if (stats & STATSBONUS_INT) {
        screenMessage("Intelligence + 5\n");
        AdjustValueMax(c->saveGame->players[player].intel, 5, 50);        
        damage += 200;
    }   
    
    /* deal damage to the party member who touched the orb */
    c->party->member(player)->applyDamage(damage);    
    /* remove the orb from the map */
    c->location->map->annotations->add(c->location->coords, replacementTile);

    return true;
}

/**
 * Handles dungeon traps
 */
bool dungeonHandleTrap(TrapType trap) {
    switch((TrapType)dungeonCurrentSubToken()) {
    case TRAP_WINDS:
        screenMessage("\nWinds!\n");
        c->saveGame->torchduration = 0;
        break;
    case TRAP_FALLING_ROCK:
        /* FIXME: implement */
    case TRAP_PIT:
        /* FIXME: implement */
    default: break;
    }

    return true;
}

/**
 * Returns true if a ladder-up is found at the given coordinates
 */
bool dungeonLadderUpAt(class Map *map, MapCoords coords) {    
    AnnotationList a = c->location->map->annotations->allAt(coords);

    if (dungeonTokenAt(map, coords) == DUNGEON_LADDER_UP ||
        dungeonTokenAt(map, coords) == DUNGEON_LADDER_UPDOWN)
        return true;

    if (a.size() > 0) {
        AnnotationList::iterator i;
        for (i = a.begin(); i != a.end(); i++) {
            if (i->getTile() == LADDERUP_TILE)            
                return true;
        }
    }
    return false;
}

/**
 * Returns true if a ladder-down is found at the given coordinates
 */
bool dungeonLadderDownAt(class Map *map, MapCoords coords) {
    AnnotationList a = c->location->map->annotations->allAt(coords);

    if (dungeonTokenAt(map, coords) == DUNGEON_LADDER_DOWN ||
        dungeonTokenAt(map, coords) == DUNGEON_LADDER_UPDOWN)
        return true;

    if (a.size() > 0) {
        AnnotationList::iterator i;
        for (i = a.begin(); i != a.end(); i++) {
            if (i->getTile() == LADDERDOWN_TILE)
                return true;
        }
    }
    return false;
}
