/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <string>
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
    const static std::string tileNames[] = {
        "brick_floor", "up_ladder", "down_ladder", "up_down_ladder", "chest",
        "unimpl_ceiling_hole", "unimpl_floor_hole", "magic_orb", 
        "ceiling_hole", "fountain", 
        "brick_floor", "dungeon_altar", "dungeon_door", "dungeon_room",
        "secret_door", "brick_wall", ""
    };

    const static std::string fieldNames[] = { "poison_field", "energy_field", "fire_field", "sleep_field", "" };

    int i;
    Tile *t = c->location->map->tileset->get(tile.id);

    for (i = 0; !tileNames[i].empty(); i++) {        
        if (strcasecmp(t->name.c_str(), tileNames[i].c_str()) == 0)
            return DungeonToken(i<<4);
    }

    for (i = 0; !fieldNames[i].empty(); i++) {        
        if (strcasecmp(t->name.c_str(), fieldNames[i].c_str()) == 0)
            return DUNGEON_FIELD;
    }

    return (DungeonToken)0;
}

/**
 * Return the dungeon sub-token associated with the given dungeon tile.
 * For instance, for tile 0x91, returns FOUNTAIN_HEALING
 * NOTE: This function will always need type-casting to the token type necessary
 */
unsigned char dungeonSubTokenForTile(MapTile tile) {    
    return (tile.type);
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
    return dungeonTokenForTile(*map->getTileFromData(coords));
}

/**
 * Returns the dungeon sub-token for the given coordinates
 */
unsigned char dungeonSubTokenAt(Map *map, MapCoords coords) {
    return dungeonSubTokenForTile(*map->getTileFromData(coords));
}

/**
 * Handles 's'earching while in dungeons
 */
void dungeonSearch(void) {
    DungeonToken token = dungeonCurrentToken(); 
    Annotation::List a = c->location->map->annotations->allAt(c->location->coords);
    const ItemLocation *item;
    if (a.size() > 0)
        token = DUNGEON_CORRIDOR;

    screenMessage("Search...\n");

    switch (token) {
    case DUNGEON_MAGIC_ORB: /* magic orb */
    screenMessage("You find a Magical Ball...\nWho touches? ");
        dungeonTouchOrb();
        break;

    case DUNGEON_FOUNTAIN: /* fountains */
        dungeonDrinkFountain();
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
void dungeonDrinkFountain() {
    screenMessage("You find a Fountain.\nWho drinks? ");
    int player = gameGetPlayer(false, false);
    if (player == -1)
        return;

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
    }
}

/**
 * Touch the magical ball at the current location
 */
void dungeonTouchOrb() {
    screenMessage("You find a Magical Ball...\nWho touches? ");
    int player = gameGetPlayer(false, false);
    if (player == -1)
        return;

    int stats = 0;
    int damage = 0;    
    MapTile replacementTile;
    
    /* Get current position and find a replacement tile for it */   
    replacementTile = c->location->getReplacementTile(c->location->coords);

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
}

/**
 * Handles dungeon traps
 */
bool dungeonHandleTrap(TrapType trap) {
    switch((TrapType)dungeonCurrentSubToken()) {
    case TRAP_WINDS:
        screenMessage("\nWinds!\n");
        c->party->quenchTorch();
        break;
    case TRAP_FALLING_ROCK:
        // Treat falling rocks and pits like bomb traps
        // XXX: That's a little harsh.
        screenMessage("\nFalling Rocks!\n");
        c->party->applyEffect(EFFECT_LAVA);
        break;
    case TRAP_PIT:
        screenMessage("\nPit!\n");
        c->party->applyEffect(EFFECT_LAVA);
        break;
    default: break;
    }

    return true;
}

/**
 * Returns true if a ladder-up is found at the given coordinates
 */
bool dungeonLadderUpAt(class Map *map, MapCoords coords) {    
    Annotation::List a = c->location->map->annotations->allAt(coords);

    if (dungeonTokenAt(map, coords) == DUNGEON_LADDER_UP ||
        dungeonTokenAt(map, coords) == DUNGEON_LADDER_UPDOWN)
        return true;

    if (a.size() > 0) {
        Annotation::List::iterator i;
        for (i = a.begin(); i != a.end(); i++) {
            if (i->getTile() == Tileset::findTileByName("up_ladder")->id)
                return true;
        }
    }
    return false;
}

/**
 * Returns true if a ladder-down is found at the given coordinates
 */
bool dungeonLadderDownAt(class Map *map, MapCoords coords) {
    Annotation::List a = c->location->map->annotations->allAt(coords);

    if (dungeonTokenAt(map, coords) == DUNGEON_LADDER_DOWN ||
        dungeonTokenAt(map, coords) == DUNGEON_LADDER_UPDOWN)
        return true;

    if (a.size() > 0) {
        Annotation::List::iterator i;
        for (i = a.begin(); i != a.end(); i++) {
            if (i->getTile() == Tileset::findTileByName("down_ladder")->id)
                return true;
        }
    }
    return false;
}
