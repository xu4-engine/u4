/*
 * $Id$
 */

#include "dungeon.h"

#include "annotation.h"
#include "context.h"
#include "debug.h"
#include "game.h"
#include "location.h"
#include "mapmgr.h"
#include "player.h"
#include "screen.h"
#include "stats.h"

/**
 * Handles 's'earching while in dungeons
 */
void dungeonSearch(void) {
    unsigned char dngTile = mapGetTileFromData(c->location->map, c->location->x, c->location->y, c->location->z);
    const Annotation *a = annotationAt(c->location->x, c->location->y, c->location->z, c->location->map->id);
    if (a) 
        dngTile = 0;

    switch (dngTile & 0xF0) {
    case 0x70: /* magic orb */
        screenMessage("You find a Magical Ball...\nWho touches? ");
        gameGetPlayerForCommand(&dungeonTouchOrb);
        break;

    case 0x90: /* fountains */
        screenMessage("You find a Fountain.\nWho drinks? ");
        gameGetPlayerForCommand(&dungeonDrinkFountain);
        break;

    default: 
        screenMessage("Nothing Here!\n");
        break;
    }
}

/**
 * Drink from the fountain at the current location
 */
int dungeonDrinkFountain(int player) {
    int retval = 1;
    unsigned char dngTile = mapGetTileFromData(c->location->map, c->location->x, c->location->y, c->location->z);

    switch(dngTile & 0xF) {
    case 0: /* plain fountain */
        screenMessage("\nHmmm--No Effect!\n");
        break;
    case 1: /* healing fountain */
        if (playerHeal(c->saveGame, HT_FULLHEAL, player))
            screenMessage("\nAhh-Refreshing!\n");
        else screenMessage("\nHmmm--No Effect!\n");
        break;
    case 2: /* acid fountain */
        playerApplyDamage(&c->saveGame->players[player], 100); /* 100 damage to drinker */
        screenMessage("\nBleck--Nasty!\n");
        break;
    case 3: /* cure fountain */
        if (playerHeal(c->saveGame, HT_CURE, player))
            screenMessage("\nHmmm--Delicious!\n");
        else screenMessage("\nHmmm--No Effect!\n");
        break;
    case 4: /* poison fountain */
        if (c->saveGame->players[player].status != STAT_POISONED) {
            playerApplyEffect(c->saveGame, EFFECT_POISON, player);
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
        c->saveGame->players[player].str += 5;
        damage += 200;
    }
    if (stats & STATSBONUS_DEX) {
        screenMessage("Dexterity + 5\n");
        c->saveGame->players[player].dex += 5;
        damage += 200;
    }
    if (stats & STATSBONUS_INT) {
        screenMessage("Intelligence + 5\n");
        c->saveGame->players[player].intel += 5;
        damage += 200;
    }   
    
    /* deal damage to the party member who touched the orb */
    playerApplyDamage(&c->saveGame->players[player], damage);
    /* remove the orb from the map */
    annotationAdd(c->location->x, c->location->y, c->location->z, c->location->map->id, BRICKFLOOR_TILE);

    statsUpdate();
    return 1;
}
