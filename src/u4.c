/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <SDL.h>

#include "u4.h"
#include "mapinit.h"
#include "screen.h"
#include "event.h"
#include "map.h"
#include "person.h"
#include "game.h"
#include "intro.h"
#include "context.h"
#include "savegame.h"
#include "stats.h"

Context *c;

extern Map world_map;
int verbose = 0;

int main(int argc, char *argv[]) {
    unsigned int i, scale = 2;
    FILE *saveGameFile;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-scale") == 0 && argc > i + 1) {
            scale = strtoul(argv[i+1], NULL, 0);
            if (scale < 1 || scale > 5)
                scale = 2;
        }
        if (strcmp(argv[i], "-v") == 0)
            verbose++;
    }

    srand(time(NULL));

    screenInit(scale);

    eventHandlerInit();

    /* do the intro */
    introInit();
    introUpdateScreen();
    eventHandlerAddTimerCallback(&introTimer);
    eventHandlerPushKeyHandler(&introKeyHandler);
    eventHandlerMain();
    eventHandlerRemoveTimerCallback(&introTimer);
    eventHandlerPopKeyHandler();
    introDelete();

    /* load in the maps */
    if (!initializeMaps()) {
        fprintf(stderr, "error initializing maps: is Ultima 4 for DOS installed?\n");
        exit(1);
    }

    /* initialize person data */
    if (!personInit()) {
        fprintf(stderr, "error initializing person data: is Ultima 4 for DOS installed?\n");
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

    /* load in the save game */
    saveGameFile = fopen("party.sav", "rb");
    if (saveGameFile) {
        saveGameRead(c->saveGame, saveGameFile);
		fclose(saveGameFile);
    } else {
        printf("No savegame found!  Initiate game first\n");
        exit(0);
    }

    /* play the game! */
    screenDrawBackground(BKGD_BORDERS);
    screenUpdate();
    statsUpdate();
    screenMessage("\020");
    screenForceRedraw();

    eventHandlerAddTimerCallback(&gameTimer);
    eventHandlerPushKeyHandler(&gameBaseKeyHandler);
    eventHandlerMain();
    eventHandlerRemoveTimerCallback(&gameTimer);
    eventHandlerPopKeyHandler();

    return 0;
}
