/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <cctype>
#include <string>
#include <vector>

#include "u4.h"

#include "person.h"

#include "city.h"
#include "context.h"
#include "conversation.h"
#include "debug.h"
#include "event.h"
#include "game.h"   // Included for ReadPlayerController
#include "io.h"
#include "location.h"
#include "music.h"
#include "names.h"
#include "player.h"
#include "savegame.h"
#include "settings.h"
#include "stats.h"
#include "types.h"
#include "u4file.h"
#include "utils.h"
#include "script.h"

using namespace std;

vector<string> hawkwindText;

/* Hawkwind text indexes */
#define HW_SPEAKONLYWITH 40
#define HW_RETURNWHEN 41
#define HW_ISREVIVED 42
#define HW_WELCOME 43
#define HW_GREETING1 44
#define HW_GREETING2 45
#define HW_PROMPT 46
#define HW_DEFAULT 49
#define HW_ALREADYAVATAR 50
#define HW_GOTOSHRINE 51
#define HW_BYE 52

/**
 * Returns true of the object that 'punknown' points
 * to is a person object
 */ 
bool isPerson(Object *punknown) {
    Person *p;
    if ((p = dynamic_cast<Person*>(punknown)) != NULL)
        return true;
    else
        return false;
}

void personGetQuestion(Conversation *cnv, string *question);

int chars_needed(const char *s, int columnmax, int linesdesired, int *real_lines);


/**
 * Splits a piece of response text into screen-sized chunks.
 */
list<string> replySplit(const string &text) {
    string str = text;
    int pos, real_lines;
    list<string> reply;

    /* skip over any initial newlines */
    if ((pos = str.find("\n\n")) == 0)
        str = str.substr(pos+2);
    
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
            str = str.substr(pos+2);

        while (num_chars != str.length()) {
            /* go to the rest of the text */
            str = str.substr(num_chars);
            /* skip over any initial newlines */
            if ((pos = str.find("\n\n")) == 0)
                str = str.substr(pos+2);            

            /* find the next chunk and add it */
            num_chars = chars_needed(str.c_str(), TEXT_AREA_W, TEXT_AREA_H, &real_lines);
            pre = str.substr(0, num_chars);            

            reply.push_back(pre);
        }
    }
    
    return reply;
}

/**
 * Loads in conversation data for special cases and vendors from
 * avatar.exe.
 */
int personInit() {
    /* FIXME: needs a personDelete() function to cleanup allocated memory */
    U4FILE *avatar;

    avatar = u4fopen("avatar.exe");
    if (!avatar)
        return 0;

    hawkwindText = u4read_stringtable(avatar, 74729, 53);

    u4fclose(avatar);
    return 1;
}

Person::Person(MapTile tile) : Creature(tile), start(0, 0) {
    setType(Object::PERSON);
    dialogue = NULL;
    npcType = NPC_EMPTY;
}

Person::Person(const Person *p) : Creature(p->tile) {
    *this = *p;
}

bool Person::canConverse() const {
    return 
        isVendor() ||
        npcType == NPC_HAWKWIND ||
        dialogue != NULL;
}

bool Person::isVendor() const {
    return
        npcType >= NPC_VENDOR_WEAPONS &&
        npcType <= NPC_VENDOR_STABLE;
}

string Person::getName() const {
    if (dialogue)
        return dialogue->getName();
    else
        return "(unnamed person)";
}

void Person::goToStartLocation() {
    setCoords(start);
}

void Person::setDialogue(Dialogue *d) {
    dialogue = d;
    if (tile.getTileType()->getName() == "beggar")
        npcType = NPC_TALKER_BEGGAR;
    else if (tile.getTileType()->getName() == "guard")
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

    text = "\n\n\n";

    /*
     * a convsation with a vendor
     */
    if (isVendor()) {
        static const string ids[] = { 
            "Weapons", "Armor", "Food", "Tavern", "Reagents", "Healer", "Inn", "Guild", "Stable"
        };
        Script *script = cnv->script;

        text.erase();        

        /**
         * We aren't currently running a script, load the appropriate one!
         */ 
        if (cnv->state == Conversation::INTRO) {
            // unload the previous script if it wasn't already unloaded
            if (script->getState() != Script::STATE_UNLOADED)
                script->unload();
            script->load("vendorScript.xml", ids[npcType - NPC_VENDOR_WEAPONS], "vendor", c->location->map->getName());
            script->run("intro");       
               
            while (script->getState() != Script::STATE_DONE) {
                // Gather input for the script
                if (script->getState() == Script::STATE_INPUT) {
                    switch(script->getInputType()) {
                    case Script::INPUT_CHOICE: {
                        // Get choice
                        char val = ReadChoiceController::get(script->getChoices());
                        if (isspace(val) || val == '\033')
                            script->unsetVar(script->getInputName());
                        else {
                            string s_val;
                            s_val.resize(1);
                            s_val[0] = val;
                            script->setVar(script->getInputName(), s_val);
                        }
                    } break;

                    case Script::INPUT_KEYPRESS:
                        ReadChoiceController::get(" \015\033");
                        break;
                        
                    case Script::INPUT_NUMBER: {
                        int val = ReadIntController::get(script->getInputMaxLen(), TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line);
                        script->setVar(script->getInputName(), val);
                    } break;

                    case Script::INPUT_STRING: {
                        string str = ReadStringController::get(script->getInputMaxLen(), TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line);
                        if (str.size()) {
                            lowercase(str);                        
                            script->setVar(script->getInputName(), str);
                        }
                        else script->unsetVar(script->getInputName());
                    } break;                    
                    
                    case Script::INPUT_PLAYER: {
                        ReadPlayerController getPlayerCtrl;
                        eventHandler->pushController(&getPlayerCtrl);
                        int player = getPlayerCtrl.waitFor();
                        if (player != -1) {                            
                            string player_str = to_string(player+1);
                            script->setVar(script->getInputName(), player_str);
                        }
                        else script->unsetVar(script->getInputName());
                    } break;

                    default: break;
                    } // } switch

                    // Continue running the script!                
                    script->_continue();
                } // } if 
            } // } while
        }
        
        // Unload the script
        script->unload();
        cnv->state = Conversation::DONE;
    }

    /*
     * a conversation with a non-vendor
     */
    else {
        switch (cnv->state) {
        case Conversation::INTRO:
            if (npcType == NPC_EMPTY)
                text = emptyGetIntro(cnv);
            else if (npcType == NPC_LORD_BRITISH)
                text = lordBritishGetIntro(cnv);
            else if (npcType == NPC_HAWKWIND)
                text = hawkwindGetIntro(cnv);
            else
                text = talkerGetIntro(cnv);

            break;

        case Conversation::TALK:
            if (npcType == NPC_LORD_BRITISH)
                text += lordBritishGetResponse(cnv, inquiry);            
            else if (npcType == NPC_HAWKWIND)
                text += hawkwindGetResponse(cnv, inquiry);
            else
                text += talkerGetResponse(cnv, inquiry) + "\n";            
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
    if (npcType == NPC_LORD_BRITISH)
        return lordBritishGetPrompt(cnv);
    else if (npcType == NPC_HAWKWIND)
        return hawkwindGetPrompt(cnv);
    else if (isVendor())
        return "";
    else
        return talkerGetPrompt(cnv);
}

/**
 * Returns the valid keyboard choices for a given conversation.
 */
const char *Person::getChoices(Conversation *cnv) {
    if (isVendor())
        return cnv->script->getChoices().c_str();
    
    switch (cnv->state) {    
    case Conversation::CONFIRMATION:
    case Conversation::CONTINUEQUESTION:
        return "ny\015 \033";

    case Conversation::PLAYER:
        return "012345678\015 \033";

    default:
        ASSERT(0, "invalid state: %d", cnv->state);
    }

    return NULL;
}

string Person::emptyGetIntro(Conversation *cnv) {
    cnv->state = Conversation::DONE;
    return string("Funny, no\nresponse!\n");
}

string Person::talkerGetIntro(Conversation *cnv) {    
    string prompt = getPrompt(cnv);

    // As far as I can tell, about 50% of the time they tell you their
    // name in the introduction
    string intro;
    if (xu4_random(2) == 0)
        intro = dialogue->getIntro();
    else
        intro = dialogue->getLongIntro();

    intro += prompt;

    if (isupper(intro[10]))
        intro[10] = tolower(intro[10]);
    cnv->state = Conversation::TALK;

    return intro;
}

string Person::talkerGetResponse(Conversation *cnv, const char *inquiry) {
    string reply;
    Virtue v;
    Dialogue::Action action = dialogue->getAction();
    
    reply = "\n";
    
    /* Does the person take action during the conversation? */
    switch(action) {
    case Dialogue::END_CONVERSATION:
        reply = dialogue->getPronoun();
        reply += " turns away!\n";
        cnv->state = Conversation::DONE;
        return reply;

    case Dialogue::ATTACK:
        reply = "\n";
        reply += getName();
        reply += " says: On guard! Fool!";
        cnv->state = Conversation::ATTACK;
        return reply;

    case Dialogue::NO_ACTION:
    default:
        break;
    }    
    
    if (inquiry[0] == '\0' ||
        strcasecmp(inquiry, "bye") == 0) {
        reply += "Bye.";
        cnv->state = Conversation::DONE;
    }

    else if (npcType == NPC_TALKER_BEGGAR && strncasecmp(inquiry, "give", 4) == 0) {
        reply.erase();
        cnv->state = Conversation::GIVEBEGGAR;
    }

    else if (strncasecmp(inquiry, "join", 4) == 0 &&
             c->party->canPersonJoin(getName(), &v)) {
        CannotJoinError join = c->party->join(name);

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
        reply = kw->getResponse();
        
        // If it's a question keyword, then show the question.
        if (kw->isQuestion()) {
            cnv->question = kw->getQuestion();
            cnv->state = Conversation::ASK;
        }
    }    

    else if (settings.debug && strncasecmp(inquiry, "dump", 4) == 0) {
        vector<string> words = split(inquiry, " \t");
        if (words.size() <= 1)
            reply = dialogue->dump("");
        else
            reply = dialogue->dump(words[1]);
    }

    else
        reply += dialogue->getDefaultAnswer();

    return reply;
}

string Person::talkerGetQuestionResponse(Conversation *cnv, const char *answer) {
    string reply;
    bool valid = false;
    bool yes;
    char ans = tolower(answer[0]);

    cnv->state = Conversation::TALK;

    if (ans == 'y' || ans == 'n') {
        valid = true;
        yes = ans == 'y';
    }

    if (valid) {
        reply = "\n";
        reply += cnv->question->getResponse(yes);
        if (cnv->question->getType() == Dialogue::Question::HUMILITY_TEST) {
            if (yes)
                c->party->adjustKarma(KA_BRAGGED);
            else
                c->party->adjustKarma(KA_HUMBLE);
        }
    }
    else {
        reply = "Yes or no!";
        cnv->state = Conversation::ASKYESNO;
    }

    return reply;
}

string Person::talkerGetPrompt(Conversation *cnv) {
    string prompt;

    if (cnv->state == Conversation::ASK)
        getQuestion(cnv, &prompt);
    else if (cnv->state == Conversation::GIVEBEGGAR)
        prompt = "How much? ";
    else if (cnv->state != Conversation::ASKYESNO)
        prompt = "\nYour Interest:\n";

    return prompt;
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

string Person::lordBritishGetIntro(Conversation *cnv) {
    string intro;

    musicMgr->lordBritish();
    cnv->state = Conversation::TALK;

    if (c->saveGame->lbintro) {
        if (c->saveGame->members == 1) {
            intro = "\n\n\nLord British\nsays:  Welcome\n";
            intro += c->party->member(0)->getName();
        }
        else if (c->saveGame->members == 2) {
            intro = "\n\nLord British\nsays:  Welcome\n";
            intro += c->party->member(0)->getName() + " and thee also " + c->party->member(1)->getName();            
            intro += "!";
        }
        else {
            intro = "\n\n\nLord British\nsays:  Welcome\n";
            intro += c->party->member(0)->getName();
            intro += " and thy\nworthy\nAdventurers!";
        }

        // Check levels here, just like the original!
        cnv->state = Conversation::ADVANCELEVELS;
    }

    else {
        intro = "\n\n\nLord British rises and says: At long last!\n";
        intro += c->party->member(0)->getName();
        intro += " thou hast come!  We have waited such a long, long time...\n\n";
        intro += "\n\nLord British sits and says: A new age is upon Britannia. The great evil Lords are gone but our people lack direction and purpose in their lives...\n\n\n\n\n";
        intro += "A champion of virtue is called for. Thou may be this champion, but only time shall tell.  I will aid thee any way that I can!\n\n";
        intro += "How may I help thee?\n";        
        c->saveGame->lbintro = 1;
    }

    return intro;
}

string Person::lordBritishGetResponse(Conversation *cnv, const char *inquiry) {
    string reply;

    if (inquiry[0] == '\0' ||
        strcasecmp(inquiry, "bye") == 0) {
        reply = "\nLord British says: Fare thee well my friends!\n";
        cnv->state = Conversation::DONE;
        musicMgr->play();
    }

    else if (strncasecmp(inquiry, "heal", 4) == 0) {
        reply = "\n\n\n\n\n\nHe says: I am\nwell, thank ye.";
        cnv->state = Conversation::CONFIRMATION;
    }

    else if (strncasecmp(inquiry, "help", 4) == 0) {
        reply = "He says: ";
        reply += lordBritishGetHelp(cnv);
    }

    else
        reply = talkerGetResponse(cnv, inquiry);

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

string Person::lordBritishGetPrompt(const Conversation *cnv) {
    string prompt;
    
    if (cnv->state == Conversation::CONFIRMATION)
        prompt = "\n\nHe asks: Art thou well?";
    else
        prompt = "\nWhat else?\n";

    return prompt;
}

/**
 * Generate the appropriate response when the player asks Lord British
 * for help.  The help text depends on the current party status; when
 * one quest item is complete, Lord British provides some direction to
 * the next one.
 */
string Person::lordBritishGetHelp(const Conversation *cnv) {
    int v;
    int fullAvatar, partialAvatar;
    string text;

    /*
     * check whether player is full avatar (in all virtues) or partial
     * avatar (in at least one virtue)
     */
    fullAvatar = 1;
    partialAvatar = 0;
    for (v = 0; v < VIRT_MAX; v++) {
        fullAvatar &= (c->saveGame->karma[v] == 0);
        partialAvatar |= (c->saveGame->karma[v] == 0);
    }

    if (c->saveGame->moves <= 1000) {
        text = "To survive in this hostile land thou must first know thyself! Seek ye to master thy weapons and thy magical ability!\n"
               "\nTake great care in these thy first travels in Britannia.\n"
               "\nUntil thou dost well know thyself, travel not far from the safety of the townes!\n";        
    }

    else if (c->saveGame->members == 1) {
        text = "Travel not the open lands alone. There are many worthy people in the diverse townes whom it would be wise to ask to Join thee!\n"
               "\nBuild thy party unto eight travellers, for only a true leader can win the Quest!\n";        
    }

    else if (c->saveGame->runes == 0) {
        text = "Learn ye the paths of virtue. Seek to gain entry unto the eight shrines!\n"
               "\nFind ye the Runes, needed for entry into each shrine, and learn each chant or \"Mantra\" used to focus thy meditations.\n"
               "\nWithin the Shrines thou shalt learn of the deeds which show thy inner virtue or vice!\n"
               "\nChoose thy path wisely for all thy deeds of good and evil are remembered and can return to hinder thee!\n";        
    }

    else if (!partialAvatar) {
        text = "Visit the Seer Hawkwind often and use his wisdom to help thee prove thy virtue.\n"
               "\nWhen thou art ready, Hawkwind will advise thee to seek the Elevation unto partial Avatarhood in a virtue.\n"
               "\nSeek ye to become a partial Avatar in all eight virtues, for only then shalt thou be ready to seek the codex!\n";        
    }

    else if (c->saveGame->stones == 0) {
        text = "Go ye now into the depths of the dungeons. Therein recover the 8 colored stones from the altar pedestals in the halls of the dungeons.\n"
               "\nFind the uses of these stones for they can help thee in the Abyss!\n";        
    }

    else if (!fullAvatar) {
        text = "Thou art doing very well indeed on the path to Avatarhood! Strive ye to achieve the Elevation in all eight virtues!\n";
    }

    else if ((c->saveGame->items & ITEM_BELL) == 0 ||
             (c->saveGame->items & ITEM_BOOK) == 0 ||
             (c->saveGame->items & ITEM_CANDLE) == 0) {
        text = "Find ye the Bell, Book and Candle!  With these three things, one may enter the Great Stygian Abyss!\n";
    }

    else if ((c->saveGame->items & ITEM_KEY_C) == 0 ||
             (c->saveGame->items & ITEM_KEY_L) == 0 ||
             (c->saveGame->items & ITEM_KEY_T) == 0) {
        text = "Before thou dost enter the Abyss thou shalt need the Key of Three Parts, and the Word of Passage.\n"
               "\nThen might thou enter the Chamber of the Codex of Ultimate Wisdom!\n";
    }

    else {
        text = "Thou dost now seem ready to make the final journey into the dark Abyss! Go only with a party of eight!\n"
               "\nGood Luck, and may the powers of good watch over thee on this thy most perilous endeavor!\n"
               "\nThe hearts and souls of all Britannia go with thee now. Take care, my friend.\n";
    }

    return text;
}

string Person::hawkwindGetIntro(Conversation *cnv) {
    string intro;   

    if (c->party->member(0)->getStatus() == STAT_SLEEPING ||
        c->party->member(0)->getStatus() == STAT_DEAD) {
        intro = hawkwindText[HW_SPEAKONLYWITH] + c->party->member(0)->getName();        
        intro += hawkwindText[HW_RETURNWHEN] + c->party->member(0)->getName();
        intro += hawkwindText[HW_ISREVIVED];
        cnv->state = Conversation::DONE;
    }

    else {
        c->party->adjustKarma(KA_HAWKWIND);

        intro = hawkwindText[HW_WELCOME] + c->party->member(0)->getName();
        intro += hawkwindText[HW_GREETING1] + hawkwindText[HW_GREETING2];
        cnv->state = Conversation::TALK;

        musicMgr->hawkwind();
    }

    return intro;
}

string Person::hawkwindGetResponse(Conversation *cnv, const char *inquiry) {
    string reply;
    int v;
    int virtue = -1, virtueLevel = -1;

    if (inquiry[0] == '\0' || strcasecmp(inquiry, "bye") == 0) {
        reply = hawkwindText[HW_BYE];
        cnv->state = Conversation::DONE;
        musicMgr->play();
        return reply;
    }

    /* check if asking about a virtue */
    for (v = 0; v < VIRT_MAX; v++) {
        if (strncasecmp(inquiry, getVirtueName((Virtue) v), 4) == 0) {
            virtue = v;
            virtueLevel = c->saveGame->karma[v];
        }
    }
    if (virtue != -1) {
        reply = "\n\n";
        if (virtueLevel == 0)            
            reply += hawkwindText[HW_ALREADYAVATAR] + "\n";        
        else if (virtueLevel < 80)            
            reply += hawkwindText[(virtueLevel/20) * 8 + virtue];
        else if (virtueLevel < 99)
            reply += hawkwindText[3 * 8 + virtue];
        else /* virtueLevel >= 99 */
            reply = hawkwindText[4 * 8 + virtue] + hawkwindText[HW_GOTOSHRINE];

        return reply;
    }

    reply = "\n";
    reply += hawkwindText[HW_DEFAULT];
        
    return reply;
}

string Person::hawkwindGetPrompt(const Conversation *cnv) {    
    return hawkwindText[HW_PROMPT];    
}

void Person::getQuestion(Conversation *cnv, string *question) {
    *question = cnv->question->getText();
    *question += "\n\nYou say: ";    
}

/**
 * Returns the number of characters needed to get to 
 * the next line of text (based on column width).
 */
int chars_to_next_line(const char *s, int columnmax) {
    const char *str;
    int chars = -1;

    if (strlen(s) > 0) {
        int lastbreak = columnmax;
        chars = 0;
        for (str = s; *str; str++) {            
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
    if (lines + linecount(text.c_str(), columnmax) <= linesdesired)
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
