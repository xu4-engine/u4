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

//#define ENABLE_PERF
#include "support/performance.h"
PERF_OBJ(perf, "debug/performance.txt")
#define PSTART      PERF_START(perf)
#define PEND(msg)   PERF_END(perf,msg)


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


Options opt;

int main(int argc, char *argv[]) {
    Debug::initGlobal("debug/global.txt");

#if defined(MACOSX)
    osxInit(argv[0]);
#endif

    /* Parse arguments before setup in case the user only wants some help. */
    memset(&opt, 0, sizeof opt);
    if (! parseOptions(&opt, argc-1, argv+1))
        return 0;

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
    settings.init(opt.profile);

    /* update the settings based upon command-line arguments */
    if (opt.used & OPT_FULLSCREEN)
        settings.fullscreen = (opt.flags & OPT_FULLSCREEN) ? true : false;
    if (opt.scale)
        settings.scale = opt.scale;
    if (opt.filter)
        settings.filter = opt.filter;

    xu4_srandom();

    PSTART
    configInit();
    screenInit();
    {
    int skipIntro = opt.flags & OPT_NO_INTRO;
    ProgressBar pb((320/2) - (200/2), (200/2), 200, 10, 0, (skipIntro ? 4 : 6));
    pb.setBorderColor(240, 240, 240);
    pb.setColor(0, 0, 128);
    pb.setBorderWidth(1);

    screenTextAt(15, 11, "Loading...");
    PEND("screenInit")
    ++pb;

    PSTART
    if (! (opt.flags & OPT_NO_AUDIO))
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

    u4fcleanup();
    return 0;
}
