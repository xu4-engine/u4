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

#include "u4.h"
#include <cstring>
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
#include "SDL.h"
#endif

bool verbose = false;
bool quit = false;
bool useProfile = false;
string profileName = "";

Performance perf("debug/performance.txt");

using namespace std;



int main(int argc, char *argv[]) {
    Debug::initGlobal("debug/global.txt");
    
#if defined(MACOSX)
    osxInit(argv[0]);
#endif

    if (!u4fopen("AVATAR.EXE"))
	{
        errorFatal(	"xu4 requires the PC version of Ultima IV to be present. "
        			"It must either be in the same directory as the xu4 executable, "
        			"or in a subdirectory named \"ultima4\"."
        			"\n\nThis can be achieved by downloading \"UltimaIV.zip\" from www.ultimaforever.com"
        			"\n - Extract the contents of UltimaIV.zip"
        			"\n - Copy the \"ultima4\" folder to your xu4 executable location."
        			"\n\nVisit the XU4 website for additional information.\n\thttp://xu4.sourceforge.net/");
	}

	unsigned int i;
    int skipIntro = 0;


    /*
     * if the -p or -profile arguments are passed to the application,
     * they need to be identified before the settings are initialized.
     */
    for (i = 1; i < (unsigned int)argc; i++) {
        if (((strcmp(argv[i], "-p") == 0) || (strcmp(argv[i], "-profile") == 0))
                && (unsigned int)argc > i + 1) {
            // when grabbing the profile name:
            // 1. trim leading whitespace
            // 2. truncate the string at 20 characters
            // 3. then strip any trailing whitespace
            profileName = argv[i+1];
            profileName = profileName.erase(0,profileName.find_first_not_of(' '));
            profileName.resize(20, ' ');
            profileName = profileName.erase(profileName.find_last_not_of(' ')+1);

            // verify that profileName is valid, otherwise do not use the profile
            if (!profileName.empty()) {
                useProfile = true;
            }
            i++;
            break;
        }
    }

    /* initialize the settings */
    settings.init(useProfile, profileName);

    /* update the settings based upon command-line arguments */
    for (i = 1; i < (unsigned int)argc; i++) {
        if (strcmp(argv[i], "-filter") == 0 && (unsigned int)argc > i + 1) {
            settings.filter = argv[i+1];
            i++;
        }
        else if (strcmp(argv[i], "-scale") == 0 && (unsigned int)argc > i + 1) {
            settings.scale = strtoul(argv[i+1], NULL, 0);
            i++;
        }
        else if (((strcmp(argv[i], "-p") == 0)
                    || (strcmp(argv[i], "-profile") == 0))
                && (unsigned int)argc > i + 1) {
            // do nothing
            i++;
        }
        else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "-skipintro") == 0)
            skipIntro = 1;
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "-verbose") == 0)
            verbose = true;
        else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "-fullscreen") == 0)
            settings.fullscreen = 1;
        else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "-quiet") == 0) {
            settings.musicVol = 0;
            settings.soundVol = 0;
        }
    }

    xu4_srandom();

    perf.start();
    screenInit();
    ProgressBar pb((320/2) - (200/2), (200/2), 200, 10, 0, (skipIntro ? 4 : 7));
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
    creatureMgr->getInstance();
    perf.end("creatureMgr->getInstance()");
    ++pb;

    intro = new IntroController();
    if (!skipIntro)
    {
        /* do the intro */
        perf.start();
        intro->init();
        perf.end("introInit()");
        ++pb;

        perf.start();
        intro->preloadMap();
        perf.end("intro->preloadMap()");
        ++pb;

        perf.start();
        musicMgr->init();
        perf.end("musicMgr->init()");
        ++pb;

        /* give a performance report */
        if (settings.debug)
            perf.report();

        eventHandler->pushController(intro);
        eventHandler->run();
        eventHandler->popController();
        intro->deleteIntro();
    }

    eventHandler->setControllerDone(false);
    if (quit)
        return 0;

    perf.reset();

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


