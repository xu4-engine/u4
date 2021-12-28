/*
 * xu4.cpp
 */

/** \mainpage xu4 Main Page
 *
 * \section intro_sec Introduction
 *
 * intro stuff goes here...
 */

#include <cstring>
#include <ctime>
#include "xu4.h"
#include "config.h"
#include "debug.h"
#include "error.h"
#include "game.h"
#include "intro.h"
#include "progress_bar.h"
#include "screen.h"
#include "settings.h"
#include "sound.h"
#include "utils.h"

#if defined(MACOSX)
#include "macosx/osxinit.h"
#endif

#ifdef DEBUG
extern int gameSave(const char*);
#endif

bool verbose = false;


enum OptionsFlag {
    OPT_FULLSCREEN = 1,
    OPT_NO_INTRO   = 2,
    OPT_NO_AUDIO   = 4,
    OPT_VERBOSE    = 8,
    OPT_RECORD     = 0x10,
    OPT_REPLAY     = 0x20,
    OPT_TEST_SAVE  = 0x80
};

struct Options {
    uint16_t flags;
    uint16_t used;
    uint32_t scale;
    uint8_t  filter;
    const char* module;
    const char* profile;
    const char* recordFile;
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
            opt->filter = Settings::settingEnum(screenGetFilterNames(),argv[i]);
        }
        else if (strEqualAlt(argv[i], "-s", "--scale"))
        {
            if (++i >= argc)
                goto missing_value;
            opt->scale = strtoul(argv[i], NULL, 0);
        }
#ifdef CONF_MODULE
        else if (strEqualAlt(argv[i], "-m", "--module"))
        {
            if (++i >= argc)
                goto missing_value;
            opt->module = argv[i];
        }
#endif
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
#ifdef CONF_MODULE
            "  -m, --module <file>     Specify game module (default is Ultima-IV).\n"
#endif
            "  -p, --profile <string>  Use another set of settings and save files.\n"
            "  -q, --quiet             Disable audio.\n"
            "  -s, --scale <int>       Specify scaling factor (1-5).\n"
            "  -v, --verbose           Enable verbose console output.\n"
#ifdef DEBUG
            "\nDEBUG Options:\n"
            "  -c, --capture <file>    Record user input.\n"
            "  -r, --replay <file>     Play using recorded input.\n"
            "      --test-save         Save to /tmp/xu4/ and quit.\n"
#endif
#ifdef USE_GL
            "\nFilters: point, HQX\n"
#else
            "\nFilters: point, 2xBi, 2xSaI, Scale2x\n"
#endif
            "\nHomepage: http://xu4.sourceforge.net\n");

            return 0;
        }
#ifdef DEBUG
        else if (strEqualAlt(argv[i], "-c", "--capture"))
        {
            if (++i >= argc)
                goto missing_value;
            opt->recordFile = argv[i];
            opt->flags |= OPT_RECORD;
            opt->used  |= OPT_RECORD;
        }
        else if (strEqualAlt(argv[i], "-r", "--replay"))
        {
            if (++i >= argc)
                goto missing_value;
            opt->recordFile = argv[i];
            opt->flags |= OPT_REPLAY;
            opt->used  |= OPT_REPLAY;
        }
        else if (strEqual(argv[i], "--test-save"))
        {
            opt->flags |= OPT_TEST_SAVE;
        }
#endif
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


#ifdef DEBUG
void servicesFree(XU4GameServices*);
#endif

void servicesInit(XU4GameServices* gs, Options* opt) {
    if (opt->flags & OPT_VERBOSE)
        verbose = true;

    if (!u4fsetup())
    {
        errorFatal( "xu4 requires the PC version of Ultima IV to be present.\n"
            "\nIt may either be a zip file or subdirectory named \"ultima4\" in the same\n"
            "directory as the xu4 executable.\n"
            "\nFor more information visit http://xu4.sourceforge.net/faq.html\n");
    }

    /* Setup the message bus early to make it available to other services. */
    notify_init(&gs->notifyBus, 8);

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

    gs->config = configInit(opt->module ? opt->module : "Ultima-IV.mod");
    screenInit();
    Tile::initSymbols(gs->config);

    if (! (opt->flags & OPT_NO_AUDIO))
        soundInit();

    gs->eventHandler = new EventHandler(1000/gs->settings->gameCyclesPerSecond,
                            1000/gs->settings->screenAnimationFramesPerSecond);

#ifdef DEBUG
    if (opt->flags & OPT_REPLAY) {
        uint32_t seed = gs->eventHandler->replay(opt->recordFile);
        if (! seed) {
            servicesFree(gs);
            errorFatal("Cannot open recorded input from %s", opt->recordFile);
        }
        xu4_srandom(seed);
    } else if (opt->flags & OPT_RECORD) {
        uint32_t seed = time(NULL);
        if (! gs->eventHandler->beginRecording(opt->recordFile, seed)) {
            servicesFree(gs);
            errorFatal("Cannot open recording file %s", opt->recordFile);
        }
        xu4_srandom(seed);
    } else
#endif
        xu4_srandom(time(NULL));

    gs->stage = (opt->flags & OPT_NO_INTRO) ? StagePlay : StageIntro;
}

void servicesFree(XU4GameServices* gs) {
    delete gs->game;
    delete gs->intro;
    delete gs->saveGame;
    delete gs->eventHandler;
    soundDelete();
    screenDelete();
    configFree(gs->config);
    delete gs->settings;
    notify_free(&gs->notifyBus);
    u4fcleanup();
}

XU4GameServices xu4;


int main(int argc, char *argv[]) {
#if defined(MACOSX)
    osxInit(argv[0]);
#endif
    //printf("sizeof(Tile) %ld\n", sizeof(Tile));

    {
    Options opt;

    /* Parse arguments before setup in case the user only wants some help. */
    memset(&opt, 0, sizeof opt);
    if (! parseOptions(&opt, argc-1, argv+1))
        return 0;

    memset(&xu4, 0, sizeof xu4);
    servicesInit(&xu4, &opt);

#ifdef DEBUG
    if (opt.flags & OPT_TEST_SAVE) {
        int status;
        xu4.game = new GameController();
        if (xu4.game->initContext()) {
            gameSave("/tmp/xu4/");
            status = 0;
        } else {
            printf("initContext failed!\n");
            status = 1;
        }
        xu4.stage = StageExitGame;
        servicesFree(&xu4);
        return status;
    }
#endif
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
            /* Show the introduction */
            if (! xu4.intro)
                xu4.intro = new IntroController;
            xu4.eventHandler->runController(xu4.intro);
        } else {
            /* Play the game! */
            if (! xu4.game)
                xu4.game = new GameController();
            xu4.eventHandler->runController(xu4.game);
        }
    }

    servicesFree(&xu4);
    return 0;
}
