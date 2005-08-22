/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <vector>
#include <string>

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
#include "stats.h"
#include "types.h"
#include "u4file.h"
#include "utils.h"
#include "script.h"

using std::string;
using std::vector;

vector<string> hawkwindText;
vector<string> lbKeywords;
vector<string> lbText;

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

string emptyGetIntro(Conversation *cnv);
string talkerGetIntro(Conversation *cnv);
string talkerGetResponse(Conversation *cnv, const char *inquiry);
string talkerGetQuestionResponse(Conversation *cnv, const char *inquiry);
string talkerGetPrompt(Conversation *cnv);
string beggarGetQuantityResponse(Conversation *cnv, const char *response);
string lordBritishGetIntro(Conversation *cnv);
string lordBritishGetResponse(Conversation *cnv, const char *inquiry);
string lordBritishGetQuestionResponse(Conversation *cnv, const char *answer);
string lordBritishGetPrompt(const Conversation *cnv);
string lordBritishGetHelp(const Conversation *cnv);
string hawkwindGetIntro(Conversation *cnv);
string hawkwindGetResponse(Conversation *cnv, const char *inquiry);
string hawkwindGetPrompt(const Conversation *cnv);
int linecount(const char *s, int columnmax);
int chars_needed(const char *s, int columnmax, int linesdesired, int *real_lines);


/**
 * Splits a piece of response text into screen-sized chunks.
 */
Reply *replyNew(const string &text) {
    string str = text;
    int pos, real_lines;
    Reply *reply;

    reply = new Reply;

    /* skip over any initial newlines */
    if ((pos = str.find("\n\n")) == 0)
        str = str.substr(pos+2);
    
    unsigned int num_chars = chars_needed(str.c_str(), TEXT_AREA_W, TEXT_AREA_H, &real_lines);
    
    /* we only have one chunk, no need to split it up */
    unsigned int len = str.length();
    if (num_chars == len)
        reply->push_back(strdup(str.c_str()));    
    else {
        string pre = str.substr(0, num_chars);        

        /* add the first chunk to the list */
        reply->push_back(strdup(pre.c_str()));
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

            reply->push_back(strdup(pre.c_str()));
        }
    }
    
    return reply;
}

/**
 * Frees a reply.
 */
void replyDelete(Reply *reply) {    
    Reply::iterator current;
    for (current = reply->begin(); current != reply->end(); current++)
        free((void*)*current);
    delete reply;
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

    lbKeywords = u4read_stringtable(avatar, 87581, 24);
    /* There's a \0 in the 19th string so we get a
       spurious 20th entry */
    lbText = u4read_stringtable(avatar, 87754, 25);
    for (int i = 20; i < 24; i++)
        lbText[i] = lbText[i+1];
    lbText.pop_back();

    hawkwindText = u4read_stringtable(avatar, 74729, 53);

    u4fclose(avatar);
    return 1;
}

int personIsVendor(const Person *person) {
    return
        person->npcType >= NPC_VENDOR_WEAPONS &&
        person->npcType <= NPC_VENDOR_STABLE;
}

Reply *personGetConversationText(Conversation *cnv, const char *inquiry) {
    string text;
    Reply *reply;

    text = "\n\n\n";

    /*
     * a convsation with a vendor
     */
    if (personIsVendor(cnv->getTalker())) {
        static const string ids[] = { 
            "Weapons", "Armor", "Food", "Tavern", "Reagents", "Healer", "Inn", "Guild", "Stable"
        };
        Script *script = c->conversation->script;

        text.erase();        

        /**
         * We aren't currently running a script, load the appropriate one!
         */ 
        if (c->conversation->state == Conversation::INTRO) {
            // unload the previous script if it wasn't already unloaded
            if (script->getState() != Script::STATE_UNLOADED)
                script->unload();
            script->load("vendorScript.xml", ids[cnv->getTalker()->npcType - NPC_VENDOR_WEAPONS], "vendor", c->location->map->getName());
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
        c->conversation->state = Conversation::DONE;
    }

    /*
     * a conversation with a non-vendor
     */
    else {
        switch (cnv->state) {
        case Conversation::INTRO:
            if (cnv->getTalker()->npcType == NPC_EMPTY)
                text = emptyGetIntro(cnv);
            else if (cnv->getTalker()->npcType == NPC_LORD_BRITISH)
                text = lordBritishGetIntro(cnv);
            else if (cnv->getTalker()->npcType == NPC_HAWKWIND)
                text = hawkwindGetIntro(cnv);
            else
                text = talkerGetIntro(cnv);

            break;

        case Conversation::TALK:
            if (cnv->getTalker()->npcType == NPC_LORD_BRITISH)
                text += lordBritishGetResponse(cnv, inquiry);            
            else if (cnv->getTalker()->npcType == NPC_HAWKWIND)
                text += hawkwindGetResponse(cnv, inquiry);
            else                
                text += talkerGetResponse(cnv, inquiry) + "\n";            
            break;

        case Conversation::CONFIRMATION:
            ASSERT(cnv->getTalker()->npcType == NPC_LORD_BRITISH, "invalid state: %d", cnv->state);
            text += lordBritishGetQuestionResponse(cnv, inquiry);
            break;

        case Conversation::ASK:
        case Conversation::ASKYESNO:
            ASSERT(cnv->getTalker()->npcType != NPC_HAWKWIND, "invalid state for hawkwind conversation");            
            text += talkerGetQuestionResponse(cnv, inquiry) + "\n";
            break;

        case Conversation::GIVEBEGGAR:
            ASSERT(cnv->getTalker()->npcType == NPC_TALKER_BEGGAR, "invalid npc type: %d", cnv->getTalker()->npcType);
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

    reply = replyNew(text);
    return reply;
}

/**
 * Get the prompt shown after each reply.
 */
string personGetPrompt(Conversation *cnv) {
    if (cnv->getTalker()->npcType == NPC_LORD_BRITISH)
        return lordBritishGetPrompt(cnv);
    else if (cnv->getTalker()->npcType == NPC_HAWKWIND)
        return hawkwindGetPrompt(cnv);
    else if (personIsVendor(cnv->getTalker()))
        return "";
    else
        return talkerGetPrompt(cnv);
}

/**
 * Returns the valid keyboard choices for a given conversation.
 */
const char *personGetChoices(Conversation *cnv) {
    if (personIsVendor(cnv->getTalker()))
        return c->conversation->script->getChoices().c_str();
    
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

string emptyGetIntro(Conversation *cnv) {
    cnv->state = Conversation::DONE;
    return string("Funny, no\nresponse!\n");
}

string talkerGetIntro(Conversation *cnv) {    
    string prompt, intro;
    Dialogue *dlg = cnv->getTalker()->dialogue;

    prompt = personGetPrompt(cnv);

    intro = "\nYou meet ";
    intro += dlg->getDesc();
    intro += "\n";
    
    // As far as I can tell, about 50% of the time they tell you their name
    if (xu4_random(2) == 0) {
        intro += "\n";
        intro += dlg->getPronoun();
        intro += " says: I am ";
        intro += dlg->getName();
        intro += "\n";
    }

    intro += prompt;

    if (isupper(intro[10]))
        intro[10] = tolower(intro[10]);
    cnv->state = Conversation::TALK;

    return intro;
}

string talkerGetResponse(Conversation *cnv, const char *inquiry) {
    string reply;    
    Dialogue *dlg = cnv->getTalker()->dialogue;
    Dialogue::Action action = dlg->getAction();
    
    reply = "\n";
    
    /* Does the person take action during the conversation? */
    switch(action) {
    case Dialogue::TURN_AWAY:
        reply = dlg->getPronoun();
        reply += " turns away!\n";
        cnv->state = Conversation::DONE;
        return reply;

    case Dialogue::ATTACK:
        reply = "\n";
        reply += dlg->getName();
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

    /*
     * Check the talker's custom keywords before the standard keywords
     * like HEAL and LOOK.  This behavior differs from u4dos, but fixes
     * a couple conversation files which have keywords that conflict
     * with the standard ones (e.g. Calabrini in Moonglow has HEAL for
     * healer, which is unreachable in u4dos, but clearly more useful
     * than "Fine." for health).
     */
    else if ((*dlg)[inquiry]) {        
        Dialogue::Keyword *kw = (*dlg)[inquiry];
        reply += kw->getResponse();
        
        // If it's a question keyword, then show the question.
        if (kw->isQuestion()) {            
            cnv->question = kw->getQuestion();
            cnv->state = Conversation::ASK;
        }
    }    

    else if (strncasecmp(inquiry, "look", 4) == 0) {
        reply += "You see ";
        reply += dlg->getDesc();
        if (isupper(reply[8]))
            reply[8] = tolower(reply[8]);
    }

    else if (strncasecmp(inquiry, "name", 4) == 0) {
        reply += dlg->getPronoun();
        reply += " says: I am ";
        reply += dlg->getName();
    }        

    else if (strncasecmp(inquiry, "give", 4) == 0) {
        if (cnv->getTalker()->npcType == NPC_TALKER_BEGGAR) {
            reply.erase();
            cnv->state = Conversation::GIVEBEGGAR;
        } else {
            reply += dlg->getPronoun();
            reply += " says: I do not need thy gold.  Keep it!";
        }
    }

    else if (strncasecmp(inquiry, "join", 4) == 0) {
        Virtue v;

        if (c->party->canPersonJoin(cnv->getTalker()->name, &v)) {

            CannotJoinError join = c->party->join(cnv->getTalker()->name);

            if (join == JOIN_SUCCEEDED) {
                City *city = dynamic_cast<City*>(c->location->map);
                reply += "I am honored to join thee!";
                city->removeObject(cnv->getTalker());
                cnv->state = Conversation::DONE;
            } else {
                reply += "Thou art not ";
                reply += (join == JOIN_NOT_VIRTUOUS) ? getVirtueAdjective(v) : "experienced";
                reply += " enough for me to join thee.";
            }
        } else {
            reply += dlg->getPronoun();
            reply += " says: I cannot join thee.";
        }
    }

    /* 
     * This little easter egg appeared in the Amiga version of Ultima IV.
     * I've never figured out what the number means.
     * "Banjo" Bob Hardy was the programmer for the Amiga version.
     */
    else if (strncasecmp(inquiry, "ojna", 4) == 0) {
        reply += "Hi Banjo Bob!\nYour secret\nnumber is\n4F4A4E0A";
    }

    else
        reply += "That I cannot\nhelp thee with.";

    return reply;
}

string talkerGetQuestionResponse(Conversation *cnv, const char *answer) {
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

string talkerGetPrompt(Conversation *cnv) {
    string prompt;

    if (cnv->state == Conversation::ASK)
        personGetQuestion(cnv, &prompt);
    else if (cnv->state == Conversation::GIVEBEGGAR)
        prompt = "How much? ";
    else if (cnv->state != Conversation::ASKYESNO)
        prompt = "\nYour Interest:\n";

    return prompt;
}

string beggarGetQuantityResponse(Conversation *cnv, const char *response) {
    string reply;

    cnv->quant = (int) strtol(response, NULL, 10);
    cnv->state = Conversation::TALK;

    if (cnv->quant > 0) {
        if (c->party->donate(cnv->quant)) {
            reply = "\n";
            reply += cnv->getTalker()->dialogue->getPronoun();
            reply += " says: Oh Thank thee! I shall never forget thy kindness!\n";
        }

        else
            reply = "\n\nThou hast not that much gold!\n";
    } else
        reply = "\n";

    return reply;
}

string lordBritishGetIntro(Conversation *cnv) {
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

string lordBritishGetResponse(Conversation *cnv, const char *inquiry) {
    string reply;
    int i;

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

    /* since the original game files are a bit sketchy on the 'abyss' keyword,
       let's handle it here just to be safe :) */
    else if (strncasecmp(inquiry, "abys", 4) == 0) {
        reply = "\n\n\n\n\nHe says:\nThe Great Stygian Abyss is the darkest pocket of evil "
                "remaining in Britannia!\n\n\n\n\nIt is said that in the deepest recesses of "
                "the Abyss is the Chamber of the Codex!\n\n\n\nIt is also said that only one "
                "of highest Virtue may enter this Chamber, one such as an Avatar!!!\n";
    }

    else {
        for (i = 0; i < 24; i++) {
            if (strncasecmp(inquiry, lbKeywords[i].c_str(), 4) == 0)
                return lbText[i];
        }
        reply = "\nHe says: I\ncannot help thee\nwith that.\n";
    }

    return reply;
}

string lordBritishGetQuestionResponse(Conversation *cnv, const char *answer) {
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

string lordBritishGetPrompt(const Conversation *cnv) {
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
string lordBritishGetHelp(const Conversation *cnv) {
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

string hawkwindGetIntro(Conversation *cnv) {
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

string hawkwindGetResponse(Conversation *cnv, const char *inquiry) {
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

string hawkwindGetPrompt(const Conversation *cnv) {    
    return hawkwindText[HW_PROMPT];    
}

void personGetQuestion(Conversation *cnv, string *question) {
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
        int lastbreak;
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
int linecount(const char *s, int columnmax) {
    int lines = 0;
    while (strlen(s)) {
        s += chars_to_next_line(s, columnmax);
        if (*s != '\0')
            s++;        
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

/*int chars_needed(const char *s, int columnmax, int linesdesired) {
    int lines = 1;
    int col;
    int charCount = 0;

    col = 0;
    for (; *s; s++) {
        if (*s == '\n' || col >= columnmax) {
            charCount++;
            lines++;
            col = 0;
            if (lines >= linesdesired)
                return charCount;
        }
        else {
            col++;
            charCount++;
        }
    }    

    return charCount;
}
*/
