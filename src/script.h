/**
 * $Id$
 */

#ifndef SCRIPT_H
#define SCRIPT_H

#include <string>
#include <map>

#include "types.h"
#include "xml.h"

using std::string;

/**
 * An xml-scripting class. It loads and runs xml scripts that
 * take information and interact with the game environment itself.
 * Currently, it is only useful for writing vendor code; however,
 * with some additions it should be possible to write any kind of
 * game script that can be run from within the game.
 *
 * @todo
 * <ul>
 *      <li>Add variable-support and strip vendor-specific code from the script</li>
 *      <li>Fill in some of the missing integration with the game</li>
 * </ul>
 */ 
class Script {
private:
    /**
     * A class that represents a script variable
     */ 
    class Variable {
    public:
        Variable(const int &v);
        Variable(const string &v);

        int&    getInt();
        string& getString();
        
        void    setValue(const int &v);
        void    setValue(const string &v);

        bool    isInt() const;
        bool    isString() const;
    
    private:
        int i_val;
        string s_val;
    };

public:
    /**
     * A script return code
     */ 
    enum ReturnCode {
        RET_OK,
        RET_REDIRECTED,
        RET_STOP
    };

    /**
     * The current state of the script
     */ 
    enum State {
        STATE_UNLOADED,
        STATE_NORMAL,
        STATE_CHOICE,
        STATE_DONE,
        STATE_WAIT_FOR_KEYPRESS,
        STATE_INPUT,
        STATE_INPUT_QUANTITY,
        STATE_INPUT_PRICE,
        STATE_INPUT_TEXT,
        STATE_INPUT_PLAYER
    };

    /**
     * The type of input the script is requesting
     */ 
    /*enum InputType {
        INPUT_NUMBER,
        INPUT_STRING,
        INPUT_DIRECTION,
        INPUT_PLAYER
    };*/

    /**
     * The action that the script is taking
     */ 
    enum Action {
        ACTION_CHOICE,
        ACTION_END,
        ACTION_REDIRECT,
        ACTION_WAIT_FOR_KEY,
        ACTION_WAIT,
        ACTION_STOP,
        ACTION_INCLUDE,
        ACTION_FOR_LOOP,
        ACTION_RANDOM,
        ACTION_MOVE,
        ACTION_SLEEP,
        ACTION_CURSOR,
        ACTION_PAY,
        ACTION_IF,
        ACTION_INPUT,
        ACTION_ADD,
        ACTION_LOSE,
        ACTION_HEAL,
        ACTION_CAST_SPELL,
        ACTION_DAMAGE,
        ACTION_KARMA,
        ACTION_MUSIC,
        ACTION_SAVE_CHOICE,
        ACTION_SET_CHOICE,
        ACTION_SET_QUANTITY,
        ACTION_SET_PRICE,
        ACTION_SET_PLAYER,
        ACTION_ZTATS
    };

    Script();
    ~Script();

    bool load(string filename, string baseId, string subNodeName = "", string subNodeId = "");
    void unload();
    void run(string script);
    ReturnCode execute(xmlNodePtr script, xmlNodePtr currentItem = NULL, string *output = NULL);    
    void _continue();
    
    void resetState();
    void setState(State state);
    State getState();
    
    void setTarget(string val);
    void setChoice(char val);
    void setChoice(string val);
    void setChoices(string val); 
    void setQuantity(int val);
    void setPrice(int val);
    void setPlayer(int val);

    string getTarget();
    string getChoice();
    string getChoices();
    int getQuantity();
    int getPrice();
    int getPlayer();

private:
    void        translate(string *script);
    xmlNodePtr  find(xmlNodePtr node, string script, string choice = "", bool _default = false);    
    string      getPropAsStr(xmlNodePtr node, string prop, bool recursive = false);
    int         getPropAsInt(xmlNodePtr node, string prop, bool recursive = false);
    string      getContent(xmlNodePtr node);   

    /*
     * Action Functions
     */ 
    ReturnCode choice(xmlNodePtr script, xmlNodePtr current);
    ReturnCode end(xmlNodePtr script, xmlNodePtr current);
    ReturnCode waitForKeypress(xmlNodePtr script, xmlNodePtr current);
    ReturnCode redirect(xmlNodePtr script, xmlNodePtr current);
    ReturnCode include(xmlNodePtr script, xmlNodePtr current);
    ReturnCode wait(xmlNodePtr script, xmlNodePtr current);
    ReturnCode forLoop(xmlNodePtr script, xmlNodePtr current);
    ReturnCode random(xmlNodePtr script, xmlNodePtr current);
    ReturnCode move(xmlNodePtr script, xmlNodePtr current);
    ReturnCode sleep(xmlNodePtr script, xmlNodePtr current);
    ReturnCode cursor(xmlNodePtr script, xmlNodePtr current);
    ReturnCode pay(xmlNodePtr script, xmlNodePtr current);
    ReturnCode _if(xmlNodePtr script, xmlNodePtr current);
    ReturnCode input(xmlNodePtr script, xmlNodePtr current);
    ReturnCode add(xmlNodePtr script, xmlNodePtr current);
    ReturnCode lose(xmlNodePtr script, xmlNodePtr current);
    ReturnCode heal(xmlNodePtr script, xmlNodePtr current);
    ReturnCode castSpell(xmlNodePtr script, xmlNodePtr current);
    ReturnCode damage(xmlNodePtr script, xmlNodePtr current);
    ReturnCode karma(xmlNodePtr script, xmlNodePtr current);
    ReturnCode music(xmlNodePtr script, xmlNodePtr current);
    ReturnCode saveChoice(xmlNodePtr script, xmlNodePtr current);
    ReturnCode setChoice(xmlNodePtr script, xmlNodePtr current);
    ReturnCode setQuantity(xmlNodePtr script, xmlNodePtr current);
    ReturnCode setPrice(xmlNodePtr script, xmlNodePtr current);
    ReturnCode setPlayer(xmlNodePtr script, xmlNodePtr current);
    ReturnCode ztats(xmlNodePtr script, xmlNodePtr current);

    /*
     * Math and comparison functions
     */
    void mathParseChildren(xmlNodePtr math, string *result);
    int mathValue(string str);
    int math(int lval, int rval, string &op);
    bool mathParse(string str, int *lval, int *rval, string *op);
    void parseOperation(string str, string *lval, string *rval, string *op);
    bool compare(string str);
    void funcParse(string str, string *funcName, string *contents);

    /*
     * Static variables
     */
private:
    typedef std::map<string, Action> ActionMap;
    static ActionMap action_map;

private:
    xmlDocPtr vendorScriptDoc;
    xmlNodePtr scriptNode;
    FILE *debug;
    
    State state;                    /**< The state the script is in */
    xmlNodePtr currentScript;       /**< The currently running script */
    xmlNodePtr currentItem;         /**< The current position in the script */
    xmlNodePtr translationContext;  /**< The node our translation context resolved to */
    string target;                  /**< The name of a target script */

    string transCtxPropName;        /**< The name of the property that explicitly changes the translation context */
    string itemPropName;            /**< The name of the property that identifies a node name to use to
                                         find a new translation context */
    string idPropName;              /**< The name of the property that uniquely identifies an 'item' node
                                         and is used to find a new translation context */

    string answer;
    string choices;
    string saved_choice;
    int iterator;
    int quant;
    int price;
    int player;

    std::map<string, Variable*> variables;    
};

#endif
