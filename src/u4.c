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
#include "screen.h"
#include "event.h"
#include "person.h"
#include "game.h"
#include "intro.h"
#include "error.h"
#include "music.h"

int verbose = 0;
int quit = 0;

int main(int argc, char *argv[]) {
    unsigned int i;
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
            settings->germanKbd = 1;
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

    /* play the game! */
    gameInit();

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
