/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "combat.h"
#include "ttype.h"
#include "map.h"

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
