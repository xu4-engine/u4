/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "u4.h"
#include "screen.h"
#include "event.h"
#include "map.h"
#include "person.h"
#include "ttype.h"
#include "context.h"
#include "savegame.h"

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

Context *c;

int main(int argc, char *argv[]) {
    int i;
    FILE *world, *saveGameFile;

    screenInit();

    world = fopen("./ultima4/world.map", "r");
    mapReadWorld(&world_map, world);
    fclose(world);

    for (i = 0; i < sizeof(cities) / sizeof(cities[0]); i++) {
        FILE *ult, *tlk;
        ult = fopen(cities[i]->ult_fname, "r");
        tlk = fopen(cities[i]->tlk_fname, "r");
        mapRead(cities[i], ult, tlk);
        fclose(ult);
        fclose(tlk);
    }

    for (i = 0; i < sizeof(areas) / sizeof(areas[0]); i++) {
        FILE *con;
        con = fopen(areas[i]->ult_fname, "r");
        mapReadCon(areas[i], con, 1);
        fclose(con);
    }

    c = (Context *) malloc(sizeof(Context));
    c->saveGame = (SaveGame *) malloc(sizeof(SaveGame));
    c->parent = NULL;
    c->map = &world_map;
    c->annotation = NULL;
    c->conversation.talker = NULL;
    c->conversation.question = 0;
    c->conversation.buffer[0] = '\0';
    c->line = 0;
    c->col = 0;

    saveGameFile = fopen("party.sav", "r");
    if (saveGameFile) {
        saveGameRead(c->saveGame, saveGameFile);
		fclose(saveGameFile);
    } else {
        SaveGamePlayerRecord avatar;
        saveGamePlayerRecordInit(&avatar);
        strcpy(avatar.name, "Avatar");
        avatar.hp = 100;
        avatar.hpMax = 100;
        avatar.str = 20;
        avatar.dex = 20;
        avatar.intel = 20;
        saveGameInit(c->saveGame, 86, 109, &avatar);
    }

    screenDrawBorders();
    screenUpdate(c);
    screenForceRedraw();

    eventHandlerMain();

    return 0;
}
