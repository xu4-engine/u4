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

const extern Map world_map;
Context *c;

int main(int argc, char *argv[]) {
    FILE *saveGameFile;

    screenInit();

    c = (Context *) malloc(sizeof(Context));
    c->saveGame = (SaveGame *) malloc(sizeof(SaveGame));
    c->parent = NULL;
    c->map = &world_map;
    c->talker = NULL;
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
