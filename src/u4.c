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

#include "error.h"
#include "event.h"
#include "game.h"
#include "intro.h"
#include "mapmgr.h"
#include "music.h"
#include "person.h"
#include "screen.h"
#include "settings.h"
#include "sound.h"

#if defined(MACOSX)
#include "macosx/osxinit.h"
#endif

int verbose = 0;
int quit = 0;

int main(int argc, char *argv[]) {
    unsigned int i;
    int skipIntro = 0;

#if defined(MACOSX)
    osxInit(argv[0]);
#endif

    settingsRead();

    for (i = 1; i < (unsigned int)argc; i++) {
        if (strcmp(argv[i], "-filter") == 0 && (unsigned int)argc > i + 1) {
            settings->filter = settingsStringToFilter(argv[i+1]);
            if (settings->filter == SCL_MAX)
                errorFatal("%s is not a valid filter", argv[i+1]);
            i++;
        }
        if (strcmp(argv[i], "-scale") == 0 && (unsigned int)argc > i + 1) {
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
    soundInit();
    eventHandlerInit();

    if (!skipIntro) {
        /* do the intro */
        introInit();
        eventHandlerAddTimerCallback(&introTimer, eventTimerGranularity);
        eventHandlerPushKeyHandler(&introKeyHandler);
        eventHandlerMain(NULL);
        eventHandlerRemoveTimerCallback(&introTimer);
        eventHandlerPopKeyHandler();
        introDelete();
    }

    if (quit)
        return 0;

    /* load in the maps */
    mapMgrInit();

    /* initialize person data */
    if (!personInit())
        errorFatal("unable to load person data files: is Ultima IV installed?  See http://xu4.sourceforge.net/");

    /* play the game! */
    gameInit();

    eventHandlerAddTimerCallback(&gameTimer, eventTimerGranularity);
    eventHandlerPushKeyHandler(&gameBaseKeyHandler);
    eventHandlerMain(&gameUpdateScreen);

    /* main event handler returned - cleanup and exit! */
    eventHandlerRemoveTimerCallback(&gameTimer);
    eventHandlerPopKeyHandler();

    gameCleanup();

    eventHandlerDelete();
    soundDelete();
    musicDelete();
    screenDelete();    

    return 0;
}
