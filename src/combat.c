/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "combat.h"
#include "context.h"
#include "ttype.h"
#include "map.h"
#include "object.h"
#include "annotation.h"
#include "event.h"
#include "savegame.h"
#include "game.h"
#include "area.h"
#include "monster.h"

extern Map brick_map;
extern Map bridge_map;
extern Map brush_map;
extern Map camp_map;
extern Map dng0_map;
extern Map dng1_map;
extern Map dng2_map;
extern Map dng3_map;
extern Map dng4_map;
extern Map dng5_map;
extern Map dng6_map;
extern Map dungeon_map;
extern Map forest_map;
extern Map grass_map;
extern Map hill_map;
extern Map inn_map;
extern Map marsh_map;
extern Map shipsea_map;
extern Map shipship_map;
extern Map shipshor_map;
extern Map shore_map;
extern Map shorship_map;

Object *party[8];
Object *monsters[16];

int combatHandleChoice(char choice);
void combatEnd(void);

void combatBegin(unsigned char partytile, unsigned short transport) {
    int i;
    GetChoiceActionInfo *info;
    int nmonsters;

    annotationClear();

    c = gameCloneContext(c);

    gameSetMap(c, getCombatMapForTile(partytile, transport), 1);
    musicPlay();

    for (i = 0; i < c->saveGame->members; i++)
        party[i] = mapAddObject(c->map, tileForClass(c->saveGame->players[i].klass), tileForClass(c->saveGame->players[i].klass), c->map->area->player_start[i].x, c->map->area->player_start[i].y);
    for (; i < 8; i++)
        party[i] = NULL;
    party[0]->hasFocus = 1;

    nmonsters = (rand() % 15) + 1;
    for (i = 0; i < nmonsters; i++)
        monsters[i] = mapAddObject(c->map, ORC_TILE, ORC_TILE + 1, c->map->area->monster_start[i].x, c->map->area->monster_start[i].y);
    for (; i < 16; i++)
        monsters[i] = NULL;

    info = (GetChoiceActionInfo *) malloc(sizeof(GetChoiceActionInfo));
    info->choices = " \033";
    info->handleChoice = &combatHandleChoice;
    eventHandlerPushKeyHandlerData(&keyHandlerGetChoice, info);
}


Map *getCombatMapForTile(unsigned char partytile, unsigned short transport) {
    int i;
    static const struct {
        unsigned char tile;
        Map *map;
    } tileToMap[] = {
        { SWAMP_TILE, &marsh_map },
        { GRASS_TILE, &grass_map },
        { BRUSH_TILE, &brush_map },
        { FOREST_TILE, &forest_map },
        { HILLS_TILE, &hill_map },
        { BRIDGE_TILE, &bridge_map },
        { NORTHBRIDGE_TILE, &bridge_map },
        { SOUTHBRIDGE_TILE, &bridge_map },
        { CHEST_TILE, &brick_map },
        { BRICKFLOOR_TILE, &brick_map },
        { MOONGATE0_TILE, &grass_map },
        { MOONGATE1_TILE, &grass_map },
        { MOONGATE2_TILE, &grass_map },
        { MOONGATE3_TILE, &grass_map }
    };
    
    for (i = 0; i < sizeof(tileToMap) / sizeof(tileToMap[0]); i++) {
        if (tileToMap[i].tile == partytile)
            return tileToMap[i].map;
    }

    return &brick_map;
}

int combatHandleChoice(char choice) {
    eventHandlerPopKeyHandler();

    combatEnd();

    return 1;
}

void combatEnd() {
    if (c->parent != NULL) {
        Context *t = c;
        annotationClear();
        mapClearObjects(c->map);
        c->parent->saveGame->x = c->saveGame->dngx;
        c->parent->saveGame->y = c->saveGame->dngy;
        c->parent->line = c->line;
        c->parent->moonPhase = c->moonPhase;
        c->parent->windDirection = c->windDirection;
        c->parent->windCounter = c->windCounter;
        c = c->parent;
        c->col = 0;
        free(t);
                
        musicPlay();
    }
    
    gameFinishTurn();
}
