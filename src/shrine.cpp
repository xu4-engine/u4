/*
 * shrine.cpp
 */

#include <string>

#include "shrine.h"

#include "game.h"
#include "imagemgr.h"
#include "party.h"
#include "portal.h"
#include "screen.h"
#include "settings.h"
#include "tileset.h"
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

    gameSetViewMode(VIEW_CUTSCENE_MAP);
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
    input = EventHandler::readString(32);
#ifdef IOS
    }
#endif

    screenMessage("\n\nFor how many Cycles (0-3)? ");
#ifdef IOS
    {
    U4IOS::IOSConversationChoiceHelper cyclesChoice;
    cyclesChoice.updateChoices("0123 \015\033");
#endif
    choice = EventHandler::readChoice("0123\015\033");
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
    }
    else if (((c->saveGame->moves / SHRINE_MEDITATION_INTERVAL) >= 0x10000) ||
            (((c->saveGame->moves / SHRINE_MEDITATION_INTERVAL) & 0xffff) != c->saveGame->lastmeditation)) {
        screenMessage("Begin Meditation\n");
        meditationCycle();
    }
    else {
        screenMessage("Thy mind is still weary from thy last Meditation!\n");
        eject();
    }

    gameSetViewMode(VIEW_NORMAL);
}

void Shrine::enhancedSequence() {
    /* replace the 'static' avatar tile with grass */
    annotations.add(Coords(5, 6, c->location->coords.z),
            tileset->getByName(Tile::sym.grass)->getId(), false, true);

    screenDisableCursor();
    screenMessage("You approach\nthe ancient\nshrine...\n");
    gameUpdateScreen();
    EventHandler::wait_msecs(1000);

    const Creature* beggar = xu4.config->creature(BEGGAR_ID);
    Object *obj = addCreature(beggar, Coords(5, 10, c->location->coords.z));

    // Change graphic to the Avatar (which has no animation).
    obj->animControl(ANIM_PAUSED);
    obj->tile = tileset->getByName(Tile::sym.avatar)->getId();

    for (int i = 0; i < 4; ++i) {
        gameUpdateScreen();
        EventHandler::wait_msecs(400);
        c->location->map->move(obj, DIR_NORTH);
    }

    gameUpdateScreen();
    EventHandler::wait_msecs(800);

    obj->tile = beggar->tile;
    obj->animControl(ANIM_PLAYING);

    screenMessage("\n...and kneel before the altar.\n");
    gameUpdateScreen();
    EventHandler::wait_msecs(1000);
    screenEnableCursor();
}

void Shrine::meditationCycle() {
    /* Calculate the millisecond interval for meditation */
    int interval = (xu4.settings->shrineTime * 1000) / MEDITATION_MANTRAS_PER_CYCLE;
    if (interval < 50)
        interval = 50;

    c->saveGame->lastmeditation = (c->saveGame->moves / SHRINE_MEDITATION_INTERVAL) & 0xffff;

    screenDisableCursor();
    for (int i = 0; i < MEDITATION_MANTRAS_PER_CYCLE; i++) {
        screenUploadToGPU();
        if (EventHandler::wait_msecs(interval))
            return;
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
    input = EventHandler::readString(4);
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
        if (elevated) {
            screenMessage("\nThou hast achieved partial Avatarhood in the Virtue of %s\n",
                          getVirtueName(virtue));
            gameSpellEffect(-1, -1, SOUND_ELEVATE);
        } else
            screenMessage("\nThy thoughts are pure. "
                          "Thou art granted a vision!\n");

#ifdef IOS
        U4IOS::IOSConversationChoiceHelper choiceDialog;
        choiceDialog.updateChoices(" ");
        U4IOS::testFlightPassCheckPoint(std::string("Gained avatarhood in: ")
                                        + getVirtueName(virtue));
#endif
        EventHandler::waitAnyKey();
        showVision(elevated);
        EventHandler::waitAnyKey();
        eject();
    }
}

void Shrine::showVision(bool elevated) {
    if (elevated) {
        screenMessage("\nThou art granted a vision!\n");
        gameSetViewMode(VIEW_CUTSCENE);
        const Symbol* visionImageNames = &BKGD_SHRINE_HON;
        screenDrawImageInMapArea(visionImageNames[virtue & 7]);
    } else {
        ShrineState* ss = &c->shrineState;
        screenMessage("\n%s", ss->advice[virtue * 3 + ss->completedCycles - 1].c_str());
    }
}

void Shrine::eject() {
    xu4.game->exitToParentMap();
    musicPlayLocale();
    c->location->turnCompleter->finishTurn();
}
