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

#include "xu4.h"
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


enum OptionsFlag {
    OPT_FULLSCREEN = 1,
    OPT_NO_INTRO   = 2,
    OPT_NO_AUDIO   = 4,
    OPT_VERBOSE    = 8
};

struct Options {
    uint16_t flags;
    uint16_t used;
    uint32_t scale;
    const char* filter;
    const char* profile;
};

#define strEqual(A,B)       (strcmp(A,B) == 0)
#define strEqualAlt(A,B,C)  (strEqual(A,B) || strEqual(A,C))

/*
 * Return non-zero if the program should continue.
 */
int parseOptions(Options* opt, int argc, char** argv) {
    int i;
    for (i = 0; i < argc; i++) {
        if (strEqual(argv[i], "--filter"))
        {
            if (++i >= argc)
                goto missing_value;
            opt->filter = argv[i];
        }
        else if (strEqualAlt(argv[i], "-s", "--scale"))
        {
            if (++i >= argc)
                goto missing_value;
            opt->scale = strtoul(argv[i], NULL, 0);
        }
        else if (strEqualAlt(argv[i], "-p", "--profile"))
        {
            if (++i >= argc)
                goto missing_value;
            opt->profile = argv[i];
        }
        else if (strEqualAlt(argv[i], "-i", "--skip-intro"))
        {
            opt->flags |= OPT_NO_INTRO;
            opt->used  |= OPT_NO_INTRO;
        }
        else if (strEqualAlt(argv[i], "-v", "--verbose"))
        {
            opt->flags |= OPT_VERBOSE;
            opt->used  |= OPT_VERBOSE;
        }
        else if (strEqualAlt(argv[i], "-f", "--fullscreen"))
        {
            opt->flags |= OPT_FULLSCREEN;
            opt->used  |= OPT_FULLSCREEN;
        }
        else if (strEqualAlt(argv[i], "-q", "--quiet"))
        {
            opt->flags |= OPT_NO_AUDIO;
            opt->used  |= OPT_NO_AUDIO;
        }
        else if (strEqualAlt(argv[i], "-h", "--help"))
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
            "      --profile <string>  Use another set of settings and save files.\n"
            "  -q, --quiet             Disable audio.\n"
            "  -s <int>,\n"
            "      --scale <int>       Specify scaling factor (1-5).\n"
            "  -v, --verbose           Enable verbose console output.\n"

            "\nFilters: point, 2xBi, 2xSaI, Scale2x\n"
            "\nHomepage: http://xu4.sourceforge.com\n");

            return 0;
        }
        else {
            errorFatal("Unrecognized argument: %s\n\n"
                   "Use --help for a list of supported arguments.", argv[i]);
            return 0;
        }
    }
    return 1;

missing_value:
    errorFatal("%s requires a value. See --help for more detail.", argv[i-1]);
    return 0;
}


void servicesInit(XU4GameServices* gs, Options* opt) {
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

    /* initialize the settings */
    gs->settings = new Settings;
    gs->settings->init(opt->profile);

    /* update the settings based upon command-line arguments */
    if (opt->used & OPT_FULLSCREEN)
        gs->settings->fullscreen = (opt->flags & OPT_FULLSCREEN) ? true : false;
    if (opt->scale)
        gs->settings->scale = opt->scale;
    if (opt->filter)
        gs->settings->filter = opt->filter;

    Debug::initGlobal("debug/global.txt");

    xu4_srandom();
    gs->config = configInit();
    screenInit();

    if (! (opt->flags & OPT_NO_AUDIO))
        soundInit();

    gs->eventHandler = new EventHandler;

    Tileset::loadAll();
    creatureMgr->getInstance();

    gs->stage = (opt->flags & OPT_NO_INTRO) ? StagePlay : StageIntro;
}

void servicesFree(XU4GameServices* gs) {
    delete gs->intro;
    Tileset::unloadAll();
    delete gs->eventHandler;
    soundDelete();
    screenDelete();
    configFree(gs->config);
    delete gs->settings;
    u4fcleanup();
}

XU4GameServices xu4;


int main(int argc, char *argv[]) {
#if defined(MACOSX)
    osxInit(argv[0]);
#endif

    {
    Options opt;

    /* Parse arguments before setup in case the user only wants some help. */
    memset(&opt, 0, sizeof opt);
    if (! parseOptions(&opt, argc-1, argv+1))
        return 0;

    memset(&xu4, 0, sizeof xu4);
    servicesInit(&xu4, &opt);
    }

#if 1
    // Modern systems are fast enough that a progress bar isn't really needed
    // but it can be useful during development to see something on screen
    // right away.

    ProgressBar pb((320/2) - (200/2), (200/2), 200, 10, 0, 2);
    pb.setBorderColor(240, 240, 240);
    pb.setColor(0, 0, 128);
    pb.setBorderWidth(1);
    screenTextAt(15, 11, "Loading...");
    ++pb;
#endif

    while( xu4.stage != StageExitGame )
    {
        if( xu4.stage == StageIntro ) {
            if (! xu4.intro)
                xu4.intro = new IntroController;

            /* Show the introduction */
            xu4.intro->init();
            xu4.intro->preloadMap();

            xu4.eventHandler->runController(xu4.intro);

            xu4.intro->deleteIntro();
        } else {

            /* Play the game! */

            if (! xu4.game) {
                xu4.game = new GameController();
                xu4.game->init();
            } else if (xu4.intro && xu4.intro->hasInitiatedNewGame()) {
                //Loads current savegame
                xu4.game->init();
            } else {
                //Inits screen stuff without renewing game
                xu4.game->initScreen();
                xu4.game->initScreenWithoutReloadingState();
                xu4.game->mapArea.reinit();
            }

            xu4.eventHandler->runController(xu4.game);

            xu4.eventHandler->popMouseAreaSet();
            screenSetMouseCursor(MC_DEFAULT);
        }
    }

    servicesFree(&xu4);
    return 0;
}
