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
#include "config.h"
#include "debug.h"
#include "error.h"
#include "event.h"
#include "game.h"
#include "intro.h"
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

//#define ENABLE_PERF
#include "support/performance.h"
PERF_OBJ(perf, "debug/performance.txt")
#define PSTART      PERF_START(perf)
#define PEND(msg)   PERF_END(perf,msg)

using namespace std;



int main(int argc, char *argv[]) {
    Debug::initGlobal("debug/global.txt");

#if defined(MACOSX)
    osxInit(argv[0]);
#endif

    if (!u4fsetup())
    {
        errorFatal( "xu4 requires the PC version of Ultima IV to be present. "
                    "It must either be in the same directory as the xu4 executable, "
                    "or in a subdirectory named \"ultima4\"."
                    "\n\nThis can be achieved by downloading \"UltimaIV.zip\" from www.ultimaforever.com"
                    "\n - Extract the contents of UltimaIV.zip"
                    "\n - Copy the \"ultima4\" folder to your xu4 executable location."
                    "\n\nVisit the XU4 website for additional information.\n\thttp://xu4.sourceforge.net/");
    }

    unsigned int i;
    int skipIntro = 0;
    bool enableAudio = true;


    /*
     * if the -p or -profile arguments are passed to the application,
     * they need to be identified before the settings are initialized.
     */
    for (i = 1; i < (unsigned int)argc; i++) {
        if (((strcmp(argv[i], "-p") == 0)
          || (strcmp(argv[i], "--profile") == 0))
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
        if (strcmp(argv[i], "--filter") == 0)
        {
            if ((unsigned int)argc > i + 1)
            {
                settings.filter = argv[i+1];
                i++;
            }
            else
                errorFatal("%s is invalid alone: Requires a string for input. See --help for more detail.\n", argv[i]);
        }
        else if (strcmp(argv[i], "-s") == 0
              || strcmp(argv[i], "--scale") == 0)
        {
            if ((unsigned int)argc > i + 1)
            {
                settings.scale = strtoul(argv[i+1], NULL, 0);
                i++;
            }
            else
                errorFatal("%s is invalid alone: Requires a number for input. See --help for more detail.\n", argv[i]);
        }
        else if ( strcmp(argv[i], "-p") == 0
               || strcmp(argv[i], "--profile") == 0)
        {
            // do nothing
            if ((unsigned int)argc > i + 1)
                i++;
            else
                errorFatal("%s is invalid alone: Requires a string as input. See --help for more detail.\n", argv[i]);

        }
        else if (strcmp(argv[i], "-i") == 0
              || strcmp(argv[i], "--skip-intro") == 0)
        {
            skipIntro = 1;
        }
        else if (strcmp(argv[i], "-v") == 0
              || strcmp(argv[i], "--verbose") == 0)
        {
            verbose = true;
        }
        else if (strcmp(argv[i], "-f") == 0
              || strcmp(argv[i], "--fullscreen") == 0)
        {
            settings.fullscreen = 1;
        }
        else if (strcmp(argv[i], "-q") == 0
              || strcmp(argv[i], "--quiet") == 0)
        {
            enableAudio = false;
        }
        else if (strcmp(argv[i], "-h") == 0
              || strcmp(argv[i], "--help") == 0)
        {
            printf("xu4: Ultima IV Recreated\n"
                   "v%s (%s)\n\n", VERSION, __DATE__ );
            printf(
            "Options:\n"
            "      --filter <string>   Specify display filtering options.\n"
            "  -f, --fullscreen        Run in fullscreen mode.\n"
            "  -h, --help              Print this message and quit.\n"
            "  -i, --skip-intro        Skip the intro. and load the last saved game.\n"
            "  -p <string>,\n"
            "      --profile <string>  Pass extra arguments to the program.\n"
            "  -q, --quiet             Disable audio.\n"
            "  -s <int>,\n"
            "      --scale <int>       Specify scaling factor (1-5).\n"
            "  -v, --verbose           Enable verbose console output.\n"

            "\nFilters: point, 2xBi, 2xSaI, Scale2x\n"
            "\nHomepage: http://xu4.sourceforge.com\n");

            goto cleanup;
        }
        else
            errorFatal("Unrecognized argument: %s\n\nUse --help for a list of supported arguments.", argv[i]);
    }

    xu4_srandom();

    PSTART
    configInit();
    screenInit();
    {
    ProgressBar pb((320/2) - (200/2), (200/2), 200, 10, 0, (skipIntro ? 4 : 6));
    pb.setBorderColor(240, 240, 240);
    pb.setColor(0, 0, 128);
    pb.setBorderWidth(1);

    screenTextAt(15, 11, "Loading...");
    screenRedrawScreen();
    PEND("screenInit")
    ++pb;

    PSTART
    if (enableAudio)
        soundInit();
    PEND("soundInit")
    ++pb;

    PSTART
    Tileset::loadAll();
    PEND("Tileset::loadAll")
    ++pb;

    PSTART
    creatureMgr->getInstance();
    PEND("creatureMgr->getInstance()")
    ++pb;

    intro = new IntroController();
    if (!skipIntro)
    {
        /* do the intro */
        PSTART
        intro->init();
        PEND("intro->init()")
        ++pb;

        PSTART
        intro->preloadMap();
        PEND("intro->preloadMap()")
        ++pb;

        PERF_REPORT(perf, NULL)

        eventHandler->pushController(intro);
        eventHandler->run();
        eventHandler->popController();
        intro->deleteIntro();
    }
    }

    eventHandler->setControllerDone(false);
    if (! quit) {
        PERF_RESET(perf)

        /* play the game! */
        PSTART
        game = new GameController();
        game->init();
        PEND("gameInit()")

        PERF_REPORT(perf, "\n===============================\n\n")

        eventHandler->pushController(game);
        eventHandler->run();
        eventHandler->popController();
    }

    Tileset::unloadAll();
    soundDelete();
    screenDelete();
    configFree();

cleanup:
    u4fcleanup();
    return 0;
}
