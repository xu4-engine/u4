/*
 * codex.cpp
 */

#include <string>
#include <cstring>
#include <vector>

#include "codex.h"

#include "context.h"
#include "game.h"
#include "imagemgr.h"
#include "savegame.h"
#include "screen.h"
#include "stats.h"
#include "u4.h"
#include "u4file.h"
#include "xu4.h"
#ifdef IOS
#include "ios_helpers.h"
#endif

struct Codex {
    vector<std::string> virtueQuestions;
    vector<std::string> endgameText1;
    vector<std::string> endgameText2;
    std::string word;
};

static void codexEject(CodexEjectCode code);
static bool codexHandleWOP(Codex*);
static bool codexHandleVirtues(Codex*);
static bool codexHandleInfinity(Codex*);
static void codexHandleEndgame(Codex*);

/**
 * Initializes the Chamber of the Codex sequence (runs from codexStart())
 */
static int codexInit(Codex* codex) {
    U4FILE* avatar = u4fopen("avatar.exe");
    if (! avatar)
        return 0;

    codex->virtueQuestions = u4read_stringtable(avatar, 0x0fc7b, 11);
    codex->endgameText1    = u4read_stringtable(avatar, 0x0fee4, 7);
    codex->endgameText2    = u4read_stringtable(avatar, 0x10187, 5);
    u4fclose(avatar);
    return 1;
}

/**
 * Frees all memory associated with the Codex sequence
 */
static void codexFree(Codex* codex) {
    codex->virtueQuestions.clear();
    codex->endgameText1.clear();
    codex->endgameText2.clear();
}

static void pausedMessage(int sec, const char* msg) {
    screenMessage(msg);
    screenUploadToGPU();
    EventHandler::wait_msecs(sec * 1000);
}

/* slight pause before continuing */
static void codexSlightPause() {
    screenMessage("\n");
    screenDisableCursor();
    screenUploadToGPU();
    EventHandler::wait_msecs(1000);
}

/**
 * Begins the Chamber of the Codex sequence
 */
void codexStart() {
    Codex codex;
    if(! codexInit(&codex))
        return;

    // disable the whirlpool cursor and black out the screen
#ifdef IOS
    U4IOS::IOSHideGameControllerHelper hideControllsHelper;
#endif
    screenDisableCursor();
    screenUpdate(&xu4.game->mapArea, false, true);

    // make the avatar alone
    c->stats->setView(STATS_PARTY_OVERVIEW);
    c->stats->update(true);

    // change the view mode so the dungeon doesn't get shown
    gameSetViewMode(VIEW_CODEX);

    pausedMessage(4, "\n\n\n\nThere is a sudden darkness, and you find yourself alone in an empty chamber.\n");

    /* check to see if you have the 3-part key */
    const int allKeyItems = ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T;
    if ((c->saveGame->items & allKeyItems) == allKeyItems) {
        screenDrawImageInMapArea(BKGD_KEY);
        screenRedrawMapArea();

        pausedMessage(3, "\nYou use your key of Three Parts.\n");

        if (codexHandleWOP(&codex)) {
            if (codexHandleVirtues(&codex)) {
                if (codexHandleInfinity(&codex)) {
                    codexHandleEndgame(&codex);
                    codexFree(&codex);
                    xu4.eventHandler->pushKeyHandler(KeyHandler::ignoreKeys);
                    return;     // Don't reset view mode - pause forever.
                }
            }
        }
    } else {
        codexEject(CODEX_EJECT_NO_3_PART_KEY);
    }

    codexFree(&codex);
    gameSetViewMode(VIEW_NORMAL);
}

/**
 * Ejects you from the chamber of the codex (and the Abyss, for that matter)
 * with the correct message.
 */
static void codexEject(CodexEjectCode code) {
    struct {
        int x, y;
    } startLocations[] = {
        { 231, 136 },
        { 83, 105 },
        { 35, 221 },
        { 59, 44 },
        { 158, 21 },
        { 105, 183 },
        { 23, 129 },
        { 186, 171 }
    };

    switch(code) {
    case CODEX_EJECT_NO_3_PART_KEY:
        screenMessage("\nThou dost not have the Key of Three Parts.\n\n");
        break;
    case CODEX_EJECT_NO_FULL_PARTY:
        pausedMessage(2, "\nThou hast not proved thy leadership in all eight virtues.\n\n");
        screenMessage("\nPassage is not granted.\n\n");
        break;
    case CODEX_EJECT_NO_FULL_AVATAR:
        pausedMessage(2, "\nThou art not ready.\n");
        screenMessage("\nPassage is not granted.\n\n");
        break;
    case CODEX_EJECT_BAD_WOP:
        screenMessage("\nPassage is not granted.\n\n");
        break;
    case CODEX_EJECT_HONESTY:
    case CODEX_EJECT_COMPASSION:
    case CODEX_EJECT_VALOR:
    case CODEX_EJECT_JUSTICE:
    case CODEX_EJECT_SACRIFICE:
    case CODEX_EJECT_HONOR:
    case CODEX_EJECT_SPIRITUALITY:
    case CODEX_EJECT_HUMILITY:
    case CODEX_EJECT_TRUTH:
    case CODEX_EJECT_LOVE:
    case CODEX_EJECT_COURAGE:
        screenMessage("\nThy quest is not yet complete.\n\n");
        break;
    case CODEX_EJECT_BAD_INFINITY:
        screenMessage("\nThou dost not know the true nature of the Universe.\n\n");
        break;
    default:
        screenMessage("\nOops, you just got too close to beating the game.\nBAD AVATAR!\n");
        break;
    }

    screenUploadToGPU();
    EventHandler::wait_msecs(2000);

    /* re-enable the cursor and show it */
    screenEnableCursor();
    screenShowCursor();

    /* return view to normal and exit the Abyss */
    gameSetViewMode(VIEW_NORMAL);
    xu4.game->exitToParentMap();
    musicPlayLocale();

    /**
     * if being ejected because of a missed virtue question,
     * then teleport the party to the starting location for
     * that virtue.
     */
    if (code >= CODEX_EJECT_HONESTY && code <= CODEX_EJECT_HUMILITY) {
        int virtue = code - CODEX_EJECT_HONESTY;
        c->location->coords.x = startLocations[virtue].x;
        c->location->coords.y = startLocations[virtue].y;
    }

    /* finally, finish the turn */
    c->location->turnCompleter->finishTurn();
    xu4.eventHandler->setController(xu4.game);
}

static void codexImpureThoughts() {
    pausedMessage(2, "\nThy thoughts are not pure.\nI ask again.\n");
}

/**
 * Handles entering the Word of Passage.
 * Return true if player succeeds.
 */
static bool codexHandleWOP(Codex* codex) {
    int i;

    for (i = 0; i < 3; ++i) {
        if (i == 0) {
            screenMessage("\nA voice rings out:\n");
#ifdef IOS
            U4IOS::IOSConversationHelper::setIntroString("What is the Word of Passage?");
#endif
        } else {
            /* entered incorrectly - try again */
            codexImpureThoughts();
#ifdef IOS
            U4IOS::IOSConversationHelper::setIntroString("Which virtue?");
#endif
        }

        screenMessage("\"What is the Word of Passage?\"\n\n");
        codex->word = gameGetInput();
        codexSlightPause();
        if (strcasecmp(codex->word.c_str(), "veramocor") == 0)
            goto correct;
    }

    /* 3 tries are up... eject! */
    codexEject(CODEX_EJECT_BAD_WOP);
    return false;

correct:
    /* eject them if they don't have all 8 party members */
    if (c->saveGame->members != 8) {
        codexEject(CODEX_EJECT_NO_FULL_PARTY);
        return false;
    }

    /* eject them if they're not a full avatar at this point */
    for (i = 0; i < VIRT_MAX; i++) {
        if (c->saveGame->karma[i] != 0) {
            codexEject(CODEX_EJECT_NO_FULL_AVATAR);
            return false;
        }
    }

    pausedMessage(4, "\nPassage is granted.\n");
    screenEraseMapArea();
    screenRedrawMapArea();
    return true;
}

/**
 * Handles naming of virtues in the Chamber of the Codex
 * Return true if player succeeds.
 */
static bool codexHandleVirtues(Codex* codex) {
    const Symbol* codexImageNames = &BKGD_HONESTY;
    int current = 0;
    int tries = 0;

    /* Ask the Virtue questions */
ask_next:
    codexSlightPause();
    pausedMessage(2, "\n\nThe voice asks:\n");
    screenMessage("\n%s\n\n", codex->virtueQuestions[current].c_str());
#ifdef IOS
    U4IOS::IOSConversationHelper::setIntroString((current < VIRT_MAX) ? "Which virtue?" : "Which principle?");
#endif
    codex->word = gameGetInput();
    screenDisableCursor();

    if ((current < VIRT_MAX) &&
        (strcasecmp(codex->word.c_str(), getVirtueName(static_cast<Virtue>(current))) == 0)) {
        /* answered with the correct one of eight virtues */

        Image::enableBlend(1);
        screenDrawImageInMapArea(codexImageNames[current]);
        Image::enableBlend(0);
        screenRedrawMapArea();
        screenUploadToGPU();

        EventHandler::wait_msecs(2000);

        tries = 0;
        if (++current == VIRT_MAX)
            pausedMessage(5, "\n\nThou art well versed in the virtues of the Avatar.\n");
        goto ask_next;
    }
    else if ((current >= VIRT_MAX) &&
             (strcasecmp(codex->word.c_str(), getBaseVirtueName(static_cast<BaseVirtue>(1 << (current - VIRT_MAX)))) == 0)) {
        /* answered with the correct base virtue (truth, love, courage) */

        Image::enableBlend(1);
        screenDrawImageInMapArea(codexImageNames[current]);
        Image::enableBlend(0);
        screenRedrawMapArea();

        tries = 0;
        if (++current < VIRT_MAX+3)
            goto ask_next;

        pausedMessage(1, "\n\nThe ground rumbles beneath your feet.\n");
        soundPlay(SOUND_RUMBLE);
        screenShake(10);
        EventHandler::wait_msecs(3000);
        return true;
    }
    else if (++tries < 3) {
        /* give them 3 tries to enter the correct virtue, then eject them! */

        codexImpureThoughts();
        screenMessage("%s\n\n", codex->virtueQuestions[current].c_str());
#ifdef IOS
        U4IOS::IOSConversationHelper::setIntroString("Which virtue?");
#endif
        goto ask_next;
    }

    /* failed 3 times... eject! */
    codexEject(static_cast<CodexEjectCode>(CODEX_EJECT_HONESTY + current));
    return false;
}

static bool codexHandleInfinity(Codex* codex) {
    ReadChoiceController pauseController("");
    int i;

    for (i = 0; i < 3; ++i) {
        if (i > 0)
            codexImpureThoughts();

        screenEnableCursor();
        screenMessage("\nAbove the din, the voice asks:\n\nIf all eight virtues of the Avatar combine into and are derived from the Three Principles of Truth, Love and Courage...");
        screenUploadToGPU();

        xu4.eventHandler->pushController(&pauseController);
        pauseController.waitFor();

        screenMessage("\n\nThen what is the one thing which encompasses and is the whole of all undeniable Truth, unending Love, and unyielding Courage?\n\n");
#ifdef IOS
        U4IOS::IOSConversationHelper::setIntroString("What is the whole of all undeniable Truth, unending Love, and unyielding Courage?");
#endif
        codex->word = gameGetInput();
#ifdef IOS
        U4IOS::IOSHideGameControllerHelper hideControllsHelper;
#endif
        codexSlightPause();
        if (strcasecmp(codex->word.c_str(), "infinity") == 0)
            goto correct;
    }

    codexEject(CODEX_EJECT_BAD_INFINITY);
    return false;

correct:
    EventHandler::wait_msecs(2000);
    soundPlay(SOUND_RUMBLE);
    screenShake(10);
    // TODO: Animate reveal of infinity symbol.
    screenDrawImageInMapArea(BKGD_RUNE_INF);
    return true;
}

static void codexHandleEndgame(Codex* codex) {
    ReadChoiceController pauseController("");
    int i;

    screenEnableCursor();

#ifdef IOS
    // Ugh, we now enter happy callback land, so I know how to do these things manually. Good thing I kept these separate functions.
    U4IOS::hideGameButtons();
    U4IOS::beginChoiceConversation();
    U4IOS::updateChoicesInDialog(" ", "", -1);
    U4IOS::testFlightPassCheckPoint("Game won!");
#endif

    for (i = 0; i < 10; ++i) {
        if (i == 0) {
            screenMessage("\n\n%s", codex->endgameText1[0].c_str());
        } else if (i < 7) {
            if (i == 6) {
                screenEraseMapArea();
                screenRedrawMapArea();
            }
            screenMessage("%s", codex->endgameText1[i].c_str());
        }
        else if (i == 7) {
            screenDrawImageInMapArea(BKGD_STONCRCL);
            screenRedrawMapArea();
            screenMessage("\n\n%s", codex->endgameText2[0].c_str());
        }
        else if (i > 7) {
            screenMessage("%s", codex->endgameText2[i-7].c_str());
        }
        screenUploadToGPU();

        xu4.eventHandler->pushController(&pauseController);
        pauseController.waitFor();
    }

    /* CONGRATULATIONS!... you have completed the game in x turns */
    screenDisableCursor();
    // Note: This text has leading spaces & should be centered.
    screenMessage("%s%d%s", codex->endgameText2[3].c_str(),
                  c->saveGame->moves,
#if 0
                  codex->endgameText2[4].c_str()
#else
                  "\n turns! Report\n thy feat unto\n"
                  "the XU4 team at\nSourceForge.net!"
#endif
                            );
#ifdef IOS
    U4IOS::endChoiceConversation();
#endif
    screenUploadToGPU();
}
