/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "u4.h"
#include "u4file.h"
#include "map.h"
#include "mapinit.h"

extern Map world_map;
extern Map britain_map;
extern Map yew_map;
extern Map minoc_map;
extern Map trinsic_map;
extern Map jhelom_map;
extern Map skara_map;
extern Map magincia_map;
extern Map moonglow_map;
extern Map paws_map;
extern Map vesper_map;
extern Map den_map;
extern Map cove_map;

extern Map empath_map;
extern Map lycaeum_map;
extern Map serpent_map;
extern Map lcb_1_map;
extern Map lcb_2_map;

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
extern Map shrine_map;

Map *cities[] = {
    &britain_map, &yew_map,
    &minoc_map, &trinsic_map,
    &jhelom_map, &skara_map,
    &magincia_map, &moonglow_map,
    &paws_map, &vesper_map,
    &den_map, &cove_map,
    &empath_map, &lycaeum_map,
    &serpent_map, &lcb_1_map,
    &lcb_2_map
};

Map *areas[] = {
    &brick_map, &bridge_map,
    &brush_map, &camp_map,
    &dng0_map, &dng1_map,
    &dng2_map, &dng3_map,
    &dng4_map, &dng5_map,
    &dng6_map, &dungeon_map,
    &forest_map, &grass_map,
    &hill_map, &inn_map,
    &marsh_map, &shipsea_map,
    &shipship_map, &shipshor_map,
    &shore_map, &shorship_map,
    &shrine_map
};

int initializeMaps() {
    unsigned int i;
    FILE *world;

    world = u4fopen("world.map");
    if (!world)
        return 0;
    mapReadWorld(&world_map, world);
    u4fclose(world);

    for (i = 0; i < sizeof(cities) / sizeof(cities[0]); i++) {
        FILE *ult, *tlk;
        ult = u4fopen(cities[i]->ult_fname);
        tlk = u4fopen(cities[i]->tlk_fname);
        if (!ult || !tlk)
            return 0;
        mapRead(cities[i], ult, tlk);
        u4fclose(ult);
        u4fclose(tlk);
    }

    for (i = 0; i < sizeof(areas) / sizeof(areas[0]); i++) {
        FILE *con;
        con = u4fopen(areas[i]->ult_fname);
        if (!con)
            return 0;
        mapReadCon(areas[i], con, 1);
        u4fclose(con);
    }

    return 1;
}
