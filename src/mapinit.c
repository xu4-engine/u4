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
#include "city.h"
#include "mapinit.h"

extern Map world_map;

extern City lcb_1_city;
extern City lcb_2_city;
extern City lycaeum_city;
extern City empath_city;
extern City serpent_city;

extern City moonglow_city;
extern City britain_city;
extern City jhelom_city;
extern City yew_city;
extern City minoc_city;
extern City trinsic_city;
extern City skara_city;
extern City magincia_city;
extern City paws_city;
extern City den_city;
extern City vesper_city;
extern City cove_city;

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

City * const cities[] = {
    &lcb_1_city, &lcb_2_city,
    &lycaeum_city, &empath_city,
    &serpent_city,
    &moonglow_city, &britain_city,
    &jhelom_city, &yew_city,
    &minoc_city, &trinsic_city,
    &skara_city, &magincia_city,
    &paws_city, &den_city,
    &vesper_city, &cove_city
};

Map * const areas[] = {
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
        ult = u4fopen(cities[i]->map->fname);
        tlk = u4fopen(cities[i]->tlk_fname);
        if (!ult || !tlk)
            return 0;
        mapRead(cities[i], ult, tlk);
        u4fclose(ult);
        u4fclose(tlk);
    }

    for (i = 0; i < sizeof(areas) / sizeof(areas[0]); i++) {
        FILE *con;
        con = u4fopen(areas[i]->fname);
        if (!con)
            return 0;
        mapReadCon(areas[i], con);
        u4fclose(con);
    }

    return 1;
}
