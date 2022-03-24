/*
 * discourse_castle.cpp
 */

#include <cstring>
#include "context.h"
#include "party.h"
#include "u4.h"

/* Lord British text indexes */
//#define LB_JOB          2
//#define LB_ABYSS        19
#define LB_KEY_COUNT    24

/* Hawkwind text indexes */
#define HW_SPEAKONLYWITH 40
#define HW_RETURNWHEN   41
#define HW_ISREVIVED    42
#define HW_WELCOME      43
#define HW_GREETING     44
#define HW_PROMPT_FIRST 45
#define HW_PROMPT       46
#define HW_DEFAULT      49
#define HW_ALREADYAVATAR 50
#define HW_GOTOSHRINE   51
#define HW_FAREWELL     52

struct U4TalkLordBritish {
    char* strings;
    uint16_t keyword[LB_KEY_COUNT+1];   // +1 for extra empty string.
    uint16_t text[LB_KEY_COUNT];
};

struct U4TalkHawkwind {
    char* strings;
    uint16_t text[53];
};

/*
 * A special case dialogue loader for Lord British.  Loads most of the
 * keyword/responses from a hardcoded location in avatar.exe.
 */
void* U4LordBritish_load(U4FILE* avatar) {
    const int lbTextSize = 172 + 1 + 2967;
    U4TalkLordBritish* lb;

    lb = (U4TalkLordBritish*) malloc(sizeof(U4TalkLordBritish) + lbTextSize);
    lb->strings = (char*) (lb + 1);

    avatar->seek(87581, SEEK_SET);
    avatar->read(lb->strings, 1, lbTextSize);

    /*
    Replace unknown characters in the abyss text with newlines.
    Here are the bytes between text entries 19 and 20:
    160C0   21 ab 1c b2 0a 08 00 b0  49 74 20 69 73 20 73 61   !.......It is sa
    */
    uint8_t* bad = (uint8_t*) (lb->strings + 2724);
    if (bad[0] == 0xAB)
        memset(bad, '\n', 7);

    U4Talk_setOffsets(lb->keyword, lb->strings, 0, LB_KEY_COUNT*2 + 1);
    return lb;
}

/*
 * A special case dialogue loader for Hawkwind.
 */
void* U4Hawkwind_load(U4FILE* avatar) {
    const int hwTextSize = 3485;
    U4TalkHawkwind* hw;

    hw = (U4TalkHawkwind*) malloc(sizeof(U4TalkHawkwind) + hwTextSize);
    hw->strings = (char*) (hw + 1);

    avatar->seek(74729, SEEK_SET);
    avatar->read(hw->strings, 1, hwTextSize);

    U4Talk_setOffsets(hw->text, hw->strings, 0, 53);
    return hw;
}

/**
 * Check the levels of each party member while talking to Lord British
 */
static void lordBritishCheckLevels() {
    bool advanced = false;

    for (int i = 0; i < c->party->size(); i++) {
        PartyMember *player = c->party->member(i);
        if (player->getRealLevel() < player->getMaxLevel()) {
            // add an extra space to separate messages
            if (! advanced) {
                advanced = true;
                screenCrLf();
            }
            player->advanceLevel();
        }
    }
}

/**
 * Print text with pauses after paragraphs and/or complete pages.
 */
static void messageParts(AnyKeyController* anyKey, const char* text) {
    const char* cp = text;
    int lines = 0;
    int len;

    while (*cp == '\n')
        ++cp;

    while (*cp) {
        if (cp[0] == '\n') {
            ++lines;
            if (lines == TEXT_AREA_H || cp[1] == '\n' ||
                (lines >= TEXT_AREA_H-2 && cp[-1] == '!')) {
                // Print newline except if page is completely full.
                if (lines != TEXT_AREA_H)
                    ++cp;
                lines = 0;

                len = cp - text;
                screenMessageN(text, len);
                text += len;
                anyKey->waitTimeout();

                while (*cp == '\n')
                    ++cp;
                continue;
            }
        }
        ++cp;
    }
    screenMessageN(text, cp - text);
}

static const uint8_t lbKeyLine[LB_KEY_COUNT] = {
    /*
    name  look      job       truth love         courage  honesty compassion
    valor justice   sacrifice honor spirituality humility pride   avatar
    quest britannia ankh      abyss mondain      minax    exodus  virtue
    */
     0,255,  1,  2,  3,  4,  5,  6,
     7,  8,  9, 10, 11, 12, 14, 16,
    18, 20, 23, 24, 27, 28, 29, 30
};

enum SpokenLines {
    LINE_WELCOME = 31,
    LINE_WELCOME_ADV,
    LINE_ASK_OF_ME,
    LINE_AT_LAST,
    LINE_NEW_AGE,
    LINE_CHAMPION,
    LINE_I_AM_WELL,
    LINE_CANNOT_HELP,
    LINE_FAREWELL,
    LINE_FAREWELL_S,
    LINE_LIVE_AGAIN,
    LINE_GOOD,
    LINE_LET_ME_HEAL,
    LINE_TO_SURVIVE,
    LINE_TRAVEL_NOT,
    LINE_LEARN_YE,
    LINE_VISIT_THE,
    LINE_GO_YE_NOW
};

/**
 * Generate the appropriate response when the player asks Lord British
 * for help.  The help text depends on the current party status; when
 * one quest item is complete, Lord British provides some direction to
 * the next one.
 */
static void lordBritishHelp(AnyKeyController* anyKey) {
    const char* text;
    const SaveGame* saveGame = c->saveGame;
    bool fullAvatar, partialAvatar;
    int v;

    /*
     * check whether player is full avatar (in all virtues) or partial
     * avatar (in at least one virtue)
     */
    fullAvatar = true;
    partialAvatar = false;
    for (v = 0; v < VIRT_MAX; v++) {
        fullAvatar &= (saveGame->karma[v] == 0);
        partialAvatar |= (saveGame->karma[v] == 0);
    }

    if (saveGame->moves <= 1000) {
        soundSpeakLine(VOICE_LB, LINE_TO_SURVIVE);
        text = "To survive in this hostile land thou must first know thyself!"
               " Seek ye to master thy weapons and thy magical ability!\n"
               "\nTake great care in these thy first travels in Britannia.\n"
               "\nUntil thou dost well know thyself, travel not far from the"
               " safety of the townes!\n";
    }
    else if (saveGame->members == 1) {
        soundSpeakLine(VOICE_LB, LINE_TRAVEL_NOT);
        text = "Travel not the open lands alone. There are many worthy people"
               " in the diverse townes whom it would be wise to ask to Join"
               " thee!\n"
               "\nBuild thy party unto eight travellers, for only a true"
               " leader can win the Quest!\n";
    }
    else if (saveGame->runes == 0) {
        soundSpeakLine(VOICE_LB, LINE_LEARN_YE);
        text = "Learn ye the paths of virtue. Seek to gain entry unto the"
               " eight shrines!\n"
               "\nFind ye the Runes, needed for entry into each shrine, and"
               " learn each chant or \"Mantra\" used to focus thy meditations.\n"
               "\nWithin the Shrines thou shalt learn of the deeds which show"
               " thy inner virtue or vice!\n"
               "\nChoose thy path wisely for all thy deeds of good and evil"
               " are remembered and can return to hinder thee!\n";
    }
    else if (! partialAvatar) {
        soundSpeakLine(VOICE_LB, LINE_VISIT_THE);
        text = "Visit the Seer Hawkwind often and use his wisdom to help thee"
               " prove thy virtue.\n"
               "\nWhen thou art ready, Hawkwind will advise thee to seek the"
               " Elevation unto partial Avatarhood in a virtue.\n"
               "\nSeek ye to become a partial Avatar in all eight virtues,"
               " for only then shalt thou be ready to seek the codex!\n";
    }
    else if (saveGame->stones == 0) {
        soundSpeakLine(VOICE_LB, LINE_GO_YE_NOW);
        text = "Go ye now into the depths of the dungeons. Therein recover the"
               " 8 colored stones from the altar pedestals in the halls of the"
               " dungeons.\n"
               "\nFind the uses of these stones for they can help thee in the"
               " Abyss!\n";
    }
    else if (! fullAvatar) {
        text = "Thou art doing very well indeed on the path to Avatarhood!"
               " Strive ye to achieve the Elevation in all eight virtues!\n";
    }
    else if ((saveGame->items & ITEM_BELL) == 0 ||
             (saveGame->items & ITEM_BOOK) == 0 ||
             (saveGame->items & ITEM_CANDLE) == 0) {
        text = "Find ye the Bell, Book and Candle!  With these three things,"
               " one may enter the Great Stygian Abyss!\n";
    }
    else if ((saveGame->items & ITEM_KEY_C) == 0 ||
             (saveGame->items & ITEM_KEY_L) == 0 ||
             (saveGame->items & ITEM_KEY_T) == 0) {
        text = "Before thou dost enter the Abyss thou shalt need the Key of"
               " Three Parts, and the Word of Passage.\n"
               "\nThen might thou enter the Chamber of the Codex of Ultimate"
               " Wisdom!\n";
    }
    else {
        text = "Thou dost now seem ready to make the final journey into the"
               " dark Abyss! Go only with a party of eight!\n"
               "\nGood Luck, and may the powers of good watch over thee on"
               " this thy most perilous endeavor!\n"
               "\nThe hearts and souls of all Britannia go with thee now."
               " Take care, my friend.\n";
    }

    screenMessageN("\nHe says: ", 10);
    messageParts(anyKey, text);
}

static void lordBritishHeal(char answer) {
    if (answer == 'y') {
        soundSpeakLine(VOICE_LB, LINE_GOOD);
        message("\n\nHe says: That is good.\n");
    }
    else if (answer == 'n') {
        soundSpeakLine(VOICE_LB, LINE_LET_ME_HEAL);
        message("\n\nHe says: Let me heal thy wounds!\n");

        // Same spell effect as 'r'esurrect.
        soundPlay(SOUND_PREMAGIC_MANA_JUMBLE, false, 1000);
        EventHandler::wait_msecs(1000);
        gameSpellEffect('r', -1, SOUND_MAGIC);

        // Cure & heal the party.
        Party* party = c->party;
        for (int i = 0; i < party->size(); i++)
            party->member(i)->heal(HT_RESTORE);
    }
}

static void runTalkLordBritish(const U4TalkLordBritish* lb)
{
    AnyKeyController anyKey;
    std::string input;
    const char* in;
    const char* strings = lb->strings;
    const Party* party = c->party;
    PartyMember* p0 = party->member(0);
    string pcName( p0->getName() );
    int k;

    /* If the avatar is dead Lord British resurrects them! */
    if (p0->getStatus() == STAT_DEAD) {
        soundSpeakLine(VOICE_LB, LINE_LIVE_AGAIN);
        screenMessage("%s, Thou shalt live again!\n", pcName.c_str());
        p0->setStatus(STAT_GOOD);
        p0->heal(HT_FULLHEAL);
        gameSpellEffect('r', -1, SOUND_LBHEAL);
    }

    musicPlay(MUSIC_RULEBRIT);

    if (c->saveGame->lbintro) {
        static const char* welcome = "\n\n\nLord British says:  Welcome ";

        switch (party->size()) {
            case 1:
                message("%s%s!\n", welcome, pcName.c_str());
                soundSpeakLine(VOICE_LB, LINE_WELCOME, true);
                break;
            case 2:
                message("%s%s and thee also %s!\n", welcome, pcName.c_str(),
                        party->member(1)->getName().c_str());
                soundSpeakLine(VOICE_LB, LINE_WELCOME, true);
                break;
            default:
                message("%s%s and thy worthy Adventurers!\n",
                        welcome, pcName.c_str());
                soundSpeakLine(VOICE_LB, LINE_WELCOME_ADV, true);
                break;
        }

        // Check levels here, just like the original!
        lordBritishCheckLevels();

        soundSpeakLine(VOICE_LB, LINE_ASK_OF_ME);
        message("\nWhat would thou ask of me?\n");
    } else {
        c->saveGame->lbintro = 1;

        soundSpeakLine(VOICE_LB, LINE_AT_LAST);
        message("\n\n\nLord British rises and says: At long last!\n%s"
                " thou hast come!  We have waited such a long, long time...\n",
                pcName.c_str());
        anyKey.waitTimeout();
        soundSpeakLine(VOICE_LB, LINE_NEW_AGE);
        message("\n\nLord British sits and says: A new age is upon Britannia."
                " The great evil Lords are gone but our people lack direction"
                " and purpose in their lives...\n");
        anyKey.wait();      // There's no timeout here in the DOS version.
        soundSpeakLine(VOICE_LB, LINE_CHAMPION);
        message("A champion of virtue is called for. Thou may be this champion,"                " but only time shall tell.  I will aid thee any way that I"
                " can!\n"
                "How may I help thee?\n");
    }

    while (xu4.stage == StagePlay) {
        input = gameGetInput(16);
        screenCrLf();
        in = input.c_str();
        if (input.empty() || strncasecmp("bye", in, 3) == 0) {
            int plural = (c->party->size() > 1) ? 1 : 0;
            soundSpeakLine(VOICE_LB, LINE_FAREWELL + plural);
            message("\nLord British says: Fare thee well my friend%s!\n",
                    plural ? "s" : "");
            break;
        }

        // Check main keywords.
        for (k = 0; k < LB_KEY_COUNT; ++k) {
            if (strncasecmp(strings + lb->keyword[k], in, 4) == 0)
                break;
        }
        if (k < LB_KEY_COUNT) {
            int l = lbKeyLine[k];
            if (l < LINE_WELCOME)
                soundSpeakLine(VOICE_LB, l);
            messageParts(&anyKey, strings + lb->text[k]);
        } else if (inputEq("help")) {
            lordBritishHelp(&anyKey);
        } else if (inputEq("heal")) {
            soundSpeakLine(VOICE_LB, LINE_I_AM_WELL);
            message("\n\n\n\n\n\nHe says: I am\nwell, thank ye.\n"
                    "\nHe asks: Art thou well? ");
            lordBritishHeal(ReadChoiceController::get("yn \n\033"));
        } else {
            soundSpeakLine(VOICE_LB, LINE_CANNOT_HELP);
            message("\nHe says: I cannot help thee with that.\n");
        }

        message("\nWhat else?\n");
    }

    musicPlayLocale();
}

static void runTalkHawkwind(const U4TalkHawkwind* hw)
{
    AnyKeyController anyKey;
    std::string input;
    const char* strings = hw->strings;
    const char* in;
    int prompt = HW_PROMPT_FIRST;
    int v;

#define HW_STRING(V)    strings + hw->text[V]

    const PartyMember* pc = c->party->member(0);
    input = pc->getName();
    const char* pcName = input.c_str();

    if (pc->isDisabled()) {
        message("%s%s%s%s%s", HW_STRING(HW_SPEAKONLYWITH), pcName,
                              HW_STRING(HW_RETURNWHEN), pcName,
                              HW_STRING(HW_ISREVIVED));
        return;
    }

    musicPlay(MUSIC_SHOPPING);
    c->party->adjustKarma(KA_HAWKWIND);

    soundSpeakLine(VOICE_HW, HW_GREETING);
    message("%s%s%s", HW_STRING(HW_WELCOME), pcName, HW_STRING(HW_GREETING));
    anyKey.wait();

    while (xu4.stage == StagePlay) {
        soundSpeakLine(VOICE_HW, prompt);
        message(HW_STRING(prompt));
        prompt = HW_PROMPT;
        input = gameGetInput(16);
        in = input.c_str();
        if (input.empty() || strncasecmp("bye", in, 3) == 0 ) {
            soundSpeakLine(VOICE_HW, HW_FAREWELL);
            message(HW_STRING(HW_FAREWELL));
            break;
        }

        /* check if asking about a virtue */
        for (v = 0; v < VIRT_MAX; v++) {
            if (inputEq(getVirtueName((Virtue) v)))
                break;
        }
        if (v < VIRT_MAX) {
            int virtueLevel = c->saveGame->karma[v];
            if (virtueLevel == 0) {
                message("\n\n%s\n", HW_STRING(HW_ALREADYAVATAR));
                soundSpeakLine(VOICE_HW, HW_ALREADYAVATAR, true);
            } else if (virtueLevel < 80) {
                v += (virtueLevel / 20) * 8;
                message("\n\n%s", HW_STRING(v));
                soundSpeakLine(VOICE_HW, v, true);
            } else if (virtueLevel < 99) {
                v += 3 * 8;
                message("\n\n%s", HW_STRING(v));
                soundSpeakLine(VOICE_HW, v, true);
            } else { /* virtueLevel >= 99 */
                soundSpeakLine(VOICE_HW, HW_GOTOSHRINE);
                message("\n\n%s\n%s", HW_STRING(4 * 8 + v),
                                      HW_STRING(HW_GOTOSHRINE));
                anyKey.wait();
                // FIXME: Remove extra LF here before the prompt.
            }
            screenCrLf();
        } else {
            message("\n%s", HW_STRING(HW_DEFAULT));
            soundSpeakLine(VOICE_HW, HW_DEFAULT, true);
        }
    }

    musicPlayLocale();
}

bool talkRunU4Castle(const Discourse* disc, int conv, Person* person)
{
    const void* tlk = ((const void**) disc->conv.table)[conv];
    if (tlk) {
        switch (conv) {
            case 0:
                runTalkLordBritish((const U4TalkLordBritish*) tlk);
                return true;
            case 1:
                runTalkHawkwind((const U4TalkHawkwind*) tlk);
                pauseFollow(person);
                return true;
        }
    }
    return false;
}
