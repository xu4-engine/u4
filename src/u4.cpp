/*
 * $Id$
 */
 
#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

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
#include "tileset.h"
#include "utils.h"

#if defined(MACOSX)
#include "macosx/osxinit.h"
#endif

bool verbose = false;
bool quit = false;

Performance perf("performance.txt");

using namespace std;

int main(int argc, char *argv[]) {
    unsigned int i;
    int skipIntro = 0;

#if defined(MACOSX)
    osxInit(argv[0]);
#endif

    /* Read the game settings from file */    
    settings.read();    

    for (i = 1; i < (unsigned int)argc; i++) {
        if (strcmp(argv[i], "-filter") == 0 && (unsigned int)argc > i + 1) {
            settings.filter = (FilterType)Settings::filters.getType(argv[i+1]);
            if (settings.filter == -1)
                errorFatal("%s is not a valid filter", argv[i+1]);
            i++;
        }
        if (strcmp(argv[i], "-scale") == 0 && (unsigned int)argc > i + 1) {
            settings.scale = strtoul(argv[i+1], NULL, 0);
            i++;
        }
        else if (strcmp(argv[i], "-i") == 0)
            skipIntro = 1;
        else if (strcmp(argv[i], "-v") == 0)
            verbose = true;
        else if (strcmp(argv[i], "-f") == 0)
            settings.fullscreen = 1;
        else if (strcmp(argv[i], "-q") == 0) {
            settings.musicVol = 0;
            settings.soundVol = 0;
        }
    }

    xu4_srandom();    

    perf.start();    
    screenInit();
    screenTextAt(15, 12, "Loading...");
    screenRedrawScreen();
    perf.end("Screen Initialization");

    perf.start();
    musicInit();
    soundInit();
    eventHandlerInit();    
    perf.end("Misc Initialization");

    perf.start();
    Tileset::loadAll("tilesets.xml");
    perf.end("Tileset::loadAll()");

    perf.start();
    creatures.loadInfoFromXml();
    perf.end("CreatureMgr::loadInfoFromXml()");

    if (!skipIntro) {
        /* do the intro */
        perf.start();
        introInit();
        perf.end("introInit()");
        
        /* give a performance report */
        if (settings.debug)
            perf.report();

        eventHandlerAddTimerCallback(&introTimer, eventTimerGranularity);
        eventHandlerPushKeyHandler(&introKeyHandler);
        eventHandlerMain(NULL);
        eventHandlerRemoveTimerCallback(&introTimer);
        eventHandlerPopKeyHandler();
        introDelete(FREE_MENUS);
    }

    if (quit)
        return 0;

    /* load in the maps */
    perf.reset();
    perf.start();
    mapMgrInit();
    perf.end("mapMgrInit()");

    /* initialize person data */
    perf.start();
    if (!personInit())
        errorFatal("unable to load person data files: is Ultima IV installed?  See http://xu4.sourceforge.net/");
    perf.end("personInit()");

    /* play the game! */
    perf.start();
    gameInit();
    perf.end("gameInit()");
    
    /* give a performance report */
    if (settings.debug)
        perf.report("\n===============================\n\n");

    eventHandlerAddTimerCallback(&gameTimer, eventTimerGranularity);
    eventHandlerPushKeyHandler(&gameBaseKeyHandler);
    eventHandlerMain(&gameUpdateScreen);

    /* main event handler returned - cleanup and exit! */
    eventHandlerRemoveTimerCallback(&gameTimer);
    eventHandlerPopKeyHandler();

    Tileset::unloadAll();

    eventHandlerDelete();
    soundDelete();
    musicDelete();
    screenDelete();

    return 0;
}
