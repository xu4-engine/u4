/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "u4.h"
#include "mapinit.h"
#include "screen.h"
#include "event.h"
#include "map.h"
#include "person.h"
#include "ttype.h"
#include "context.h"
#include "savegame.h"
#include "stats.h"

Context *c;

extern Map world_map;

int main(int argc, char *argv[]) {
    unsigned int i, scale = 2;
    FILE *saveGameFile;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-scale") == 0 && argc > i + 1) {
            scale = strtoul(argv[i+1], NULL, 0);
            if (scale < 1 || scale > 5)
                scale = 2;
        }
    }

    screenInit(scale);

    if (!initializeMaps()) {
        fprintf(stderr, "error initializing maps: is Ultima 4 for DOS installed?\n");
        exit(1);
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
    c->statsItem = STATS_PARTY_OVERVIEW;
    c->moonPhase = 0;

    saveGameFile = fopen("party.sav", "rb");
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
    statsUpdate();
    screenForceRedraw();

    eventHandlerMain();

    return 0;
}
