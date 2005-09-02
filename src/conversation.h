/*
 * $Id$
 */

#ifndef CONVERSATION_H
#define CONVERSATION_H

#include <list>
#include <map>
#include <string>

#include "utils.h"

using std::list;
using std::string;

class Debug;
class Person;
class Script;

/**
 * The dialogue class, which holds conversation information for
 * townspeople and others who may talk to you.  It includes information
 * like pronouns, keywords, actual conversation text (of course), 
 * questions, and what happens when you answer these questions.
 */
class Dialogue {
public:
    /**
     * The action the talker is taking.
     */ 
    enum Action {
        NO_ACTION,
        TURN_AWAY,
        ATTACK
    };

    /**
     * A question-response to a keyword.
     */ 
    class Question {
    public:
        /** The Type of question that the talker asks */
        enum Type {
            NORMAL,
            HUMILITY_TEST
        };        

        Question(const string &txt, const string &yes, const string &no, Type t);

        string getText();
        Type getType() const;
        string getResponse(bool yes);

    private:
        string text, yesresp, noresp;
        Type type;
    };

    /**
     * A dialogue keyword.
     * It contains all the keywords that the talker will respond to, as
     * well as the responses to those keywords (including question-responses).
     */ 
    class Keyword {
    public:        
        Keyword(const string &kw, const string &resp, Question *q = NULL);
        
        bool operator==(const string &kw) const;
        
        /*
         * Accessor methods
         */
        string      getKeyword();
        string      getResponse();
        Question*   getQuestion();
        void        setQuestion(Question *q);
        
        /*
         * Member functions
         */
        bool        isQuestion() const;

    private:
        string keyword, response;
        Question *question;
    };

    /**
     * A mapping of keywords to the Keyword object that represents them
     */
    typedef std::map<string, Keyword*> KeywordMap;

    /*
     * Constructors/Destructors
     */
    Dialogue();

    /*
     * Accessor methods
     */ 
    string getName();
    string getPronoun();
    string getDesc();
    void   setName(const string &n);
    void   setPronoun(const string &pn);
    void   setDesc(const string &d);
    void   setTurnAwayProb(int prob);
    void   addKeyword(Keyword *kw);

    /*
     * Operators 
     */ 
    Keyword *operator[](const string &kw);

    
    /*
     * Member functions 
     */ 
    Action getAction() const;

private:
    string name;
    string pronoun;
    string description;
    KeywordMap keywords;
    union {
        int turnAwayProb;
        int attackProb;    
    };
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
    bool isValid() const;       
    InputType getInputRequired(int *bufferLen);

    /* Accessor functions */
    Person *getTalker();
    void setTalker(Person *);

    /* Static variables */
    static const unsigned int BUFFERLEN;    /**< The default maxixum length of input */
    
private:
    class Person *talker;       /**< The person object the player is talking with */
    Debug *logger;
public:    
    State state;                /**< The state of the conversation */    
    string playerInput;         /**< A string holding the text the player inputs */
    list<string> reply;         /**< What the talker says */
    class Script *script;       /**< A script that this person follows during the conversation (may be NULL) */
    Dialogue::Question *question; /**< The current question the player is being asked */
    int quant;                  /**< For vendor transactions */
    int player;                 /**< For vendor transactions */
    int price;                  /**< For vendor transactions */
};

#endif
