/**
 * $Id$
 */
    
#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <map>
#include <string>
#include "script.h"

#include "armor.h"
#include "camp.h"
#include "context.h"
#include "debug.h"
#include "error.h"
#include "event.h"
#include "game.h"
#include "music.h"
#include "player.h"
#include "savegame.h"
#include "screen.h"
#include "settings.h"
#include "spell.h"
#include "stats.h"
#include "u4file.h"
#include "utils.h"
#include "weapon.h"
#include "xml.h"

using std::string;

/**
 * Constructor
 */ 
Script::Script() : vendorScriptDoc(NULL), debug(NULL), state(SCRIPT_STATE_UNLOADED) {}
Script::~Script() {
    unload();
}

/**
 * Loads the vendor script
 */ 
bool Script::load(string filename, string baseId, string subNodeName, string subNodeId) {
    xmlNodePtr root, node, child;
    this->state = SCRIPT_STATE_NORMAL;

    /* unload previous script */
    unload();

    /**
     * Open and parse the .xml file
     */ 
    this->vendorScriptDoc = xmlParse(filename.c_str());
    root = xmlDocGetRootElement(vendorScriptDoc);
    if (xmlStrcmp(root->name, (const xmlChar *) "scripts") != 0)
        errorFatal("malformed %s", filename.c_str());

    /**
     * If the script is set to debug, then open our script debug file
     */ 
    if (xmlPropExists(root, "debug")) {
        static const char *dbg_filename = "script_debug.txt";
        // Our script is going to hog all the debug info
        if (xmlGetPropAsBool(root, "debug"))
            debug = fopen(dbg_filename, "wt");
        else {
            // See if we share our debug space with other scripts
            string val = xmlGetPropAsStr(root, "debug");
            if (val == "share")
                debug = fopen(dbg_filename, "at");
        }
    }

    this->currentScript = NULL;
    this->currentItem = NULL;
    this->quant = 1;
    this->price = 1;

    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node) || (xmlStrcmp(node->name, (const xmlChar *) "script") != 0))
            continue;
        
        if (baseId == xmlGetPropAsStr(node, "id")) {
            /**
             * We use the base node as our main script node
             */
            if (subNodeName.empty()) {
                this->scriptNode = node;            
                this->translationContext = node;
                
                if (debug)
                    fprintf(debug, "\n<Loaded script '%s'>\n", baseId.c_str());

                return true;
            }

            for (child = node->xmlChildrenNode; child; child = child->next) {
                if (xmlNodeIsText(child) ||
                    xmlStrcmp(child->name, (const xmlChar *) subNodeName.c_str()) != 0)
                    continue;
         
                string id = xmlGetPropAsStr(child, "id");

                if (id == subNodeId) {                    
                    this->scriptNode = child;                    
                    this->translationContext = child;                                     

                    if (debug)
                        fprintf(debug, "\n<Loaded subscript '%s' where id='%s' for script '%s'>\n", subNodeName.c_str(), subNodeId.c_str(), baseId.c_str());

                    return true;
                }
            }        
        }
    }
    
    if (subNodeName.empty())
        errorFatal("Couldn't find script '%s' in %s", baseId.c_str(), filename.c_str());
    else errorFatal("Couldn't find subscript '%s' where id='%s' in script '%s' in %s", subNodeName.c_str(), subNodeId.c_str(), baseId.c_str(), filename.c_str());

    this->state = SCRIPT_STATE_UNLOADED;

    return false;
}

/**
 * Unload the script
 */ 
void Script::unload() {
    if (vendorScriptDoc) {
        xmlFree(vendorScriptDoc);
        vendorScriptDoc = NULL;
    }

    if (debug) {
        fclose(debug);
        debug = NULL;
    }
}

/**
 * Runs a script after it's been loaded
 */ 
 void Script::run(string script) {
    xmlNodePtr scriptNode;    
    
    scriptNode = find(this->scriptNode, script, this->answer);    

    if (!scriptNode)
        errorFatal("Script '%s' not found in vendorScript.xml", script.c_str());

    execute(scriptNode);
}

/**
 * Executes the subscript 'script' of the main script 'node'
 */ 
ScriptReturnCode Script::execute(xmlNodePtr script, xmlNodePtr currentItem, string *output) {
    xmlNodePtr node = this->scriptNode;
    xmlNodePtr current;
    xmlNodePtr transContext = this->translationContext;
    ScriptReturnCode retval = SCRIPT_RET_OK;

    /**
     * Setup the different script actions that can be taken
     *
     * FIXME: this should probably be initialized in the constructor of its own class
     */ 
    typedef std::map<string, ScriptAction, std::less<string> > ScriptActionMap;
    static ScriptActionMap action_map;

    if (action_map.size() == 0) {        
        action_map["choice"]        = SCRIPT_CHOICE;
        action_map["end"]           = SCRIPT_END;
        action_map["redirect"]      = SCRIPT_REDIRECT;
        action_map["wait_for_keypress"] = SCRIPT_WAIT_FOR_KEY;
        action_map["wait"]          = SCRIPT_WAIT;
        action_map["stop"]          = SCRIPT_STOP;
        action_map["include"]       = SCRIPT_INCLUDE;
        action_map["for"]           = SCRIPT_FOR_LOOP;
        action_map["random"]        = SCRIPT_RANDOM;
        action_map["move"]          = SCRIPT_MOVE;
        action_map["sleep"]         = SCRIPT_SLEEP;
        action_map["cursor"]        = SCRIPT_CURSOR;
        action_map["pay"]           = SCRIPT_PAY;
        action_map["if"]            = SCRIPT_IF;
        action_map["input"]         = SCRIPT_INPUT;
        action_map["add"]           = SCRIPT_ADD;
        action_map["lose"]          = SCRIPT_LOSE;
        action_map["heal"]          = SCRIPT_HEAL;
        action_map["cast_spell"]    = SCRIPT_CAST_SPELL;
        action_map["damage"]        = SCRIPT_DAMAGE;
        action_map["karma"]         = SCRIPT_KARMA;
        action_map["music"]         = SCRIPT_MUSIC;
        action_map["save_choice"]   = SCRIPT_SAVE_CHOICE;
        action_map["set_choice"]    = SCRIPT_SET_CHOICE;
        action_map["set_quantity"]  = SCRIPT_SET_QUANTITY;
        action_map["set_price"]     = SCRIPT_SET_PRICE;
        action_map["set_player"]    = SCRIPT_SET_PLAYER;
        action_map["ztats"]         = SCRIPT_ZTATS;
    }    
    
    /**
     * The "item" property changes the translation context
     * of the script to one of the vendor's child nodes.
     */
    if (xmlPropExists(script, "item")) {
        /* get the choice to search for */
        string choice = this->answer;
        if (xmlPropExists(script, "use_saved_choice") && xmlGetPropAsBool(script, "use_saved_choice"))
            choice = this->saved_choice;

        this->translationContext = find(node, getPropAsStr(script, "item"), choice);
        if (debug) {
            if (!this->translationContext)
                fprintf(debug, "\nWarning!!! Invalid translation context <%s choice=\"%s\" ...>", getPropAsStr(script, "item").c_str(), choice.c_str());
            else fprintf(debug, "\nChanging translation context to <%s choice=\"%s\" ...>\n", getPropAsStr(script, "item").c_str(), choice.c_str());
        }
    }
    
    if (!script->children) {
        /* redirect the script to another node */
        if (xmlPropExists(script, "redirect"))
            retval = redirect(NULL, script);        
        /* end the conversation */
        else {
            if (debug)
                fprintf(debug, "\nA script with no children found (nowhere to go). Ending script...\n");
            screenMessage("\n");            
            this->state = SCRIPT_STATE_DONE;
        }
    }

    /* do we start where we left off, or start from the beginning? */
    if (currentItem) {
        current = currentItem->next;
        if (debug)
            fprintf(debug, "\nReturning to execution from end of '%s' script\n", currentItem->name);
    }
    else current = script->children;
        
    for (; current; current = current->next) {
        string name = (char *)current->name;        
        retval = SCRIPT_RET_OK;
        ScriptActionMap::iterator action;

        /* nothing left to do */
        if (this->state == SCRIPT_STATE_DONE)
            break;

        /* begin execution of script */       

        /**
         * Handle Text
         */
        if (xmlNodeIsText(current)) {
            string content = getContent(current);
            if (output)
                *output += content;
            else screenMessage(content.c_str());

            if (debug && content.length())
                fprintf(debug, "\nOutput: \n====================\n%s\n====================", content.c_str());
        }
        else {
            /**
             * Search for the corresponding action and execute it!
             */ 
            action = action_map.find(name);
            if (action != action_map.end()) {
                /**
                 * Found it!
                 */ 
                switch(action->second) {
                case SCRIPT_CHOICE:         retval = choice(script, current); break;            
                case SCRIPT_END:            retval = end(script, current); break;                    
                case SCRIPT_REDIRECT:       retval = redirect(script, current); break;
                case SCRIPT_WAIT_FOR_KEY:   retval = waitForKeypress(script, current); break;
                case SCRIPT_WAIT:           retval = wait(script, current); break;                    
                case SCRIPT_STOP:           retval = SCRIPT_RET_STOP; break;
                case SCRIPT_INCLUDE:        retval = include(script, current); break;                    
                case SCRIPT_FOR_LOOP:       retval = forLoop(script, current); break;
                case SCRIPT_RANDOM:         retval = random(script, current); break;
                case SCRIPT_MOVE:           retval = move(script, current); break;
                case SCRIPT_SLEEP:          retval = sleep(script, current); break;
                case SCRIPT_CURSOR:         retval = cursor(script, current); break;
                case SCRIPT_PAY:            retval = pay(script, current); break;                    
                case SCRIPT_IF:             retval = _if(script, current); break;
                case SCRIPT_INPUT:          retval = input(script, current); break;
                case SCRIPT_ADD:            retval = add(script, current); break;                    
                case SCRIPT_LOSE:           retval = lose(script, current); break;                    
                case SCRIPT_HEAL:           retval = heal(script, current); break;
                case SCRIPT_CAST_SPELL:     retval = castSpell(script, current); break;                    
                case SCRIPT_DAMAGE:         retval = damage(script, current); break;                    
                case SCRIPT_KARMA:          retval = karma(script, current); break;                    
                case SCRIPT_MUSIC:          retval = music(script, current); break;                    
                case SCRIPT_SAVE_CHOICE:    retval = saveChoice(script, current); break;
                case SCRIPT_SET_CHOICE:     retval = setChoice(script, current); break;
                case SCRIPT_SET_QUANTITY:   retval = setQuantity(script, current); break;
                case SCRIPT_SET_PRICE:      retval = setPrice(script, current); break;
                case SCRIPT_SET_PLAYER:     retval = setPlayer(script, current); break;
                case SCRIPT_ZTATS:          retval = ztats(script, current); break;
                default:
                    
                    break;
                }
            }
            /**
             * Didn't find the corresponding action...
             */ 
            else if (debug)
                 fprintf(debug, "ERROR: '%s' method not found", name.c_str());

            /* The script was redirected or stopped, stop now! */
            if ((retval == SCRIPT_RET_REDIRECTED) || (retval== SCRIPT_RET_STOP))
                break;
        }        
        
        if (debug)
            fprintf(debug, "\n");        
    }

    /**
     * Restore the translation context to the original
     */ 
    this->translationContext = transContext;

    return retval;
}

/**
 * Continues the script from where it left off, or where the last script indicated
 */ 
void Script::_continue() {
    /* reset our script state to normal */
    resetState();
    
    /* there's no target indicated, just start where we left off! */            
    if (target.empty())
        execute(currentScript, currentItem);                
    else run(target);
}

/**
 * Set and retrieve property values
 */ 
void Script::resetState()               { state = SCRIPT_STATE_NORMAL; }
void Script::setState(ScriptState s)    { state = s; }
void Script::setTarget(string val)      { target = val; }
void Script::setChoice(char val)        { answer = val; }
void Script::setChoice(string val)      { answer = val; }
void Script::setChoices(string val)     { choices = val; }
void Script::setQuantity(int val)       { quant = val; }
void Script::setPrice(int val)          { price = val; }
void Script::setPlayer(int val)         { player = val; }

ScriptState Script::getState()          { return state; }
string Script::getTarget()              { return target; }
string Script::getChoice()              { return answer; }
string Script::getChoices()             { return choices; }
int Script::getQuantity()               { return quant; }
int Script::getPrice()                  { return price; }
int Script::getPlayer()                 { return player; }

/**
 * Translates a script string with dynamic variables
 */ 
void Script::translate(string *text) {
    unsigned int pos;      
    bool nochars = true;
    xmlNodePtr node = this->translationContext;
    
    /* determine if the script is completely whitespace */
    for (string::iterator current = text->begin(); current != text->end(); current++) {
        if (isalnum(*current)) {
            nochars = false;
            break;
        }
    }

    /* erase scripts that are composed entirely of whitespace */
    if (nochars)
        text->erase();    

    while ((pos = text->find_first_of("{")) < text->length()) {
        string pre = text->substr(0, pos);
        string post;
        string item = text->substr(pos+1);
        char buffer[16];
        
        /**
         * Handle embedded items
         */
        int num_embedded = 0;
        int total_pos = 0;
        string current = item;
        while (true) {
            unsigned int open = current.find_first_of("{"),
                         close = current.find_first_of("}");

            if (open < 0)
                open = current.length();
            if (close < 0)
                close = current.length();

            if (close == current.length())
                errorFatal("Error: no closing } found in vendor script.");

            if (open < close) {
                num_embedded++;
                total_pos += open + 1;
                current = current.substr(open+1);
            }
            if (close < open) {
                total_pos += close;
                if (num_embedded == 0) {
                    pos = total_pos;
                    break;
                }
                num_embedded--;
                total_pos += 1;
                current = current.substr(close+1);
            }            
        }        
        
        /**
         * Separate the item itself from the pre- and post-data
         */ 
        post = item.substr(pos+1);
        item = item.substr(0, pos);

        if (debug)
            fprintf(debug, "\n{%s} == ", item.c_str());

        /* translate any stuff contained in the item */
        translate(&item);

        string prop;
        
        // Get the current iterator for our loop
        if (item == "iterator") {
            sprintf(buffer, "%d", this->iterator);
            prop = buffer;
        }
        // Get the current choice
        else if (item == "current_choice") {
            prop = this->answer;
            if (prop.empty())
                prop = " ";
        }
        // Get the saved choice
        else if (item == "saved_choice") {
            prop = this->saved_choice;
        }
        // Get the current quantity
        else if (item == "current_quantity") {            
            sprintf(buffer, "%d", this->quant);
            prop = buffer;
        }
        // Get the current price
        else if (item == "current_price") {        
            sprintf(buffer, "%d", this->price);
            prop = buffer;
        }
        // Get the current player
        else if (item == "current_player") {            
            sprintf(buffer, "%d", this->player);
            prop = buffer;
        }
        // Get the party's gold amount
        else if (item == "totalgold") {            
            sprintf(buffer, "%d", c->saveGame->gold);
            prop = buffer;
        }
        // Get the number of party members in the party
        else if (item == "party_members") {            
            sprintf(buffer, "%d", c->saveGame->members);
            prop = buffer;
        }
        // Get the current transport
        else if (item == "transport") {
            if (c->transportContext & TRANSPORT_FOOT)
                prop = "foot";
            if (c->transportContext & TRANSPORT_HORSE)
                prop = "horse";
            if (c->transportContext & TRANSPORT_SHIP)
                prop = "ship";
            if (c->transportContext & TRANSPORT_BALLOON)
                prop = "balloon";
        }
        else if ((pos = item.find("quantity:")) < item.length()) {
            char buffer[16] = {0};
            pos = item.find_first_of(":");

            string subItem = item.substr(pos+1);
            pos = subItem.find_first_of(":");
            
            /* multi-parameter quantity */
            if ((pos > 0) && (pos < subItem.length())) {
                string lastItem = subItem.substr(pos+1);
                subItem = subItem.substr(0, pos);                

                if (subItem == "weapons") {
                    const Weapon *weapon = Weapon::get(lastItem);
                    if (weapon != NULL)
                        sprintf(buffer, "%d", c->saveGame->weapons[weapon->getType()]);
                }
                else if (subItem == "armor") {
                    const Armor *armor = Armor::get(lastItem);
                    if (armor != NULL)
                        sprintf(buffer, "%d", c->saveGame->armor[armor->getType()]);
                }
            }

            /* single-parameter quantity */
            else {
                if (subItem == "keys")
                    sprintf(buffer, "%d", c->saveGame->keys);
                else if (subItem == "torches")
                    sprintf(buffer, "%d", c->saveGame->torches);
                else if (subItem == "gems")
                    sprintf(buffer, "%d", c->saveGame->gems);
                else if (subItem == "sextants")
                    sprintf(buffer, "%d", c->saveGame->sextants);
                else if (subItem == "food")
                    sprintf(buffer, "%d", (c->saveGame->food / 100));
                else if (subItem == "gold")
                    sprintf(buffer, "%d", c->saveGame->gold);
                else if (subItem == "party_members")
                    sprintf(buffer, "%d", c->saveGame->members);
                else if (subItem == "moves")
                    sprintf(buffer, "%d", c->saveGame->moves);                
            }

            if (strlen(buffer) > 0)
                prop = buffer;
        }
        else if ((pos = item.find("show_inventory:")) < item.length()) {
            pos = item.find(":");
            string itemScript = item.substr(pos+1);

            xmlNodePtr itemShowScript = find(node, itemScript);

            xmlNodePtr item;
            prop.erase();
            
            /**
             * Save translation context and iterator
             */ 
            int oldIterator = this->iterator;
            xmlNodePtr oldContext = this->translationContext;

            /* start iterator at 0 */
            this->iterator = 0;
            
            for (item = node->children; item; item = item->next) {
                if (xmlStrcmp(item->name, (const xmlChar *)"item") == 0) {
                    bool hidden = (bool)xmlGetPropAsBool(item, "hidden");                    

                    if (!hidden) {
                        /* make sure the item's requisites are met */
                        if (!xmlPropExists(item, "req") || compare(getPropAsStr(item, "req"))) {
                            /* put a newline after each */
                            if (this->iterator > 0)
                                prop += "\n";                            

                            /* set translation context to item */
                            this->translationContext = item;
                            execute(itemShowScript, NULL, &prop);

                            this->iterator++;
                        }
                    }                    
                }
            }
            
            /**
             * Restore translation context and iterator to previous values
             */ 
            this->translationContext = oldContext;
            this->iterator = oldIterator;
        }

        /**
         * Make a string containing the available choices using the
         * vendor's inventory (i.e. "bcde")
         */ 
        else if (item == "inventory_choices") {
            xmlNodePtr item;
            string choices;

            for (item = node->children; item; item = item->next) {
                if (xmlStrcmp(item->name, (const xmlChar *)"item") == 0) {
                    string choice = getPropAsStr(item, "choice");
                    /* make sure the item's requisites are met */
                    if (!xmlPropExists(item, "req") || (compare(getPropAsStr(item, "req"))))
                        choices += choice[0];
                }
            }

            prop = choices;
        }

        /**
         * Retrieve game settings
         */ 
        else if (item.find("settings:") == 0) {
            string subitem;
            pos = item.find_first_of(":");
            char buffer[16] = {0};

            subitem = item.substr(pos+1);
            item = item.substr(0, pos);

            if (subitem == "enhancements")
                prop = settings.enhancements ? "true" : "false";
            else if (subitem == "game_cycles_per_second") {
                sprintf(buffer, "%d", settings.gameCyclesPerSecond);
                prop = buffer;
            }
            else if (subitem == "music")
                prop = settings.musicVol ? "true" : "false";
            else if (subitem == "sound")
                prop = settings.soundVol ? "true" : "false";            
        }

        /**
         * Retrieve party data
         */ 
        else if ((pos = item.find("party:")) < item.length()) {
            string item2, item3, item4;

            pos = item.find_first_of(":");

            /* get 2nd parameter */
            item2 = item.substr(pos+1);
            pos = item2.find_first_of(":");

            /* get 3rd and 4th parameters, if they exist */
            if ((pos > 0) && (pos < item2.length())) {
                item3 = item2.substr(pos+1);
                item2 = item2.substr(0, pos);
                pos = item3.find_first_of(":");

                if ((pos > 0) && (pos < item3.length())) {
                    item4 = item3.substr(pos+1);
                    item3 = item3.substr(0, pos);
                }
            }

            /* party:member# */
            if (item2.find_first_of("member") == 0) {                
                int member = (int)strtol(item2.substr(6,1).c_str(), NULL, 10) - 1;
                PartyMember *p = c->party->member(member);
                
                /* party:member#:needs */
                if (item3 == "needs") {                    
                    if (item4 == "cure") /* party:member:needs:cure */
                        prop = (p->getStatus() == STAT_POISONED) ? "true" : "false";
                    else if ((item4 == "heal") || (item4 == "fullheal")) /* party:member:needs:(heal||fullheal) */
                        prop = (p->getHp() < p->getMaxHp()) ? "true" : "false";
                    else if (item4 == "resurrect") /* party:member:needs:resurrect */
                        prop = (p->getStatus() == STAT_DEAD) ? "true" : "false";
                }
                /* party:member#:hp */
                else if (item3 == "hp") {
                    char buffer[16];
                    sprintf(buffer, "%d", p->getHp());
                    prop = buffer;
                }
            }
        }        
        
        /**
         * Check for functions
         */ 
        else {            
            string funcName, content;

            funcParse(item, &funcName, &content);
            
            if (funcName.empty()) {
                /* we have the property name, now go get the property value! */
                prop = getPropAsStr(node, item, true);
            }
            else {
                /* we have a function, make it work! */

                char buffer[16];

                /* perform the <math> function on the content */
                if (funcName == "math") {
                    if (content.empty())
                        errorWarning("Error: empty math() function");

                    sprintf(buffer, "%d", mathValue(content));                    

                    prop = buffer;
                }

                /**
                 * Does a true/false comparison on the content.
                 * Replaced with "true" if evaluates to true, or "false" if otherwise
                 */
                else if (funcName == "compare") {
                    if (compare(content))
                        prop = "true";
                    else prop = "false";
                }

                /* make the string upper case */
                else if (funcName == "toupper") {
                    string::iterator current;
                    for (current = content.begin(); current != content.end(); current++)
                        *current = toupper(*current);
                
                    prop = content;
                }
                /* make the string lower case */
                else if (funcName == "tolower") {
                    string::iterator current;
                    for (current = content.begin(); current != content.end(); current++)
                        *current = tolower(*current);
                    
                    prop = content;
                }
                /* generate a random number */
                else if (funcName == "random") {                    
                    sprintf(buffer, "%d", xu4_random((int)strtol(content.c_str(), NULL, 10)));
                    prop = buffer;
                }
                /* replaced with "true" if content is empty, or "false" if not */
                else if (funcName == "isempty") {                    
                    if (content.empty())
                        prop = "true";
                    else prop = "false";
                }                
            }
        }        
       
        if (prop.empty() && debug)
            fprintf(debug, "\nWarning: dynamic property '{%s}' not found in vendor script (was this intentional?)", item.c_str());        

        if (debug)
            fprintf(debug, "\"%s\"", prop.c_str());

        /* put the script back together */
        *text = pre + prop + post;
    }

    /* remove all unnecessary spaces from xml */    
    while ((pos = text->find("\t")) < text->length())
        text->replace(pos, 1, "");
    while ((pos = text->find("  ")) < text->length())
        text->replace(pos, 2, "");
    while ((pos = text->find("\n ")) < text->length())
        text->replace(pos, 2, "\n");    
}

/**
 * Finds a subscript of script 'node'
 */ 
 xmlNodePtr Script::find(xmlNodePtr node, string script_to_find, string choice, bool _default) {
    xmlNodePtr current;
    if (node) {
        for (current = node->children; current; current = current->next) {
            if (!xmlNodeIsText(current) && (script_to_find == (char *)current->name)) {
                if (choice.empty() && !xmlPropExists(current, "choice") && !_default)
                    return current;
                else if (xmlPropExists(current, "choice") && (choice == xmlGetPropAsStr(current, "choice")))
                    return current;
                else if (_default && xmlPropExists(current, "default") && xmlGetPropAsBool(current, "default"))
                    return current;
            }
        }

        /* only search the parent nodes if we haven't hit the base <script> node */
        if (xmlStrcmp(node->name, (const xmlChar *)"script") != 0)
            current = find(node->parent, script_to_find, choice);

        /* find the default script instead */
        if (!current && !choice.empty() && !_default)
            current = find(node, script_to_find, "", true);
        return current;        
    }
    return NULL;
}

/**
 * Gets a property as string from the script, and
 * translates it using scriptTranslate.
 */ 
string Script::getPropAsStr(xmlNodePtr node, string prop, bool recursive) {
    string propvalue;
    if (xmlPropExists(node, prop.c_str()))
        propvalue = xmlGetPropAsStr(node, prop.c_str());
    if (propvalue.empty() && node->parent && recursive)
        propvalue = getPropAsStr(node->parent, prop, recursive);

    translate(&propvalue);
    return propvalue;    
}

/**
 * Gets a property as int from the script
 */ 
int Script::getPropAsInt(xmlNodePtr node, string prop, bool recursive) {
    string propvalue = getPropAsStr(node, prop, recursive);
    return mathValue(propvalue);
}

/**
 * Gets the content of a script node
 */ 
string Script::getContent(xmlNodePtr node) {
    string content = (char *)xmlNodeGetContent(node);
    translate(&content);
    return content;
}

/**
 * Get a 1-key keyboard input as a "choice" (usually y/n)
 */ 
 ScriptReturnCode Script::choice(xmlNodePtr script, xmlNodePtr current) {
    this->currentScript = script;
    this->currentItem = current;
    this->choices = getPropAsStr(current, "options");
    if (xmlPropExists(current, "target"))
        this->target = getPropAsStr(current, "target");
    else this->target.erase();

    if (debug)
        fprintf(debug, "\nChoice: %s - target: %s", this->choices.c_str(), this->target.c_str());

    this->choices += "\015 \033";

    this->state = SCRIPT_STATE_CHOICE;
    return SCRIPT_RET_STOP;
}

/**
 * End script execution
 */ 
ScriptReturnCode Script::end(xmlNodePtr script, xmlNodePtr current) {
    /**
     * See if there's a global <end> node declared for cleanup
     */
    xmlNodePtr endScript = find(scriptNode, "end");
    if (endScript)
        execute(endScript);

    if (debug)
        fprintf(debug, "\n<End script>");
    
    this->state = SCRIPT_STATE_DONE;
    
    return SCRIPT_RET_STOP;
}

/**
 * Wait for keypress from the user
 */ 
ScriptReturnCode Script::waitForKeypress(xmlNodePtr script, xmlNodePtr current) {
    this->currentScript = script;
    this->currentItem = current;
    this->choices = "abcdefghijklmnopqrstuvwxyz01234567890\015 \033";
    this->target.erase();
    this->state = SCRIPT_STATE_WAIT_FOR_KEYPRESS;

    if (debug)
        fprintf(debug, "\n<Wait>");

    return SCRIPT_RET_STOP;
}

/**
 * Redirects script execution to another script
 */ 
ScriptReturnCode Script::redirect(xmlNodePtr script, xmlNodePtr current) {
    string target;
    
    if (xmlPropExists(current, "redirect"))
        target = getPropAsStr(current, "redirect");
    else target = getPropAsStr(current, "target");

    /* set a new current_choice */
    string choice = getPropAsStr(current, "choice");
    if (!choice.empty())
        this->answer = choice;    

    xmlNodePtr newScript = find(this->scriptNode, target, choice);
    if (!newScript)
        errorFatal("Error: redirect failed -- could not find target script '%s' with choice=\"%s\"", target.c_str(), choice.c_str());

    if (debug) {
        fprintf(debug, "\nRedirected to '%s", target.c_str());
        if (choice.length())
            fprintf(debug, ":%s", choice.c_str());
        fprintf(debug, "'");
    }
    
    execute(newScript);
    return SCRIPT_RET_REDIRECTED;
}

/**
 * Includes a script to be executed
 */ 
ScriptReturnCode Script::include(xmlNodePtr script, xmlNodePtr current) {
    string scriptName = getPropAsStr(current, "script");
    string choice = getPropAsStr(current, "choice");    

    xmlNodePtr newScript = find(this->scriptNode, scriptName, choice);
    if (!newScript)
        errorFatal("Error: include failed -- could not find target script '%s' with choice=\"%s\"", scriptName.c_str(), choice.c_str());

    if (debug) {
        fprintf(debug, "\nIncluded script <%s", scriptName.c_str());
        if (choice.length())
            fprintf(debug, " choice=\"%s\"", choice.c_str());
        fprintf(debug, " .../>");
    }

    execute(newScript);
    return SCRIPT_RET_OK;
}

/**
 * Waits a given number of milliseconds before continuing execution
 */ 
ScriptReturnCode Script::wait(xmlNodePtr script, xmlNodePtr current) {
    int msecs = getPropAsInt(current, "msecs");
    eventHandlerSleep(msecs);
    return SCRIPT_RET_OK;
}

/**
 * Executes a 'for' loop script
 */ 
ScriptReturnCode Script::forLoop(xmlNodePtr script, xmlNodePtr current) {
    ScriptReturnCode retval = SCRIPT_RET_OK;
    int start = getPropAsInt(current, "start"),
        end = getPropAsInt(current, "end"),
        /* save the iterator in case this loop is nested */
        oldIterator = this->iterator,
        i;

    if (debug)
        fprintf(debug, "\n\n<For Start=%d End=%d>\n", start, end);
    
    for (i = start, this->iterator = start;
         i <= end;
         i++, this->iterator++) {
        
        if (debug)
            fprintf(debug, "\n%d: ", i);

        retval = execute(current);
        if ((retval == SCRIPT_RET_REDIRECTED) || (retval == SCRIPT_RET_STOP))
            break;
    }

    /* restore the previous iterator */
    this->iterator = oldIterator;

    return retval;
}

/**
 * Randomely executes script code
 */ 
ScriptReturnCode Script::random(xmlNodePtr script, xmlNodePtr current) {
    int perc = getPropAsInt(current, "chance");
    int num = xu4_random(100);
    ScriptReturnCode retval = SCRIPT_RET_OK;

    if (num < perc)
        retval = execute(current);

    if (debug)
        fprintf(debug, "\nRandom (%d%%): rolled %d (%s)", perc, num, (num < perc) ? "Succeeded" : "Failed");

    return retval;
}

/**
 * Moves the player's current position
 */ 
ScriptReturnCode Script::move(xmlNodePtr script, xmlNodePtr current) {
    if (xmlPropExists(current, "x"))
        c->location->coords.x = getPropAsInt(current, "x");
    if (xmlPropExists(current, "y"))
        c->location->coords.y = getPropAsInt(current, "y");
    if (xmlPropExists(current, "z"))
        c->location->coords.z = getPropAsInt(current, "z");

    if (debug)
        fprintf(debug, "\nMove: x-%d y-%d z-%d", c->location->coords.x, c->location->coords.y, c->location->coords.z);
    
    gameUpdateScreen();
    return SCRIPT_RET_OK;
}

/**
 * Puts the player to sleep. Useful when coding inn scripts
 */ 
ScriptReturnCode Script::sleep(xmlNodePtr script, xmlNodePtr current) {
    if (debug)
        fprintf(debug, "\nSleep!\n");
    innBegin();

    return SCRIPT_RET_OK;
}

/**
 * Enables/Disables the keyboard cursor
 */ 
ScriptReturnCode Script::cursor(xmlNodePtr script, xmlNodePtr current) {
    bool enable = (bool)xmlGetPropAsBool(current, "enable");
    if (enable)
        screenEnableCursor();
    else screenDisableCursor();

    return SCRIPT_RET_OK;
}

/**
 * Pay gold to someone
 */ 
ScriptReturnCode Script::pay(xmlNodePtr script, xmlNodePtr current) {
    /* now the translation context should be our item */
    xmlNodePtr item = this->translationContext;

    int price = xmlPropExists(current, "price") ?
        getPropAsInt(current, "price") :
        getPropAsInt(item, "price");
    int quant = xmlPropExists(current, "quant") ?
        getPropAsInt(current, "quant") :
        this->quant;

    string cantpay = getPropAsStr(current, "cantpay");

    if (price < 0)
        errorFatal("Error: could not find price for item");

    if (debug) {
        fprintf(debug, "\nPay: price(%d) quantity(%d)", price, quant);                
        fprintf(debug, "\n\tParty gold:  %d -", c->saveGame->gold);
        fprintf(debug, "\n\tTotal price: %d", price * quant);
    }
    
    price *= quant;
    if (price > c->saveGame->gold) {
        if (debug)
            fprintf(debug, "\n\t=== Can't pay! ===");
        run(cantpay);
        return SCRIPT_RET_STOP;
    }
    else c->party->adjustGold(-price);

    if (debug)
        fprintf(debug, "\n\tBalance:     %d\n", c->saveGame->gold);

    return SCRIPT_RET_OK;
}

/**
 * Perform a limited 'if' statement
 */ 
ScriptReturnCode Script::_if(xmlNodePtr script, xmlNodePtr current) {
    string test = getPropAsStr(current, "test");
    ScriptReturnCode retval = SCRIPT_RET_OK;

    if (debug)
        fprintf(debug, "\nIf(%s) - ", test.c_str());

    if (compare(test)) {
        if (debug)
            fprintf(debug, "True - Executing '%s'", current->name);

        retval = execute(current);                
    }
    else if (debug)
        fprintf(debug, "False");

    return retval;
}

/**
 * Get input from the player
 */ 
ScriptReturnCode Script::input(xmlNodePtr script, xmlNodePtr current) {
    string type = getPropAsStr(current, "type");
            
    this->currentScript = script;
    this->currentItem = current;

    if (xmlPropExists(current, "target"))
        this->target = getPropAsStr(current, "target");
    else this->target.erase();

    if (type == "quantity")                
        this->state = SCRIPT_STATE_INPUT_QUANTITY;
    else if (type == "price")
        this->state = SCRIPT_STATE_INPUT_PRICE;
    else if (type == "text")
        this->state = SCRIPT_STATE_INPUT_TEXT;
    else if (type == "player") {
        int i;
        char buffer[16];
        this->state = SCRIPT_STATE_INPUT_PLAYER;

        /* add member numbers to our choices */
        this->choices = "0";
        for (i = 0; i < c->saveGame->members; i++) {
            sprintf(buffer, "%d", i+1);
            this->choices += buffer;
        }
    }            

    if (debug)
        fprintf(debug, "\nInput: %s", type.c_str());

    /* the script stops here, at least for now */
    return SCRIPT_RET_STOP;    
}

/**
 * Add item to inventory
 */ 
ScriptReturnCode Script::add(xmlNodePtr script, xmlNodePtr current) {
    string type = getPropAsStr(current, "type");
    string subtype = getPropAsStr(current, "subtype");
    int quant = getPropAsInt(this->translationContext, "quant");
    if (quant == 0)
        quant = this->quant;
    else
        quant *= this->quant;

    if (debug) {
        fprintf(debug, "\nAdd: %s ", type.c_str());
        if (subtype.length())
            fprintf(debug, "- %s ", subtype.c_str());
    }

    if (type == "gold") {
        quant *= this->price;
        c->party->adjustGold(quant);
    }
    else if (type == "food") {
        quant *= 100;
        c->party->adjustFood(quant);
    }
    else if (type == "horse")
        gameSetTransport(tileGetHorseBase());
    else if (type == "torch")
        AdjustValueMax(c->saveGame->torches, quant, 99);
    else if (type == "gem")
        AdjustValueMax(c->saveGame->gems, quant, 99);
    else if (type == "key")
        AdjustValueMax(c->saveGame->keys, quant, 99);
    else if (type == "sextant")
        AdjustValueMax(c->saveGame->sextants, quant, 99);
    else if (type == "weapon")                
        AdjustValueMax(c->saveGame->weapons[subtype[0] - 'a'], quant, 99);
    else if (type == "armor")
        AdjustValueMax(c->saveGame->armor[subtype[0] - 'a'], quant, 99);
    else if (type == "reagent") {
        int reagent;
        static const string reagents[] = {
            "ash", "ginseng", "garlic", "silk", "moss", "pearl", "mandrake", "nightshade", ""
        };

        for (reagent = 0; reagents[reagent].length(); reagent++) {
            if (reagents[reagent] == subtype)
                break;
        }

        if (reagents[reagent].length()) {
            AdjustValueMax(c->saveGame->reagents[reagent], quant, 99);
            gameResetSpellMixing();
        }
        else errorWarning("Error: reagent '%s' not found", subtype.c_str());
    }

    if (debug)
        fprintf(debug, "(x%d)", quant);

    return SCRIPT_RET_OK;
}

/**
 * Lose item
 */ 
ScriptReturnCode Script::lose(xmlNodePtr script, xmlNodePtr current) {
    string type = getPropAsStr(current, "type");
    string subtype = getPropAsStr(current, "subtype");
    int quant = this->quant;            

    if (type == "weapon")
        AdjustValueMin(c->saveGame->weapons[subtype[0] - 'a'], -quant, 0);            
    else if (type == "armor")
        AdjustValueMin(c->saveGame->armor[subtype[0] - 'a'], -quant, 0);            

    if (debug) {
        fprintf(debug, "\nLose: %s ", type.c_str());
        if (subtype.length())
            fprintf(debug, "- %s ", subtype.c_str());
        fprintf(debug, "(x%d)", quant);
    }

    return SCRIPT_RET_OK;
}

/**
 * Heals a party member
 */ 
ScriptReturnCode Script::heal(xmlNodePtr script, xmlNodePtr current) {
    string type = getPropAsStr(current, "type");
    PartyMember *p = c->party->member(getPropAsInt(current, "player")-1);

    if (type == "cure")
        p->heal(HT_CURE);
    else if (type == "heal")
        p->heal(HT_HEAL);
    else if (type == "fullheal")
        p->heal(HT_FULLHEAL);
    else if (type == "resurrect")
        p->heal(HT_RESURRECT);

    return SCRIPT_RET_OK;
}

/**
 * Performs all of the effects of casting a spell
 */ 
ScriptReturnCode Script::castSpell(xmlNodePtr script, xmlNodePtr current) {
    extern SpellEffectCallback spellEffectCallback;
    (*spellEffectCallback)('r', -1, SOUND_MAGIC);
    if (debug)
        fprintf(debug, "\n<Spell effect>");

    return SCRIPT_RET_OK;
}

/**
 * Apply damage to a player
 */ 
ScriptReturnCode Script::damage(xmlNodePtr script, xmlNodePtr current) {
    int player = getPropAsInt(current, "player") - 1;
    int pts = getPropAsInt(current, "pts");
    PartyMember *p;

    if (player == -1)
        player = this->player - 1;

    p = c->party->member(player);
    p->applyDamage(pts);

    if (debug)
        fprintf(debug, "\nDamage: %d damage to player %d", pts, player);

    return SCRIPT_RET_OK;
}

/**
 * Apply karma changes based on the action taken
 */ 
ScriptReturnCode Script::karma(xmlNodePtr script, xmlNodePtr current) {
    string action = getPropAsStr(current, "action");            

    if (debug)
        fprintf(debug, "\nKarma: adjusting - '%s'", action.c_str());            

    typedef std::map<string, KarmaAction, std::less<string> > KarmaActionMap;
    static KarmaActionMap action_map;

    if (action_map.size() == 0) {
        action_map["found_item"]            = KA_FOUND_ITEM;
        action_map["stole_chest"]           = KA_STOLE_CHEST;
        action_map["gave_to_beggar"]        = KA_GAVE_TO_BEGGAR;
        action_map["bragged"]               = KA_BRAGGED;
        action_map["humble"]                = KA_HUMBLE;
        action_map["hawkwind"]              = KA_HAWKWIND;
        action_map["meditation"]            = KA_MEDITATION;
        action_map["bad_mantra"]            = KA_BAD_MANTRA;
        action_map["attacked_good"]         = KA_ATTACKED_GOOD;
        action_map["fled_evil"]             = KA_FLED_EVIL;
        action_map["healthy_fled_evil"]     = KA_HEALTHY_FLED_EVIL;
        action_map["killed_evil"]           = KA_KILLED_EVIL;
        action_map["spared_good"]           = KA_SPARED_GOOD;
        action_map["gave_blood"]            = KA_DONATED_BLOOD;
        action_map["didnt_give_blood"]      = KA_DIDNT_DONATE_BLOOD;
        action_map["cheated_merchant"]      = KA_CHEAT_REAGENTS;
        action_map["honest_to_merchant"]    = KA_DIDNT_CHEAT_REAGENTS;
        action_map["used_skull"]            = KA_USED_SKULL;
        action_map["destroyed_skull"]       = KA_DESTROYED_SKULL;
    }

    KarmaActionMap::iterator ka = action_map.find(action);
    if (ka != action_map.end())
        c->party->adjustKarma(ka->second);
    else if (debug)
        fprintf(debug, " <FAILED - action '%s' not found>", action.c_str());

    return SCRIPT_RET_OK;

}

/**
 * Set the currently playing music
 */ 
ScriptReturnCode Script::music(xmlNodePtr script, xmlNodePtr current) {
    if (xmlGetPropAsBool(current, "reset"))
        musicPlay();
    else {
        string type = getPropAsStr(current, "type");

        if (xmlGetPropAsBool(current, "play"))
            musicPlay();
        if (xmlGetPropAsBool(current, "stop"))
            musicStop();
        else if (type == "shopping")
            musicShopping();
        else if (type == "camp")
            musicCamp();
    }

    return SCRIPT_RET_OK;
}

/**
 * Save currently selected choice, useful for when somebody selects
 * something that you want to remember later in the script
 */ 
ScriptReturnCode Script::saveChoice(xmlNodePtr script, xmlNodePtr current) {
    this->saved_choice = this->answer;
    if (debug)
        fprintf(debug, "\nSave Choice: '%s'", this->saved_choice.c_str());

    return SCRIPT_RET_OK;
}

/**
 * Set a new choice, useful for "remembering" what somebody has
 * selected earlier or in another script.
 */ 
ScriptReturnCode Script::setChoice(xmlNodePtr script, xmlNodePtr current) {
    string value = getPropAsStr(current, "value");
    this->answer = value;

    if (debug)
        fprintf(debug, "\nSet Choice: '%s'", this->answer.c_str());

    return SCRIPT_RET_OK;
}

/**
 * 
 */ 
ScriptReturnCode Script::setQuantity(xmlNodePtr script, xmlNodePtr current) {
    this->quant = getPropAsInt(current, "value");

    if (debug)
        fprintf(debug, "\nSet Quantity: '%d'", this->quant);

    return SCRIPT_RET_OK;
}

/**
 * 
 */ 
ScriptReturnCode Script::setPrice(xmlNodePtr script, xmlNodePtr current) {
    this->price = getPropAsInt(current, "value");

    if (debug)
        fprintf(debug, "\nSet Price: '%d'", this->price);

    return SCRIPT_RET_OK;
}

/**
 * 
 */ 
ScriptReturnCode Script::setPlayer(xmlNodePtr script, xmlNodePtr current) {
    this->player = getPropAsInt(current, "value");

    if (debug)
        fprintf(debug, "\nSet Player: '%d'", this->player);
    
    return SCRIPT_RET_OK;
}

/**
 * Display a different ztats screen
 */ 
ScriptReturnCode Script::ztats(xmlNodePtr script, xmlNodePtr current) {
    typedef std::map<string, StatsView, std::less<string> > StatsViewMap;
    static StatsViewMap view_map;

    if (view_map.size() == 0) {        
        view_map["party"]       = STATS_PARTY_OVERVIEW;
        view_map["party1"]      = STATS_CHAR1;
        view_map["party2"]      = STATS_CHAR2;
        view_map["party3"]      = STATS_CHAR3;
        view_map["party4"]      = STATS_CHAR4;
        view_map["party5"]      = STATS_CHAR5;
        view_map["party6"]      = STATS_CHAR6;
        view_map["party7"]      = STATS_CHAR7;
        view_map["party8"]      = STATS_CHAR8;
        view_map["weapons"]     = STATS_WEAPONS;
        view_map["armor"]       = STATS_ARMOR;
        view_map["equipment"]   = STATS_EQUIPMENT;
        view_map["item"]        = STATS_ITEMS;
        view_map["reagents"]    = STATS_REAGENTS;
        view_map["mixtures"]    = STATS_MIXTURES;
    }

    if (xmlPropExists(current, "screen")) {
        string screen = getPropAsStr(current, "screen");
        StatsViewMap::iterator view;

        if (debug)
            fprintf(debug, "\nZtats: %s", screen.c_str());
    
        /**
         * Find the correct stats view
         */ 
        view = view_map.find(screen);
        if (view != view_map.end()) 
            c->stats->view = view->second; /* change it! */
        else if (debug)
            fprintf(debug, " <FAILED - view could not be found>");
        c->stats->update();
    }
    else c->stats->showPartyView();

    return SCRIPT_RET_OK;
}

/**
 * Parses a math string's children into results so
 * there is only 1 equation remaining.
 * 
 * ie. <math>5*<math>6/3</math></math>
 */
void Script::mathParseChildren(xmlNodePtr math, string *result) {
    xmlNodePtr current;
    result->erase();

    for (current = math->children; current; current = current->next) {
        if (xmlNodeIsText(current)) {
            *result = getContent(current);        
        }
        else if (xmlStrcmp(current->name, (const xmlChar *)"math") == 0) {
            string children_results;
            char buffer[16];
            
            mathParseChildren(current, &children_results);
            sprintf(buffer, "%d", mathValue(children_results));
            *result += buffer;
        }
    }    
}

/**
 * Parses a string into left integer value, right integer value,
 * and operator. Returns false if the string is not a valid
 * math equation
 */ 
bool Script::mathParse(string str, int *lval, int *rval, string *op) {
    string left, right;
    parseOperation(str, &left, &right, op);

    if (op->empty())
        return false;

    if (left.length() == 0 || right.length() == 0)
        return false;

    /* make sure that we're dealing with numbers */
    if (!isdigit(left[0]) || !isdigit(right[0]))
        return false;    

    *lval = (int)strtol(left.c_str(), NULL, 10);
    *rval = (int)strtol(right.c_str(), NULL, 10);    
    return true;
}

/**
 * Parses a string containing an operator (+, -, *, /, etc.) into 3 parts,
 * left, right, and operator.
 */
void Script::parseOperation(string str, string *left, string *right, string *op) {
    /* list the longest operators first, so they're detected correctly */
    static const string ops[] = {"==", ">=", "<=", "+", "-", "*", "/", "%", "=", ">", "<", ""};
    int pos = 0,
        i = 0;

    pos = str.find(ops[i]);
    while ((pos <= 0) && !ops[i].empty()) {    
        i++;
        pos = str.find(ops[i]);
    }

    if (ops[i].empty()) {
        op->erase();
        return;
    }
    else *op = ops[i];

    *left = str.substr(0, pos),
    *right = str.substr(pos+ops[i].length());
}

/**
 * Takes a simple equation string and returns the value
 */ 
int Script::mathValue(string str) {
    int lval, rval;
    string op;
    
    /* something was invalid, just return the integer value */
    if (!mathParse(str, &lval, &rval, &op))    
        return (int)strtol(str.c_str(), NULL, 10);
    else return math(lval, rval, op);
}

/**
 * Performs simple math operations in the script
 */ 
int Script::math(int lval, int rval, string &op) {    
    if (op == "+")
        return lval + rval;
    else if (op == "-")
        return lval - rval; 
    else if (op == "*")
        return lval * rval; 
    else if (op == "/")
        return lval / rval; 
    else if (op == "%")
        return lval % rval;
    else if ((op == "=") || (op == "=="))
        return lval == rval;
    else if (op == ">")
        return lval > rval;
    else if (op == "<")
        return lval < rval;
    else if (op == ">=")
        return lval >= rval;
    else if (op == "<=")
        return lval <= rval;
    else
        errorFatal("Error: invalid 'math' operation attempted in vendorScript.xml");

    return 0;
}

/**
 * Does a boolean comparison on a string (math or string),
 * fails if the string doesn't contain a valid comparison
 */ 
bool Script::compare(string str) {
    int lval, rval;
    string left, right, op;
    int and_pos, or_pos;
    bool invert = false,
         _and = false;         

    /**
     * Handle parsing of complex comparisons
     * For example:
     *
     * true&&true&&true||false
     *
     * Since this resolves right-to-left, this would evaluate
     * similarly to (true && (true && (true || false))), returning
     * true.     
     */
    and_pos = str.find_first_of("&&");
    or_pos = str.find_first_of("||");

    if ((and_pos > 0) || (or_pos > 0)) {
        bool retfirst, retsecond;
        int pos;
        
        if ((or_pos < 0) || ((and_pos > 0) && (and_pos < or_pos)))
            _and = true;        
        
        if (_and)
            pos = and_pos;
        else pos = or_pos;

        retsecond = compare(str.substr(pos+2));
        str = str.substr(0, pos);
        retfirst = compare(str);

        if (_and)
            return (retfirst && retsecond);
        else return (retfirst || retsecond);
    }

    if (str[0] == '!') {
        str = str.substr(1);
        invert = true;
    }

    if (str == "true")
        return !invert;
    else if (str == "false")
        return invert;
    else if (mathParse(str, &lval, &rval, &op))
        return (bool)math(lval, rval, op) ? !invert : invert;
    else {
        parseOperation(str, &left, &right, &op);
        /* can only really do equality comparison */
        if ((op[0] == '=') && (left == right))
            return !invert;
    }
    return invert;
}

/**
 * Parses a function into its name and contents
 */ 
void Script::funcParse(string str, string *funcName, string *contents) {
    unsigned int pos;
    *funcName = str;

    pos = funcName->find_first_of("(");
    if (pos < funcName->length()) {
        *funcName = funcName->substr(0, pos);
        
        *contents = str.substr(pos+1);
        pos = contents->find_first_of(")");
        if ((pos < 0) || (pos >= contents->length()))
            errorWarning("Error: No closing ) in function %s()", funcName->c_str());
        else *contents = contents->substr(0, pos);
    }
    else funcName->erase();
}
