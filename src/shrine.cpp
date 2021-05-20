/*
 * shrine.cpp
 */

#include <string>
#include <vector>

#include "shrine.h"

#include "annotation.h"
#include "config.h"
#include "context.h"
#include "event.h"
#include "game.h"
#include "imagemgr.h"
#include "location.h"
#include "creature.h"
#include "names.h"
#include "player.h"
#include "portal.h"
#include "screen.h"
#include "settings.h"
#include "tileset.h"
#include "types.h"
#include "u4.h"
#include "xu4.h"

#ifdef IOS
#include "ios_helpers.h"
#endif

/**
 * Returns true if the player can use the portal to the shrine
 */
bool shrineCanEnter(const Portal *p) {
    Shrine *shrine = dynamic_cast<Shrine*>(xu4.config->map(p->destid));
    if (!c->party->canEnterShrine(shrine->virtue)) {
        screenMessage("Thou dost not bear the rune of entry!  A strange force keeps you out!\n");
        return 0;
    }
    return 1;
}

const char* Shrine::getName() const {
    string& str = c->shrineState.shrineName;
    str = "Shrine of ";
    str += getVirtueName(virtue);
    return str.c_str();
}

const char* Shrine::mantraStr() const {
    return xu4.config->symbolName(mantra);
}

/**
 * Enter the shrine
 */
void Shrine::enter() {
    string input;
    ShrineState* ss = &c->shrineState;
    int choice;

    if (ss->advice.empty()) {
        U4FILE *avatar = u4fopen("avatar.exe");
        if (!avatar)
            return;
        ss->advice = u4read_stringtable(avatar, 93682, 24);
        u4fclose(avatar);
    }
#ifdef IOS
    U4IOS::IOSHideGameControllerHelper hideControllsHelper;
#endif
    if (xu4.settings->enhancements &&
        xu4.settings->enhancementsOptions.u5shrines)
        enhancedSequence();
    else
        screenMessage("You enter the ancient shrine and sit before the altar...");

    screenMessage("\nUpon which virtue dost thou meditate?\n");
#ifdef IOS
    {
    U4IOS::IOSConversationHelper inputVirture;
    inputVirture.beginConversation(U4IOS::UIKeyboardTypeDefault, "Upon which virtue dost thou meditate?");
#endif
    input = ReadStringController::get(32, TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line);
#ifdef IOS
    }
#endif

    screenMessage("\n\nFor how many Cycles (0-3)? ");
#ifdef IOS
    {
    U4IOS::IOSConversationChoiceHelper cyclesChoice;
    cyclesChoice.updateChoices("0123 \015\033");
#endif
    choice = ReadChoiceController::get("0123\015\033");
#ifdef IOS
    }
#endif
    if (choice == '\033' || choice == '\015')
        ss->cycles = 0;
    else
        ss->cycles = choice - '0';
    ss->completedCycles = 0;

    screenMessage("\n\n");

    // ensure the player chose the right virtue and entered a valid number for cycles
    if (strncasecmp(input.c_str(), getVirtueName(virtue), 6) != 0 || ss->cycles == 0) {
        screenMessage("Thou art unable to focus thy thoughts on this subject!\n");
        eject();
        return;
    }

    if (((c->saveGame->moves / SHRINE_MEDITATION_INTERVAL) >= 0x10000) || (((c->saveGame->moves / SHRINE_MEDITATION_INTERVAL) & 0xffff) != c->saveGame->lastmeditation)) {
        screenMessage("Begin Meditation\n");
        meditationCycle();
    }
    else {
        screenMessage("Thy mind is still weary from thy last Meditation!\n");
        eject();
    }
}

void Shrine::enhancedSequence() {
    int cyclesPerSec = xu4.settings->gameCyclesPerSecond;

    /* replace the 'static' avatar tile with grass */
    annotations->add(Coords(5, 6, c->location->coords.z),
            tileset->getByName(Tile::sym.grass)->getId(), false, true);

    screenDisableCursor();
    screenMessage("You approach\nthe ancient\nshrine...\n");
    gameUpdateScreen();
    EventHandler::wait_cycles(cyclesPerSec);

    Object *obj = addCreature(xu4.config->creature(BEGGAR_ID),
                              Coords(5, 10, c->location->coords.z));
    obj->setTile(tileset->getByName(Tile::sym.avatar)->getId());

    for (int i = 0; i < 4; ++i) {
        gameUpdateScreen();
        EventHandler::wait_msecs(400);
        c->location->map->move(obj, DIR_NORTH);
    }

    gameUpdateScreen();
    EventHandler::wait_msecs(800);
    obj->setTile(xu4.config->creature(BEGGAR_ID)->getTile());
    gameUpdateScreen();

    screenMessage("\n...and kneel before the altar.\n");
    EventHandler::wait_cycles(cyclesPerSec);
    screenEnableCursor();
}

void Shrine::meditationCycle() {
    /* find our interval for meditation */
    int interval = (xu4.settings->shrineTime * 1000) / MEDITATION_MANTRAS_PER_CYCLE;
    interval -= (interval % eventTimerGranularity);
    interval /= eventTimerGranularity;
    if (interval <= 0)
        interval = 1;

    c->saveGame->lastmeditation = (c->saveGame->moves / SHRINE_MEDITATION_INTERVAL) & 0xffff;

    screenDisableCursor();
    for (int i = 0; i < MEDITATION_MANTRAS_PER_CYCLE; i++) {
        WaitController controller(interval);
        xu4.eventHandler->pushController(&controller);
        controller.wait();
        screenMessage(".");
    }
    askMantra();
}

void Shrine::askMantra() {
    string input;
    ShrineState* ss = &c->shrineState;

    screenEnableCursor();
    screenMessage("\nMantra: ");

#ifdef IOS
    {
    U4IOS::IOSConversationHelper mantraHelper;
    mantraHelper.beginConversation(U4IOS::UIKeyboardTypeASCIICapable, "Mantra?");
#endif
    input = ReadStringController::get(4, TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line);
    screenMessage("\n");
#ifdef IOS
    }
#endif

    if (strcasecmp(input.c_str(), mantraStr()) != 0) {
        c->party->adjustKarma(KA_BAD_MANTRA);
        screenMessage("Thou art not able to focus thy thoughts with that Mantra!\n");
        eject();
    }
    else if (--ss->cycles > 0) {
        ss->completedCycles++;
        c->party->adjustKarma(KA_MEDITATION);
        meditationCycle();
    }
    else {
        ss->completedCycles++;
        c->party->adjustKarma(KA_MEDITATION);

        bool elevated = ss->completedCycles == 3 && c->party->attemptElevation(virtue);
        if (elevated)
            screenMessage("\nThou hast achieved partial Avatarhood in the Virtue of %s\n\n",
                          getVirtueName(virtue));
        else
            screenMessage("\nThy thoughts are pure. "
                          "Thou art granted a vision!\n");

#ifdef IOS
        U4IOS::IOSConversationChoiceHelper choiceDialog;
        choiceDialog.updateChoices(" ");
        U4IOS::testFlightPassCheckPoint(std::string("Gained avatarhood in: ")
                                        + getVirtueName(virtue));
#endif
        ReadChoiceController::get("");
        showVision(elevated);
        ReadChoiceController::get("");
        gameSetViewMode(VIEW_NORMAL);
        eject();
    }
}

void Shrine::showVision(bool elevated) {
    static const char *visionImageNames[] = {
        BKGD_SHRINE_HON, BKGD_SHRINE_COM, BKGD_SHRINE_VAL, BKGD_SHRINE_JUS,
        BKGD_SHRINE_SAC, BKGD_SHRINE_HNR, BKGD_SHRINE_SPI, BKGD_SHRINE_HUM
    };

    if (elevated) {
        screenMessage("Thou art granted a vision!\n");
        gameSetViewMode(VIEW_RUNE);
        screenDrawImageInMapArea(visionImageNames[virtue]);
    }
    else {
        ShrineState* ss = &c->shrineState;
        screenMessage("\n%s", ss->advice[virtue * 3 + ss->completedCycles - 1].c_str());
    }
}

void Shrine::eject() {
    xu4.game->exitToParentMap();
    musicPlayLocale();
    c->location->turnCompleter->finishTurn();
}
