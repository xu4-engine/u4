#ifndef SCRIPT_H
#define SCRIPT_H

#include "types.h"
#include "xml.h"

typedef enum {
    SCRIPT_RET_OK,
    SCRIPT_RET_REDIRECTED,
    SCRIPT_RET_STOP
} ScriptReturnCode;

typedef enum {
    SCRIPT_STATE_UNLOADED,
    SCRIPT_STATE_NORMAL,
    SCRIPT_STATE_CHOICE,
    SCRIPT_STATE_DONE,
    SCRIPT_STATE_WAIT_FOR_KEYPRESS,
    SCRIPT_STATE_INPUT_QUANTITY,
    SCRIPT_STATE_INPUT_PRICE,
    SCRIPT_STATE_INPUT_TEXT,
    SCRIPT_STATE_INPUT_PLAYER
} ScriptState;

typedef enum {
    SCRIPT_CHOICE,
    SCRIPT_END,
    SCRIPT_REDIRECT,
    SCRIPT_WAIT_FOR_KEY,
    SCRIPT_WAIT,
    SCRIPT_STOP,
    SCRIPT_INCLUDE,
    SCRIPT_FOR_LOOP,
    SCRIPT_RANDOM,
    SCRIPT_MOVE,
    SCRIPT_SLEEP,
    SCRIPT_CURSOR,
    SCRIPT_PAY,
    SCRIPT_IF,
    SCRIPT_INPUT,
    SCRIPT_ADD,
    SCRIPT_LOSE,
    SCRIPT_HEAL,
    SCRIPT_CAST_SPELL,
    SCRIPT_DAMAGE,
    SCRIPT_KARMA,
    SCRIPT_MUSIC,
    SCRIPT_SAVE_CHOICE,
    SCRIPT_SET_CHOICE,
    SCRIPT_SET_QUANTITY,
    SCRIPT_SET_PRICE,
    SCRIPT_SET_PLAYER,
    SCRIPT_ZTATS
} ScriptAction;

class Script {
public:
    Script();
    ~Script();

    bool load(string filename, string baseId, string subNodeName = "", string subNodeId = "");
    void unload();
    void run(string script);
    ScriptReturnCode execute(xmlNodePtr script, xmlNodePtr currentItem = NULL, string *output = NULL);    
    void _continue();
    
    void resetState();
    void setState(ScriptState state);
    ScriptState getState();
    
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

    /**
     * Action Functions
     */ 
    ScriptReturnCode choice(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode end(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode waitForKeypress(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode redirect(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode include(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode wait(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode forLoop(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode random(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode move(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode sleep(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode cursor(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode pay(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode _if(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode input(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode add(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode lose(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode heal(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode castSpell(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode damage(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode karma(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode music(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode saveChoice(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode setChoice(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode setQuantity(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode setPrice(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode setPlayer(xmlNodePtr script, xmlNodePtr current);
    ScriptReturnCode ztats(xmlNodePtr script, xmlNodePtr current);

    /**
     * Math and comparison functions
     */
    void mathParseChildren(xmlNodePtr math, string *result);
    int mathValue(string str);
    int math(int lval, int rval, string &op);
    bool mathParse(string str, int *lval, int *rval, string *op);
    void parseOperation(string str, string *lval, string *rval, string *op);
    bool compare(string str);
    void funcParse(string str, string *funcName, string *contents);

private:
    xmlDocPtr vendorScriptDoc;
    xmlNodePtr scriptNode;
    FILE *debug;

    ScriptState state;
    xmlNodePtr currentScript;
    xmlNodePtr currentItem;
    xmlNodePtr translationContext;
    string target;
    string answer;
    string choices;
    string saved_choice;
    int iterator;
    int quant;
    int price;
    int player;
};

#endif
