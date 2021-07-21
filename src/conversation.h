/*
 * conversation.h
 */

#ifndef CONVERSATION_H
#define CONVERSATION_H

#include <list>
#include <map>
#include <string>
#include <vector>

#include "utils.h"

using std::list;
using std::string;
using std::vector;

class Debug;
class Person;
class Script;

enum ResponseCommand {
    RC_NONE,
    RC_ASK,
    RC_END,
    RC_ATTACK,
    RC_BRAGGED,
    RC_HUMBLE,
    RC_ADVANCELEVELS,
    RC_HEALCONFIRM,
    RC_STARTMUSIC_LB,
    RC_STARTMUSIC_HW,
    RC_STOPMUSIC,
    RC_HAWKWIND
};

/**
 * A response part can be text or a "command" that triggers an
 * action.
 */
class ResponsePart {
public:
    enum { MaxCommand = 2 };

    ResponsePart(const string& text);

    const string& text() const { return value; }
    int command(int n) const { return cmd[n]; }

    bool operator==(const ResponsePart &rhs) const;

private:
    string value;
    uint16_t cmd[ MaxCommand ];

    friend class Response;
};

/**
 * A static response.  Each response can be made up of any number of
 * ResponseParts, which are either text fragments or commands.
 */
class Response {
public:
    Response();
    Response(const string &response);
    virtual ~Response() {}

    void add(const ResponsePart &part);
    void setText(const string&);
    void setCommand(int command, int command2 = RC_NONE);

    virtual const vector<ResponsePart>& getParts();

    operator string() const;

    Response *addref();
    void release();

private:
    int references;
    vector<ResponsePart> parts;

    friend class DynamicResponse;
};

/**
 * A dynamically generated response.  This class allows the response
 * to be generated dynamically at the time of the conversation instead
 * of when the conversation data is loaded.
 */
class DynamicResponse : public Response {
public:
    DynamicResponse(Response* (*generator)(DynamicResponse*),
                    const string &param = "");

    virtual const vector<ResponsePart>& getParts();

    const string &getParam() const { return param; }

private:
    Response* (*generator)(DynamicResponse*);
    string param;
};

/**
 * The dialogue class, which holds conversation information for
 * townspeople and others who may talk to you.  It includes information
 * like pronouns, keywords, actual conversation text (of course),
 * questions, and what happens when you answer these questions.
 */
class Dialogue {
public:
    /**
     * A question-response to a keyword.
     */
    class Question {
    public:
        Question() : yes(NULL), no(NULL) {}
        ~Question() {
            delete yes;
            delete no;
        }
        void assign(const string &txt, Response *resYes, Response *resNo) {
            text = txt;
            yes  = resYes;
            no   = resNo;
        }
        Response* getResponse(bool getYes) const {
            return getYes ? yes : no;
        }

        string text;

    private:
        Response *yes, *no;
    };

    /**
     * A dialogue keyword.
     * It contains all the keywords that the talker will respond to, as
     * well as the responses to those keywords.
     */
    class Keyword {
    public:
        Keyword(const string &kw, Response *resp);
        Keyword(const string &kw, const string &resp);
        ~Keyword();

        bool operator==(const string &kw) const;

        /*
         * Accessor methods
         */
        const string &getKeyword()  {return keyword;}
        Response *getResponse()     {return response;}

    private:
        string keyword;
        Response *response;
    };

    /**
     * A mapping of keywords to the Keyword object that represents them
     */
    typedef std::map<string, Keyword*> KeywordMap;

    /*
     * Constructors/Destructors
     */
    Dialogue();
    virtual ~Dialogue();

    /*
     * Accessor methods
     */
    const string &getName() const                   {return name;}
    const string &getPronoun() const                {return pronoun;}
    const string &getPrompt() const                 {return prompt;}
    Response *getIntro(bool familiar = false)       {return intro;}
    Response *getLongIntro(bool familiar = false)   {return longIntro;}
    Response *getDefaultAnswer()                    {return defaultAnswer;}
    const Dialogue::Question* getQuestion() const   {return &question;}

    /*
     * Setters
     */
    void setName(const string &n)       {name           = n;}
    void setPronoun(const string &pn)   {pronoun        = pn;}
    void setPrompt(const string &prompt){this->prompt   = prompt;}
    void setIntro(Response *i)          {intro          = i;}
    void setLongIntro(Response *i)      {longIntro      = i;}
    void setDefaultAnswer(Response *a)  {defaultAnswer  = a;}
    void setTurnAwayProb(int prob)      {turnAwayProb   = prob;}
    void setQuestion(const string &txt, Response *yes, Response *no) {
        question.assign(txt, yes, no);
    }
    void addKeyword(const string &kw, Response *response);

    ResponseCommand getAction() const;
    string dump(const string &arg);

    /*
     * Operators
     */
    Keyword *operator[](const string &kw);

private:
    string name;
    string pronoun;
    string prompt;
    Response *intro;
    Response *longIntro;
    Response *defaultAnswer;
    KeywordMap keywords;
    union {
        int turnAwayProb;
        int attackProb;
    };
    Question question;
};

/**
 * The conversation class, which handles the flow of text from the
 * player to the talker and vice-versa.  It is responsible for beginning
 * and termination conversations and handing state changes during.
 */
class Conversation {
public:
    /** Different states the conversation may be in */
    enum State {
        INTRO,                  /**< The initial state of the conversation, before anything is said */
        TALK,                   /**< The "default" state of the conversation */
        ASK,                    /**< The talker is asking the player a question */
        ASKYESNO,               /**< The talker is asking the player a yes/no question */
        VENDORQUESTION,         /**< A vendor is asking the player a question */
        BUY_ITEM,               /**< Asked which item to buy */
        SELL_ITEM,              /**< Asked which item to sell */
        BUY_QUANTITY,           /**< Asked how many items to buy */
        SELL_QUANTITY,          /**< Asked how many items to sell */
        BUY_PRICE,              /**< Asked how much money to give someone */
        CONFIRMATION,           /**< Asked by a vendor to confirm something */
        CONTINUEQUESTION,       /**< Asked whether or not to continue */
        TOPIC,                  /**< Asked a topic to speak about */
        PLAYER,                 /**< Input for which player is required */
        FULLHEAL,               /**< Heal the entire party before continuing conversation */
        ADVANCELEVELS,          /**< Check and advance the party's levels before continuing */
        GIVEBEGGAR,             /**< Asked how much to give a beggar */
        ATTACK,                 /**< The conversation ends with the talker attacking you */
        DONE                    /**< The conversation is over */
    };

    /** Different types of conversation input required */
    enum InputType {
        INPUT_STRING,
        INPUT_CHARACTER,
        INPUT_NONE
    };

    /* Constructor/Destructors */
    Conversation();
    ~Conversation();

    /* Member functions */
    InputType getInputRequired(int *bufferLen);

    /* Static variables */
    static const unsigned int BUFFERLEN;    /**< The default maxixum length of input */

private:
    Debug *logger;
public:
    State state;                /**< The state of the conversation */
    string playerInput;         /**< A string holding the text the player inputs */
    list<string> reply;         /**< What the talker says */
#ifndef USE_BORON
    class Script *script;       /**< A script that this person follows during the conversation (may be NULL) */
#endif
    const Dialogue::Question *question; /**< The current question the player is being asked */
    int quant;                  /**< For vendor transactions */
    int player;                 /**< For vendor transactions */
    int price;                  /**< For vendor transactions */
};

#endif
