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
#include "settings.h"
#include "mapinit.h"
#include "direction.h"
#include "screen.h"
#include "event.h"
#include "map.h"
#include "person.h"
#include "game.h"
#include "intro.h"
#include "context.h"
#include "savegame.h"
#include "stats.h"
#include "error.h"

Context *c;

extern Map world_map;
int verbose = 0;
int quit = 0;
int germanKbd = 0;

int main(int argc, char *argv[]) {
    unsigned int i;
    FILE *saveGameFile, *monstersFile;
    int skipIntro = 0;

    settingsRead();

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-filter") == 0 && argc > i + 1) {
            settings->filter = settingsStringToFilter(argv[i+1]);
            if (settings->filter == SCL_MAX)
                errorFatal("%s is not a valid filter", argv[i+1]);
            i++;
        }
        if (strcmp(argv[i], "-scale") == 0 && argc > i + 1) {
            settings->scale = strtoul(argv[i+1], NULL, 0);
            i++;
        }
        else if (strcmp(argv[i], "-i") == 0)
            skipIntro = 1;
        else if (strcmp(argv[i], "-g") == 0)
            germanKbd = 1;
        else if (strcmp(argv[i], "-v") == 0)
            verbose++;
        else if (strcmp(argv[i], "-f") == 0)
            settings->fullscreen = 1;
        else if (strcmp(argv[i], "-q") == 0)
            settings->vol = 0;
    }

    srand(time(NULL));

    screenInit();
    musicInit();
    eventHandlerInit();

    if (!skipIntro) {
        /* do the intro */
        introInit();
        musicIntro();
        eventHandlerAddTimerCallback(&introTimer, 1);
        eventHandlerPushKeyHandler(&introKeyHandler);
        eventHandlerMain(NULL);
        eventHandlerRemoveTimerCallback(&introTimer);
        eventHandlerPopKeyHandler();
        introDelete();
    }

    if (quit)
        return 0;

    /* load in the maps */
    if (!initializeMaps())
        errorFatal("unable to load map files: is Ultima IV installed?  See http://xu4.sourceforge.net/");

    /* initialize person data */
    if (!personInit())
        errorFatal("unable to load person data files: is Ultima IV installed?  See http://xu4.sourceforge.net/");

    c = (Context *) malloc(sizeof(Context));
    c->saveGame = (SaveGame *) malloc(sizeof(SaveGame));
    c->parent = NULL;
    c->map = &world_map;
    c->map->annotation = NULL;
    c->conversation.talker = NULL;
    c->conversation.state = 0;
    c->conversation.buffer[0] = '\0';
    c->line = TEXT_AREA_H - 1;
    c->col = 0;
    c->statsItem = STATS_PARTY_OVERVIEW;
    c->moonPhase = 0;
    c->windDirection = DIR_NORTH;
    c->windCounter = 0;
    c->aura = AURA_NONE;
    c->auraDuration = 0;
    c->horseSpeed = 0;

    /* load in the save game */
    saveGameFile = fopen("party.sav", "rb");
    if (saveGameFile) {
        saveGameRead(c->saveGame, saveGameFile);
        fclose(saveGameFile);
    } else
        errorFatal("no savegame found!");

    monstersFile = fopen("monsters.sav", "rb");
    if (monstersFile) {
        saveGameMonstersRead(&c->map->objects, monstersFile);
        fclose(monstersFile);
    }

    /* play the game! */
    musicPlay();
    screenDrawBackground(BKGD_BORDERS);
    statsUpdate();
    screenMessage("\020");

    eventHandlerAddTimerCallback(&gameTimer, 1);
    eventHandlerPushKeyHandler(&gameBaseKeyHandler);
    eventHandlerMain(&gameUpdateScreen);

    eventHandlerRemoveTimerCallback(&gameTimer);
    eventHandlerPopKeyHandler();

    eventHandlerDelete();
    musicDelete();
    screenDelete();

    return 0;
}
