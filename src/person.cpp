/*
 * $Id$
 */

#include <cstring>
#include <vector>

#include "person.h"

#include "context.h"
#include "conversation.h"
#include "debug.h"
#include "game.h"   // Included for ReadPlayerController
#include "player.h"
#include "settings.h"
#include "u4.h"
#include "xu4.h"

#ifdef USE_BORON
#include "config.h"
#include <boron/boron.h>
#else
#include "script.h"
#endif

#ifdef IOS
#include "ios_helpers.h"
#endif

using namespace std;

int chars_needed(const char *s, int columnmax, int linesdesired, int *real_lines);

/**
 * Returns true of the object that 'punknown' points
 * to is a person object
 */
bool isPerson(const Object *punknown) {
    const Person *p;
    if ((p = dynamic_cast<const Person*>(punknown)) != NULL)
        return true;
    else
        return false;
}

/**
 * Splits a piece of response text into screen-sized chunks.
 */
list<string> replySplit(const string &text) {
    string str = text;
    int pos, real_lines;
    list<string> reply;

    /* skip over any initial newlines */
    if ((pos = str.find("\n\n")) == 0)
        str = str.substr(pos+1);

    unsigned int num_chars = chars_needed(str.c_str(), TEXT_AREA_W, TEXT_AREA_H, &real_lines);

    /* we only have one chunk, no need to split it up */
    unsigned int len = str.length();
    if (num_chars == len)
        reply.push_back(str);
    else {
        string pre = str.substr(0, num_chars);

        /* add the first chunk to the list */
        reply.push_back(pre);
        /* skip over any initial newlines */
        if ((pos = str.find("\n\n")) == 0)
            str = str.substr(pos+1);

        while (num_chars != str.length()) {
            /* go to the rest of the text */
            str = str.substr(num_chars);
            /* skip over any initial newlines */
            if ((pos = str.find("\n\n")) == 0)
                str = str.substr(pos+1);

            /* find the next chunk and add it */
            num_chars = chars_needed(str.c_str(), TEXT_AREA_W, TEXT_AREA_H, &real_lines);
            pre = str.substr(0, num_chars);

            reply.push_back(pre);
        }
    }

    return reply;
}

Person::Person(const MapTile& tile) :
    Creature(Creature::getByTile(tile)),
    start(0, 0)
{
    setType(Object::PERSON);
    dialogue = NULL;
    npcType = NPC_EMPTY;
}

Person::Person(const Person *p) {
    *this = *p;
}

bool Person::canConverse() const {
    return isVendor() || dialogue != NULL;
}

bool Person::isVendor() const {
    return
        npcType >= NPC_VENDOR_WEAPONS &&
        npcType <= NPC_VENDOR_STABLE;
}

string Person::getName() const {
    if (dialogue)
        return dialogue->getName();
    else if (npcType == NPC_EMPTY)
        return Creature::getName();
    else
        return "(unnamed person)";
}

void Person::setDialogue(Dialogue *d) {
    dialogue = d;
    if (tile.getTileType()->name == Tile::sym.beggar)
        npcType = NPC_TALKER_BEGGAR;
    else if (tile.getTileType()->name == Tile::sym.guard)
        npcType = NPC_TALKER_GUARD;
    else
        npcType = NPC_TALKER;
}

void Person::setNpcType(PersonNpcType t) {
    npcType = t;
    ASSERT(!isVendor() || dialogue == NULL, "vendor has dialogue");
}

list<string> Person::getConversationText(Conversation *cnv, const char *inquiry) {
    string text;

    /*
     * a conversation with a vendor
     */
    if (isVendor()) {
#ifdef USE_BORON
        static const char* vendorId[] = {
            "weapons", "armor", "food", "tavern", "reagents",
            "healer", "inn", "guild", "stable"
        };
        if (cnv->state == Conversation::INTRO) {
            // Make a valid Boron word! from names with spaces.
            text = c->location->map->getName();
            replace(text.begin(), text.end(), ' ', '-');

            xu4.config->scriptEvalArg("talk-to %s '%s",
                vendorId[npcType - NPC_VENDOR_WEAPONS], text.c_str());
            text.clear();
        }
#else
        static const string ids[] = {
            "Weapons", "Armor", "Food", "Tavern", "Reagents", "Healer", "Inn", "Guild", "Stable"
        };
        Script *script = cnv->script;

        /**
         * We aren't currently running a script, load the appropriate one!
         */
        if (cnv->state == Conversation::INTRO) {
            script->talkToVendor(ids[npcType - NPC_VENDOR_WEAPONS]);
        }

        // Unload the script
        script->unload();
#endif
        cnv->state = Conversation::DONE;
    }

    /*
     * a conversation with a non-vendor
     */
    else {
        text = "\n\n\n";

        switch (cnv->state) {
        case Conversation::INTRO:
            text = getIntro(cnv);
            break;

        case Conversation::TALK:
            text += getResponse(cnv, inquiry) + "\n";
            break;

        case Conversation::CONFIRMATION:
            ASSERT(npcType == NPC_LORD_BRITISH, "invalid state: %d", cnv->state);
            text += lordBritishGetQuestionResponse(cnv, inquiry);
            break;

        case Conversation::ASK:
        case Conversation::ASKYESNO:
            ASSERT(npcType != NPC_HAWKWIND, "invalid state for hawkwind conversation");
            text += talkerGetQuestionResponse(cnv, inquiry) + "\n";
            break;

        case Conversation::GIVEBEGGAR:
            ASSERT(npcType == NPC_TALKER_BEGGAR, "invalid npc type: %d", npcType);
            text = beggarGetQuantityResponse(cnv, inquiry);
            break;

        case Conversation::FULLHEAL:
        case Conversation::ADVANCELEVELS:
            /* handled elsewhere */
            break;

        default:
            ASSERT(0, "invalid state: %d", cnv->state);
        }
    }

    return replySplit(text);
}

/**
 * Get the prompt shown after each reply.
 */
string Person::getPrompt(Conversation *cnv) {
    if (isVendor())
        return "";

    string prompt;
    if (cnv->state == Conversation::ASK)
        prompt = getQuestion(cnv);
    else if (cnv->state == Conversation::GIVEBEGGAR)
        prompt = "How much? ";
    else if (cnv->state == Conversation::CONFIRMATION)
        prompt = "\n\nHe asks: Art thou well?";
    else if (cnv->state != Conversation::ASKYESNO)
        prompt = dialogue->getPrompt();

    return prompt;
}

string Person::getIntro(Conversation *cnv) {
    if (npcType == NPC_EMPTY) {
        cnv->state = Conversation::DONE;
        return string("Funny, no\nresponse!\n");
    }

    // As far as I can tell, about 50% of the time they tell you their
    // name in the introduction
    Response *intro;
    if (xu4_random(2) == 0)
        intro = dialogue->getIntro();
    else
        intro = dialogue->getLongIntro();

    cnv->state = Conversation::TALK;
    string text = processResponse(cnv, intro);

    return text;
}

string Person::processResponse(Conversation *cnv, Response *response) {
    string text;
    int cmd;
    const vector<ResponsePart> &parts = response->getParts();
    vector<ResponsePart>::const_iterator it;
    for (it = parts.begin(); it != parts.end(); it++) {
        const ResponsePart& rp = *it;
        text += rp.text();

        // Execute any associated command triggers.
        for(int c = 0; c < ResponsePart::MaxCommand; ++c) {
            if ((cmd = rp.command(c)) == RC_NONE)
                break;
            runCommand(cnv, cmd);
        }
    }
    return text;
}

void Person::runCommand(Conversation *cnv, int command) {
    switch (command) {
        case RC_ASK:
            cnv->question = dialogue->getQuestion();
            cnv->state = Conversation::ASK;
            break;
        case RC_END:
            cnv->state = Conversation::DONE;
            break;
        case RC_ATTACK:
            cnv->state = Conversation::ATTACK;
            break;
        case RC_BRAGGED:
            c->party->adjustKarma(KA_BRAGGED);
            break;
        case RC_HUMBLE:
            c->party->adjustKarma(KA_HUMBLE);
            break;
        case RC_ADVANCELEVELS:
            cnv->state = Conversation::ADVANCELEVELS;
            break;
        case RC_HEALCONFIRM:
            cnv->state = Conversation::CONFIRMATION;
            break;
        case RC_STARTMUSIC_LB:
            musicPlay(MUSIC_RULEBRIT);
            break;
        case RC_STARTMUSIC_HW:
            musicPlay(MUSIC_SHOPPING);
            break;
        case RC_STOPMUSIC:
            musicPlayLocale();
            break;
        case RC_HAWKWIND:
            c->party->adjustKarma(KA_HAWKWIND);
            break;
        default:
            ASSERT(0, "Unknown dialogue response command: %d\n", command);
            break;
    }
}

string Person::getResponse(Conversation *cnv, const char *inquiry) {
    string reply;
    Virtue v;
    int cmd = dialogue->getAction();

    reply = "\n";

    /* Does the person take action during the conversation? */
    if (cmd == RC_END) {
        runCommand(cnv, cmd);
        return dialogue->getPronoun() + " turns away!\n";
    }
    if (cmd == RC_ATTACK) {
        runCommand(cnv, cmd);
        return string("\n") + getName() + " says: On guard! Fool!";
    }

    if (npcType == NPC_TALKER_BEGGAR && strncasecmp(inquiry, "give", 4) == 0) {
        reply.erase();
        cnv->state = Conversation::GIVEBEGGAR;
    }

    else if (strncasecmp(inquiry, "join", 4) == 0 &&
             c->party->canPersonJoin(getName(), &v)) {
        CannotJoinError join = c->party->join(getName());

        if (join == JOIN_SUCCEEDED) {
            reply += "I am honored to join thee!";
            c->location->map->removeObject(this);
            cnv->state = Conversation::DONE;
        } else {
            reply += "Thou art not ";
            reply += (join == JOIN_NOT_VIRTUOUS) ? getVirtueAdjective(v) : "experienced";
            reply += " enough for me to join thee.";
        }
    }

    else if ((*dialogue)[inquiry]) {
        Dialogue::Keyword *kw = (*dialogue)[inquiry];

        reply = processResponse(cnv, kw->getResponse());
    }

    else if (xu4.settings->debug && strncasecmp(inquiry, "dump", 4) == 0) {
        vector<string> words = split(inquiry, " \t");
        if (words.size() <= 1)
            reply = dialogue->dump("");
        else
            reply = dialogue->dump(words[1]);
    }

    else
        reply += processResponse(cnv, dialogue->getDefaultAnswer());

    return reply;
}

string Person::talkerGetQuestionResponse(Conversation *cnv, const char *answer) {
    bool valid = false;
    bool yes;
    char ans = tolower(answer[0]);

    if (ans == 'y' || ans == 'n') {
        valid = true;
        yes = (ans == 'y');
    }

    if (!valid) {
        cnv->state = Conversation::ASKYESNO;
        return "Yes or no!";
    }

    cnv->state = Conversation::TALK;
    return "\n" + processResponse(cnv, cnv->question->getResponse(yes));
}

string Person::beggarGetQuantityResponse(Conversation *cnv, const char *response) {
    string reply;

    cnv->quant = (int) strtol(response, NULL, 10);
    cnv->state = Conversation::TALK;

    if (cnv->quant > 0) {
        if (c->party->donate(cnv->quant)) {
            reply = "\n";
            reply += dialogue->getPronoun();
            reply += " says: Oh Thank thee! I shall never forget thy kindness!\n";
        }

        else
            reply = "\n\nThou hast not that much gold!\n";
    } else
        reply = "\n";

    return reply;
}

string Person::lordBritishGetQuestionResponse(Conversation *cnv, const char *answer) {
    string reply;

    cnv->state = Conversation::TALK;

    if (tolower(answer[0]) == 'y') {
        reply = "Y\n\nHe says: That is good.\n";
    }

    else if (tolower(answer[0]) == 'n') {
        reply = "N\n\nHe says: Let me heal thy wounds!\n";
        cnv->state = Conversation::FULLHEAL;
    }

    else
        reply = "\n\nThat I cannot\nhelp thee with.\n";

    return reply;
}

string Person::getQuestion(Conversation *cnv) {
    return "\n" + cnv->question->text + "\n\nYou say: ";
}

/**
 * Returns the number of characters needed to get to
 * the next line of text (based on column width).
 */
int chars_to_next_line(const char *s, int columnmax) {
    int chars = -1;

    if (strlen(s) > 0) {
        int lastbreak = columnmax;
        chars = 0;
        for (const char *str = s; *str; str++) {
            if (*str == '\n')
                return (str - s);
            else if (*str == ' ')
                lastbreak = (str - s);
            else if (++chars >= columnmax)
                return lastbreak;
        }
    }

    return chars;
}

/**
 * Counts the number of lines (of the maximum width given by
 * columnmax) in the string.
 */
int linecount(const string &s, int columnmax) {
    int lines = 0;
    unsigned ch = 0;
    while (ch < s.length()) {
        ch += chars_to_next_line(s.c_str() + ch, columnmax);
        if (ch < s.length())
            ch++;
        lines++;
    }
    return lines;
}

/**
 * Returns the number of characters needed to produce a
 * valid screen of text (given a column width and row height)
 */
int chars_needed(const char *s, int columnmax, int linesdesired, int *real_lines) {
    int chars = 0,
        totalChars = 0;

    char *new_str = strdup(s),
         *str = new_str;

    // try breaking text into paragraphs first
    string text = s;
    string paragraphs;
    unsigned int pos;
    int lines = 0;
    while ((pos = text.find("\n\n")) < text.length()) {
        string p = text.substr(0, pos);
        lines += linecount(p.c_str(), columnmax);
        if (lines <= linesdesired)
            paragraphs += p + "\n";
        else break;
        text = text.substr(pos+1);
    }
    // Seems to be some sort of clang compilation bug in this code, that causes this addition
    // to not work correctly.
    int totalPossibleLines = lines + linecount(text.c_str(), columnmax);
    if (totalPossibleLines <= linesdesired)
        paragraphs += text;

    if (!paragraphs.empty()) {
        *real_lines = lines;
        free(new_str);
        return paragraphs.length();
    }
    else {
        // reset variables and try another way
        lines = 1;
    }
    // gather all the line breaks
    while ((chars = chars_to_next_line(str, columnmax)) >= 0) {
        if (++lines >= linesdesired)
            break;

        int num_to_move = chars;
        if (*(str + num_to_move) == '\n')
            num_to_move++;

        totalChars += num_to_move;
        str += num_to_move;
    }

    free(new_str);

    *real_lines = lines;
    return totalChars;
}
