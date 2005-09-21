/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include "conversation.h"
#include "debug.h"
#include "person.h"
#include "script.h"

/* Static variable initialization */
const unsigned int Conversation::BUFFERLEN = 16;

/*
 * Dialogue::Question class
 */
Dialogue::Question::Question(const string &txt, const string &yes, const string &no, Type t) :
    text(txt), yesresp(yes), noresp(no), type(t) {}

string Dialogue::Question::getText() {
    return text;
}

Dialogue::Question::Type Dialogue::Question::getType() const {
    return type;
}

string Dialogue::Question::getResponse(bool yes) {
    if (yes)
        return yesresp;
    return noresp;
}

/*
 * Dialogue::Keyword class
 */ 
Dialogue::Keyword::Keyword(const string &kw, const string &resp, Question *q) :
    keyword(kw), response(resp), question(q) {
    trim(keyword);
    lowercase(keyword);
}

bool Dialogue::Keyword::operator==(const string &kw) const {
    /* minimum 4-character "guessing" */
    int testLen = (keyword.size() < 4) ? keyword.size() : 4;

    if (strncasecmp(kw.c_str(), keyword.c_str(), testLen) == 0)
        return true;
    return false;
}

string              Dialogue::Keyword::getKeyword()             { return keyword; }
string              Dialogue::Keyword::getResponse()            { return response; }
Dialogue::Question* Dialogue::Keyword::getQuestion()            { return question; }
void                Dialogue::Keyword::setQuestion(Question *q) { question = q; }
bool                Dialogue::Keyword::isQuestion() const       { return question != NULL; }

/*
 * Dialogue class 
 */ 

Dialogue::Dialogue() {}
string Dialogue::getName()                    { return name; }
string Dialogue::getPronoun()                 { return pronoun; }
string Dialogue::getDesc()                    { return description; }
void   Dialogue::setName(const string &n)     { name = n; }
void   Dialogue::setPronoun(const string &pn) { pronoun = pn; }
void   Dialogue::setDesc(const string &d)     { description = d; }
void   Dialogue::setTurnAwayProb(int prob)    { turnAwayProb = prob; }
void   Dialogue::addKeyword(Keyword *kw)      { keywords[kw->getKeyword()] = kw; }

Dialogue::Keyword *Dialogue::operator[](const string &kw) {
    KeywordMap::iterator i = keywords.find(kw);
    
    // If they entered the keyword verbatim, return it!
    if (i != keywords.end())
        return i->second;
    // Otherwise, go find one that fits the description.
    else {            
        for (i = keywords.begin(); i != keywords.end(); i++) {                
            if ((*i->second) == kw)
                return i->second;
        }            
    }
    return NULL;
}

Dialogue::Action Dialogue::getAction() const { 
    int prob = xu4_random(0x100);

    /* Does the person turn away from/attack you? */
    if (prob >= turnAwayProb)
        return NO_ACTION;
    else {
        if (attackProb - prob < 0x40)
            return TURN_AWAY;
        else
            return ATTACK;
    }
}

/*
 * Conversation class 
 */ 


Conversation::Conversation() : state(INTRO), script(new Script()) {
    logger = new Debug("debug/conversation.txt", "Conversation"); 
}

Conversation::~Conversation() {
    delete script;
}

Conversation::InputType Conversation::getInputRequired(int *bufferlen) {    
    switch (state) {
    case BUY_QUANTITY:
    case SELL_QUANTITY:
        {
            *bufferlen = 2;
            return INPUT_STRING;
        }

    case TALK:
    case BUY_PRICE:
    case TOPIC:
        {
            *bufferlen = BUFFERLEN;
            return INPUT_STRING;
        }

    case GIVEBEGGAR:
        {
            *bufferlen = 2;
            return INPUT_STRING;
        }

    case ASK:
    case ASKYESNO:
        {
            *bufferlen = 3;
            return INPUT_STRING;
        }

    case VENDORQUESTION:
    case BUY_ITEM:
    case SELL_ITEM:
    case CONFIRMATION:
    case CONTINUEQUESTION:
    case PLAYER:
        return INPUT_CHARACTER;

    case ATTACK:
    case DONE:
    case INTRO:
    case FULLHEAL:
    case ADVANCELEVELS:
        return INPUT_NONE;
    }

    ASSERT(0, "invalid state: %d", state);
    return INPUT_NONE;
}

