/*
 * $Id$
 */

/** \mainpage xu4 Main Page
 *
 * \section intro_sec Introduction
 *
 * intro stuff goes here...
 */
 
#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <SDL.h>

#include "u4.h"

#include "debug.h"
#include "error.h"
#include "event.h"
#include "game.h"
#include "intro.h"
#include "music.h"
#include "person.h"
#include "progress_bar.h"
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

Performance perf("debug/performance.txt");

using namespace std;

int main(int argc, char *argv[]) {
    unsigned int i;
    int skipIntro = 0;    

#if defined(MACOSX)
    osxInit(argv[0]);
#endif

    Debug::initGlobal("debug/global.txt");

    for (i = 1; i < (unsigned int)argc; i++) {
        if (strcmp(argv[i], "-filter") == 0 && (unsigned int)argc > i + 1) {
            settings.filter = (FilterType)settings.filters.getType(argv[i+1]);
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
    ProgressBar pb((320/2) - (200/2), (200/2), 200, 10, 0, 4);
    pb.setBorderColor(240, 240, 240);
    pb.setColor(0, 0, 128);
    pb.setBorderWidth(1);    

    screenTextAt(15, 11, "Loading...");
    screenRedrawScreen();
    perf.end("Screen Initialization");
    ++pb;    

    perf.start();
    soundInit();    
    perf.end("Misc Initialization");
    ++pb;

    perf.start();
    Tileset::loadAll();
    perf.end("Tileset::loadAll()");
    ++pb;

    perf.start();
    creatures.loadInfoFromXml();
    perf.end("CreatureMgr::loadInfoFromXml()");
    ++pb;

    intro = new IntroController();
    if (!skipIntro) {
        /* do the intro */
        perf.start();
        intro->init();
        perf.end("introInit()");        
        
        /* give a performance report */
        if (settings.debug)
            perf.report();

        eventHandler->pushController(intro);
        eventHandler->run();
        eventHandler->popController();
        intro->deleteIntro();
        delete intro;
    }

    eventHandler->setControllerDone(false);
    if (quit)
        return 0;

    /* initialize person data */
    perf.reset();
    perf.start();
    if (!personInit())
        errorFatal("unable to load person data files: is Ultima IV installed?  See http://xu4.sourceforge.net/");
    perf.end("personInit()");

    /* play the game! */
    perf.start();
    game = new GameController();
    game->init();    
    perf.end("gameInit()");
    
    /* give a performance report */
    if (settings.debug)
        perf.report("\n===============================\n\n");

    eventHandler->pushController(game);    
    eventHandler->run();
    eventHandler->popController();

    Tileset::unloadAll();

    delete musicMgr;
    soundDelete();
    screenDelete();

    return 0;
}
