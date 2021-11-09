/*
 * conversation.cpp
 */

#include <cstring>
#include "conversation.h"
#include "debug.h"
#ifndef USE_BORON
#include "script.h"
#endif
#ifdef IOS
#include "context.h"
#include "ios_helpers.h"
#endif

/* Static variable initialization */
const unsigned int Conversation::BUFFERLEN = 16;

Response::Response() : references(0) {
}

Response::Response(const string &response) : references(0) {
    add(response);
}

void Response::add(const ResponsePart &part) {
    parts.push_back(part);
}

/*
 * Set the text of the last ResponsePart. The commands are not changed.
 * If no parts exist, one is added.
 */
void Response::setText(const string& text) {
    size_t n = parts.size();
    if (n)
        parts[n - 1].value = text;
    else
        parts.push_back(ResponsePart(text));
}

/* Set commands of last ResponsePart. */
void Response::setCommand(int c1, int c2) {
    size_t n = parts.size();
    if (n) {
        ResponsePart& rp = parts[n - 1];
        rp.cmd[0] = c1;
        rp.cmd[1] = c2;
    }
}

const vector<ResponsePart>& Response::getParts() {
    return parts;
}

// For debug output.
Response::operator string() const {
    string result;
    int cmd;
    char buf[12];
    vector<ResponsePart>::const_iterator it;

    for (it = parts.begin(); it != parts.end(); it++) {
        const ResponsePart& rp = *it;
        result.append(rp.text());

        for(int c = 0; c < ResponsePart::MaxCommand; ++c) {
            if ((cmd = rp.command(c)) == RC_NONE)
                break;
            sprintf(buf, "<cmd_%d>", cmd);
            result.append(buf);
        }
    }
    return result;
}

Response *Response::addref() {
    references++;
    return this;
}

void Response::release() {
    references--;
    if (references <= 0)
        delete this;
}

ResponsePart::ResponsePart(const string& text) {
    value = text;
    cmd[0] = cmd[1] = RC_NONE;
}

bool ResponsePart::operator==(const ResponsePart &rhs) const {
    return value == rhs.value;
}

DynamicResponse::DynamicResponse(Response* (*generator)(DynamicResponse *), const string &param) :
    param(param) {
    this->generator = generator;
}

const vector<ResponsePart>& DynamicResponse::getParts() {
    return (*generator)(this)->parts;
}

/*
 * Dialogue::Keyword class
 */
Dialogue::Keyword::Keyword(const string &kw, Response *resp) :
    keyword(kw), response(resp->addref()) {
    trim(keyword);
    lowercase(keyword);
}

Dialogue::Keyword::Keyword(const string &kw, const string &resp) :
    keyword(kw), response((new Response(resp))->addref()) {
    trim(keyword);
    lowercase(keyword);
}

Dialogue::Keyword::~Keyword() {
    response->release();
}

bool Dialogue::Keyword::operator==(const string &kw) const {
    // minimum 4-character "guessing"
    int testLen = (keyword.size() < 4) ? keyword.size() : 4;

    // exception: empty keyword only matches empty string (alias for 'bye')
    if (testLen == 0 && kw.size() > 0)
        return false;

    if (strncasecmp(kw.c_str(), keyword.c_str(), testLen) == 0)
        return true;
    return false;
}

/*
 * Dialogue class
 */

Dialogue::Dialogue()
    : intro(NULL)
    , longIntro(NULL)
    , defaultAnswer(NULL) {
}

Dialogue::~Dialogue() {
    for (KeywordMap::iterator i = keywords.begin(); i != keywords.end(); i++) {
        delete i->second;
    }
    if (intro && (intro != longIntro))
        delete intro;
    delete longIntro;
    delete defaultAnswer;
}

void Dialogue::addKeyword(const string &kw, Response *response) {
    if (keywords.find(kw) != keywords.end())
        delete keywords[kw];

    keywords[kw] = new Keyword(kw, response);
}

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

ResponseCommand Dialogue::getAction() const {
    int prob = xu4_random(0x100);

    /* Does the person turn away from/attack you? */
    if (prob >= turnAwayProb)
        return RC_NONE;

    return (attackProb - prob < 0x40) ? RC_END : RC_ATTACK;
}

string Dialogue::dump(const string &arg) {
    string result;
    if (arg == "") {
        result = "keywords:\n";
        for (KeywordMap::iterator i = keywords.begin(); i != keywords.end(); i++) {
            result += i->first + "\n";
        }
    } else {
        if (keywords.find(arg) != keywords.end())
            result = static_cast<string>(*keywords[arg]->getResponse());
    }

    return result;
}

/*
 * Conversation class
 */

Conversation::Conversation() : logger(0), state(INTRO) {
    logger = new Debug("debug/conversation.txt", "Conversation");
#ifndef USE_BORON
    script = new Script();
#endif
#ifdef IOS
    U4IOS::incrementConversationCount();
#endif

}

Conversation::~Conversation() {
#ifdef IOS
    U4IOS::decrementConversationCount();
#endif
    delete logger;
#ifndef USE_BORON
    delete script;
#endif
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

