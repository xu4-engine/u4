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
