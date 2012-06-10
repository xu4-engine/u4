/**
 * $Id$
 */
    
#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <cctype>
#include <map>
#include <string>
#include "script.h"

#include "armor.h"
#include "camp.h"
#include "context.h"
#include "conversation.h"
#include "debug.h"
#include "error.h"
#include "event.h"
#include "filesystem.h"
#include "game.h"
#include "music.h"
#include "player.h"
#include "savegame.h"
#include "screen.h"
#include "settings.h"
#include "spell.h"
#include "stats.h"
#include "tileset.h"
#include "u4file.h"
#include "utils.h"
#include "weapon.h"
#include "xml.h"

using namespace std;

/*
 * Script::Variable class
 */
Script::Variable::Variable() : i_val(0), s_val(""), set(false) {}
Script::Variable::Variable(const string &v) : set(true) {
    i_val = static_cast<int>(strtol(v.c_str(), NULL, 10));
    s_val = v;
}

Script::Variable::Variable(const int &v) : set(true) {
    i_val = v;
    s_val = to_string(v);
}

int&    Script::Variable::getInt()      { return i_val; }
string& Script::Variable::getString()   { return s_val; }

void    Script::Variable::setValue(const int &v)    { i_val = v; }
void    Script::Variable::setValue(const string &v) { s_val = v; }
void    Script::Variable::unset()                   {
    set = false;
    i_val = 0;
    s_val = "";
}

bool    Script::Variable::isInt() const             { return i_val > 0; }
bool    Script::Variable::isString() const          { return i_val == 0; }
bool    Script::Variable::isSet() const             { return set; }

/*
 * Static member variables 
 */ 
Script::ActionMap Script::action_map;

/**
 * Constructs a script object
 */ 
Script::Script() : vendorScriptDoc(NULL), scriptNode(NULL), debug(NULL), state(STATE_UNLOADED),
    nounName("item"), idPropName("id")
{       
    action_map["context"]           = ACTION_SET_CONTEXT;
    action_map["unset_context"]     = ACTION_UNSET_CONTEXT;
    action_map["end"]               = ACTION_END;
    action_map["redirect"]          = ACTION_REDIRECT;
    action_map["wait_for_keypress"] = ACTION_WAIT_FOR_KEY;
    action_map["wait"]              = ACTION_WAIT;
    action_map["stop"]              = ACTION_STOP;
    action_map["include"]           = ACTION_INCLUDE;
    action_map["for"]               = ACTION_FOR_LOOP;
    action_map["random"]            = ACTION_RANDOM;
    action_map["move"]              = ACTION_MOVE;
    action_map["sleep"]             = ACTION_SLEEP;
    action_map["cursor"]            = ACTION_CURSOR;
    action_map["pay"]               = ACTION_PAY;
    action_map["if"]                = ACTION_IF;
    action_map["input"]             = ACTION_INPUT;
    action_map["add"]               = ACTION_ADD;
    action_map["lose"]              = ACTION_LOSE;
    action_map["heal"]              = ACTION_HEAL;
    action_map["cast_spell"]        = ACTION_CAST_SPELL;
    action_map["damage"]            = ACTION_DAMAGE;
    action_map["karma"]             = ACTION_KARMA;
    action_map["music"]             = ACTION_MUSIC;
    action_map["var"]               = ACTION_SET_VARIABLE;    
    action_map["ztats"]             = ACTION_ZTATS;    
}

Script::~Script() {
    unload();
}

/**
 * Adds an information provider for the script
 */
void Script::addProvider(const string &name, Provider *p) {
    providers[name] = p;
}

/**
 * Loads the vendor script
 */ 
bool Script::load(string filename, string baseId, string subNodeName, string subNodeId) {
    xmlNodePtr root, node, child;
    this->state = STATE_NORMAL;

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
        static const char *dbg_filename = "debug/script.txt";
        // Our script is going to hog all the debug info
        if (xmlGetPropAsBool(root, "debug"))
            debug = FileSystem::openFile(dbg_filename, "wt");
        else {
            // See if we share our debug space with other scripts
            string val = xmlGetPropAsString(root, "debug");
            if (val == "share")
                debug = FileSystem::openFile(dbg_filename, "at");
        }
    }

    /**
     * Get a new global item name or id name
     */
    if (xmlPropExists(root, "noun"))
        nounName = xmlGetPropAsString(root, "noun");
    if (xmlPropExists(root, "id_prop"))
        idPropName = xmlGetPropAsString(root, "id_prop");

    this->currentScript = NULL;
    this->currentItem = NULL;

    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node) || (xmlStrcmp(node->name, (const xmlChar *) "script") != 0))
            continue;
        
        if (baseId == xmlGetPropAsString(node, "id")) {
            /**
             * We use the base node as our main script node
             */
            if (subNodeName.empty()) {
                this->scriptNode = node;            
                this->translationContext.push_back(node);

                break;                
            }

            for (child = node->xmlChildrenNode; child; child = child->next) {
                if (xmlNodeIsText(child) ||
                    xmlStrcmp(child->name, (const xmlChar *) subNodeName.c_str()) != 0)
                    continue;
         
                string id = xmlGetPropAsString(child, "id");

                if (id == subNodeId) {                    
                    this->scriptNode = child;                    
                    this->translationContext.push_back(child);

                    /**
                     * Get a new local item name or id name
                     */
                    if (xmlPropExists(node, "noun"))
                        nounName = xmlGetPropAsString(node, "noun");
                    if (xmlPropExists(node, "id_prop"))
                        idPropName = xmlGetPropAsString(node, "id_prop");

                    break;
                }                
            }    
            
            if (scriptNode) 
                break;
        }
    }

    if (scriptNode) {
        /**
         * Get a new local item name or id name
         */
        if (xmlPropExists(scriptNode, "noun"))
            nounName = xmlGetPropAsString(scriptNode, "noun");
        if (xmlPropExists(scriptNode, "id_prop"))
            idPropName = xmlGetPropAsString(scriptNode, "id_prop");

        if (debug)
            fprintf(debug, "\n<Loaded subscript '%s' where id='%s' for script '%s'>\n", subNodeName.c_str(), subNodeId.c_str(), baseId.c_str());
    }
    else {    
        if (subNodeName.empty())
            errorFatal("Couldn't find script '%s' in %s", baseId.c_str(), filename.c_str());
        else errorFatal("Couldn't find subscript '%s' where id='%s' in script '%s' in %s", subNodeName.c_str(), subNodeId.c_str(), baseId.c_str(), filename.c_str());
    }

    this->state = STATE_UNLOADED;

    return false;
}

/**
 * Unloads the script
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
    string search_id;
    
    if (variables.find(idPropName) != variables.end()) {
        if (variables[idPropName]->isSet())
            search_id = variables[idPropName]->getString();
        else search_id = "null";
    }
    
    scriptNode = find(this->scriptNode, script, search_id);

    if (!scriptNode)
        errorFatal("Script '%s' not found in vendorScript.xml", script.c_str());

    execute(scriptNode);
}

/**
 * Executes the subscript 'script' of the main script
 */ 
Script::ReturnCode Script::execute(xmlNodePtr script, xmlNodePtr currentItem, string *output) {
    xmlNodePtr current;    
    Script::ReturnCode retval = RET_OK;
    
    if (!script->children) {
        /* redirect the script to another node */
        if (xmlPropExists(script, "redirect"))
            retval = redirect(NULL, script);        
        /* end the conversation */
        else {
            if (debug)
                fprintf(debug, "\nA script with no children found (nowhere to go). Ending script...\n");
            screenMessage("\n");            
            this->state = STATE_DONE;
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
        retval = RET_OK;
        ActionMap::iterator action;

        /* nothing left to do */
        if (this->state == STATE_DONE)
            break;

        /* begin execution of script */       

        /**
         * Handle Text
         */
        if (xmlNodeIsText(current)) {
            string content = getContent(current);
            if (output)
                *output += content;
            else screenMessage("%s", content.c_str());

            if (debug && content.length())
                fprintf(debug, "\nOutput: \n====================\n%s\n====================", content.c_str());
        }
        /* skip comments */
        else if (current->type == XML_COMMENT_NODE) {}
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
                case ACTION_SET_CONTEXT:    retval = pushContext(script, current); break;
                case ACTION_UNSET_CONTEXT:  retval = popContext(script, current); break;
                case ACTION_END:            retval = end(script, current); break;                    
                case ACTION_REDIRECT:       retval = redirect(script, current); break;
                case ACTION_WAIT_FOR_KEY:   retval = waitForKeypress(script, current); break;
                case ACTION_WAIT:           retval = wait(script, current); break;                    
                case ACTION_STOP:           retval = RET_STOP; break;
                case ACTION_INCLUDE:        retval = include(script, current); break;                    
                case ACTION_FOR_LOOP:       retval = forLoop(script, current); break;
                case ACTION_RANDOM:         retval = random(script, current); break;
                case ACTION_MOVE:           retval = move(script, current); break;
                case ACTION_SLEEP:          retval = sleep(script, current); break;
                case ACTION_CURSOR:         retval = cursor(script, current); break;
                case ACTION_PAY:            retval = pay(script, current); break;                    
                case ACTION_IF:             retval = _if(script, current); break;
                case ACTION_INPUT:          retval = input(script, current); break;
                case ACTION_ADD:            retval = add(script, current); break;                    
                case ACTION_LOSE:           retval = lose(script, current); break;                    
                case ACTION_HEAL:           retval = heal(script, current); break;
                case ACTION_CAST_SPELL:     retval = castSpell(script, current); break;                    
                case ACTION_DAMAGE:         retval = damage(script, current); break;                    
                case ACTION_KARMA:          retval = karma(script, current); break;                    
                case ACTION_MUSIC:          retval = music(script, current); break;                    
                case ACTION_SET_VARIABLE:   retval = setVar(script, current); break;
                case ACTION_ZTATS:          retval = ztats(script, current); break;
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
            if ((retval == RET_REDIRECTED) || (retval== RET_STOP))
                break;
        }        
        
        if (debug)
            fprintf(debug, "\n");        
    }    

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
void Script::resetState()               { state = STATE_NORMAL; }
void Script::setState(Script::State s)  { state = s; }
void Script::setTarget(string val)      { target = val; }
void Script::setChoices(string val)     { choices = val; }
void Script::setVar(string name, string val)    { variables[name] = new Variable(val); }
void Script::setVar(string name, int val)       { variables[name] = new Variable(val); }
void Script::unsetVar(string name) {
    // Ensure that the variable at least exists, but has no value
    if (variables.find(name) != variables.end())
        variables[name]->unset();
    else variables[name] = new Variable;
}

Script::State Script::getState()        { return state; }
string Script::getTarget()              { return target; }
Script::InputType Script::getInputType(){ return inputType; }
string Script::getChoices()             { return choices; }
string Script::getInputName()           { return inputName; }
int Script::getInputMaxLen()            { return inputMaxLen; }

/**
 * Translates a script string with dynamic variables
 */ 
void Script::translate(string *text) {
    unsigned int pos;
    bool nochars = true;
    xmlNodePtr node = this->translationContext.back();
    
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
        
        /**
         * Handle embedded items
         */
        int num_embedded = 0;
        int total_pos = 0;
        string current = item;
        while (true) {
            unsigned int open = current.find_first_of("{"),
                         close = current.find_first_of("}");

            if (close == current.length())
                errorFatal("Error: no closing } found in script.");

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

        // Get defined variables
        if (item[0] == '$') {
            string varName = item.substr(1);
            if (variables.find(varName) != variables.end())
                prop = variables[varName]->getString();
        }        
        // Get the current iterator for our loop
        else if (item == "iterator")
            prop = to_string(this->iterator);        
        else if ((pos = item.find("show_inventory:")) < item.length()) {
            pos = item.find(":");
            string itemScript = item.substr(pos+1);

            xmlNodePtr itemShowScript = find(node, itemScript);

            xmlNodePtr item;
            prop.erase();
            
            /**
             * Save iterator
             */ 
            int oldIterator = this->iterator;            

            /* start iterator at 0 */
            this->iterator = 0;
            
            for (item = node->children; item; item = item->next) {
                if (xmlStrcmp(item->name, (const xmlChar *)nounName.c_str()) == 0) {
                    bool hidden = (bool)xmlGetPropAsBool(item, "hidden");                    

                    if (!hidden) {
                        /* make sure the item's requisites are met */
                        if (!xmlPropExists(item, "req") || compare(getPropAsStr(item, "req"))) {
                            /* put a newline after each */
                            if (this->iterator > 0)
                                prop += "\n";                            

                            /* set translation context to item */
                            translationContext.push_back(item);
                            execute(itemShowScript, NULL, &prop);
                            translationContext.pop_back();

                            this->iterator++;
                        }
                    }                    
                }
            }
            
            /**
             * Restore iterator to previous value
             */             
            this->iterator = oldIterator;
        }

        /**
         * Make a string containing the available ids using the
         * vendor's inventory (i.e. "bcde")
         */ 
        else if (item == "inventory_choices") {
            xmlNodePtr item;
            string ids;

            for (item = node->children; item; item = item->next) {
                if (xmlStrcmp(item->name, (const xmlChar *)nounName.c_str()) == 0) {
                    string id = getPropAsStr(item, idPropName.c_str());
                    /* make sure the item's requisites are met */
                    if (!xmlPropExists(item, "req") || (compare(getPropAsStr(item, "req"))))
                        ids += id[0];
                }
            }

            prop = ids;
        }

        /**
         * Ask our providers if they have a valid translation for us
         */
        else if (item.find_first_of(":") != string::npos) {
            int pos = item.find_first_of(":");
            string provider = item;
            string to_find;

            provider = item.substr(0, pos);
            to_find = item.substr(pos + 1);
            if (providers.find(provider) != providers.end()) {
                std::vector<string> parts = split(to_find, ":");
                Provider* p = providers[provider];
                prop = p->translate(parts);
            }
        }
        
        /**
         * Resolve as a property name or a function
         */
        else {            
            string funcName, content;

            funcParse(item, &funcName, &content);
            
            /*
             * Check to see if it's a property name
             */
            if (funcName.empty()) {
                /* we have the property name, now go get the property value! */
                prop = getPropAsStr(translationContext, item, true);
            }
            
            /**
             * We have a function, make it work!
             */ 
            else {
                /* perform the <math> function on the content */
                if (funcName == "math") {
                    if (content.empty())
                        errorWarning("Error: empty math() function");

                    prop = to_string(mathValue(content));
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
                else if (funcName == "random")
                    prop = to_string(xu4_random((int)strtol(content.c_str(), NULL, 10)));
                
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
 xmlNodePtr Script::find(xmlNodePtr node, string script_to_find, string id, bool _default) {
    xmlNodePtr current;
    if (node) {
        for (current = node->children; current; current = current->next) {
            if (!xmlNodeIsText(current) && (script_to_find == (char *)current->name)) {
                if (id.empty() && !xmlPropExists(current, idPropName.c_str()) && !_default)
                    return current;
                else if (xmlPropExists(current, idPropName.c_str()) && (id == xmlGetPropAsString(current, idPropName.c_str())))
                    return current;
                else if (_default && xmlPropExists(current, "default") && xmlGetPropAsBool(current, "default"))
                    return current;
            }
        }

        /* only search the parent nodes if we haven't hit the base <script> node */
        if (xmlStrcmp(node->name, (const xmlChar *)"script") != 0)
            current = find(node->parent, script_to_find, id);

        /* find the default script instead */
        if (!current && !id.empty() && !_default)
            current = find(node, script_to_find, "", true);
        return current;        
    }
    return NULL;
}

/**
 * Gets a property as string from the script, and
 * translates it using scriptTranslate.
 */ 
string Script::getPropAsStr(std::list<xmlNodePtr>& nodes, string prop, bool recursive) {
    string propvalue;
    std::list<xmlNodePtr>::reverse_iterator i;
    
    for (i = nodes.rbegin(); i != nodes.rend(); i++) {
        xmlNodePtr node = *i;
        if (xmlPropExists(node, prop.c_str())) {
            propvalue = xmlGetPropAsString(node, prop.c_str());
            break;
        }
    }

    if (propvalue.empty() && recursive) {
        for (i = nodes.rbegin(); i != nodes.rend(); i++) {
            xmlNodePtr node = *i;
            if (node->parent) {
                propvalue = getPropAsStr(node->parent, prop, recursive);
                break;
            }
        }
    }
    
    translate(&propvalue);
    return propvalue;
}
string Script::getPropAsStr(xmlNodePtr node, string prop, bool recursive) {
    std::list<xmlNodePtr> list;
    list.push_back(node);
    return getPropAsStr(list, prop, recursive);    
}

/**
 * Gets a property as int from the script
 */ 
int Script::getPropAsInt(std::list<xmlNodePtr>& nodes, string prop, bool recursive) {
    string propvalue = getPropAsStr(nodes, prop, recursive);
    return mathValue(propvalue);
}
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
 * Sets a new translation context for the script
 */ 
Script::ReturnCode Script::pushContext(xmlNodePtr script, xmlNodePtr current) {
    string nodeName = getPropAsStr(current, "name");
    string search_id;

    if (xmlPropExists(current, idPropName.c_str()))         
        search_id = getPropAsStr(current, idPropName);
    else if (variables.find(idPropName) != variables.end()) {
        if (variables[idPropName]->isSet())
            search_id = variables[idPropName]->getString();
        else search_id = "null";
    }

    // When looking for a new context, start from within our old one
    translationContext.push_back(find(translationContext.back(), nodeName, search_id));
    if (debug) {
        if (!this->translationContext.back())
            fprintf(debug, "\nWarning!!! Invalid translation context <%s %s=\"%s\" ...>", nodeName.c_str(), idPropName.c_str(), search_id.c_str());
        else fprintf(debug, "\nChanging translation context to <%s %s=\"%s\" ...>", nodeName.c_str(), idPropName.c_str(), search_id.c_str());
    }

    return RET_OK;
}

/**
 * Removes a node from the translation context
 */ 
Script::ReturnCode Script::popContext(xmlNodePtr script, xmlNodePtr current) {
    if (translationContext.size() > 1) {
        translationContext.pop_back();
        if (debug)
            fprintf(debug, "\nReverted translation context to <%s ...>", translationContext.back()->name);
    }
    return RET_OK;
}

/**
 * End script execution
 */ 
Script::ReturnCode Script::end(xmlNodePtr script, xmlNodePtr current) {
    /**
     * See if there's a global 'end' node declared for cleanup
     */
    xmlNodePtr endScript = find(scriptNode, "end");
    if (endScript)
        execute(endScript);

    if (debug)
        fprintf(debug, "\n<End script>");
    
    this->state = STATE_DONE;
    
    return RET_STOP;
}

/**
 * Wait for keypress from the user
 */ 
Script::ReturnCode Script::waitForKeypress(xmlNodePtr script, xmlNodePtr current) {
    this->currentScript = script;
    this->currentItem = current;
    this->choices = "abcdefghijklmnopqrstuvwxyz01234567890\015 \033";
    this->target.erase();
    this->state = STATE_INPUT;
    this->inputType = INPUT_KEYPRESS;

    if (debug)
        fprintf(debug, "\n<Wait>");

    return RET_STOP;
}

/**
 * Redirects script execution to another script
 */ 
Script::ReturnCode Script::redirect(xmlNodePtr script, xmlNodePtr current) {
    string target;
    
    if (xmlPropExists(current, "redirect"))
        target = getPropAsStr(current, "redirect");
    else target = getPropAsStr(current, "target");

    /* set a new search id */
    string search_id = getPropAsStr(current, idPropName);
    
    xmlNodePtr newScript = find(this->scriptNode, target, search_id);
    if (!newScript)
        errorFatal("Error: redirect failed -- could not find target script '%s' with %s=\"%s\"", target.c_str(), idPropName.c_str(), search_id.c_str());

    if (debug) {
        fprintf(debug, "\nRedirected to <%s", target.c_str());
        if (search_id.length())
            fprintf(debug, " %s=\"%s\"", idPropName.c_str(), search_id.c_str());
        fprintf(debug, " .../>");
    }
    
    execute(newScript);
    return RET_REDIRECTED;
}

/**
 * Includes a script to be executed
 */ 
Script::ReturnCode Script::include(xmlNodePtr script, xmlNodePtr current) {
    string scriptName = getPropAsStr(current, "script");
    string id = getPropAsStr(current, idPropName);

    xmlNodePtr newScript = find(this->scriptNode, scriptName, id);
    if (!newScript)
        errorFatal("Error: include failed -- could not find target script '%s' with %s=\"%s\"", scriptName.c_str(), idPropName.c_str(), id.c_str());

    if (debug) {
        fprintf(debug, "\nIncluded script <%s", scriptName.c_str());
        if (id.length())
            fprintf(debug, " %s=\"%s\"", idPropName.c_str(), id.c_str());
        fprintf(debug, " .../>");
    }

    execute(newScript);
    return RET_OK;
}

/**
 * Waits a given number of milliseconds before continuing execution
 */ 
Script::ReturnCode Script::wait(xmlNodePtr script, xmlNodePtr current) {
    int msecs = getPropAsInt(current, "msecs");
    EventHandler::wait_msecs(msecs);    
    return RET_OK;
}

/**
 * Executes a 'for' loop script
 */ 
Script::ReturnCode Script::forLoop(xmlNodePtr script, xmlNodePtr current) {
    Script::ReturnCode retval = RET_OK;
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
        if ((retval == RET_REDIRECTED) || (retval == RET_STOP))
            break;
    }

    /* restore the previous iterator */
    this->iterator = oldIterator;

    return retval;
}

/**
 * Randomely executes script code
 */ 
Script::ReturnCode Script::random(xmlNodePtr script, xmlNodePtr current) {
    int perc = getPropAsInt(current, "chance");
    int num = xu4_random(100);
    Script::ReturnCode retval = RET_OK;

    if (num < perc)
        retval = execute(current);

    if (debug)
        fprintf(debug, "\nRandom (%d%%): rolled %d (%s)", perc, num, (num < perc) ? "Succeeded" : "Failed");

    return retval;
}

/**
 * Moves the player's current position
 */ 
Script::ReturnCode Script::move(xmlNodePtr script, xmlNodePtr current) {
    if (xmlPropExists(current, "x"))
        c->location->coords.x = getPropAsInt(current, "x");
    if (xmlPropExists(current, "y"))
        c->location->coords.y = getPropAsInt(current, "y");
    if (xmlPropExists(current, "z"))
        c->location->coords.z = getPropAsInt(current, "z");

    if (debug)
        fprintf(debug, "\nMove: x-%d y-%d z-%d", c->location->coords.x, c->location->coords.y, c->location->coords.z);
    
    gameUpdateScreen();
    return RET_OK;
}

/**
 * Puts the player to sleep. Useful when coding inn scripts
 */ 
Script::ReturnCode Script::sleep(xmlNodePtr script, xmlNodePtr current) {
    if (debug)
        fprintf(debug, "\nSleep!\n");

    CombatController *cc = new InnController();
    cc->begin();

    return RET_OK;
}

/**
 * Enables/Disables the keyboard cursor
 */ 
Script::ReturnCode Script::cursor(xmlNodePtr script, xmlNodePtr current) {
    bool enable = (bool)xmlGetPropAsBool(current, "enable");
    if (enable)
        screenEnableCursor();
    else screenDisableCursor();

    return RET_OK;
}

/**
 * Pay gold to someone
 */ 
Script::ReturnCode Script::pay(xmlNodePtr script, xmlNodePtr current) {    
    int price = getPropAsInt(current, "price");
    int quant = getPropAsInt(current, "quantity");       

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
        return RET_STOP;
    }
    else c->party->adjustGold(-price);

    if (debug)
        fprintf(debug, "\n\tBalance:     %d\n", c->saveGame->gold);

    return RET_OK;
}

/**
 * Perform a limited 'if' statement
 */ 
Script::ReturnCode Script::_if(xmlNodePtr script, xmlNodePtr current) {
    string test = getPropAsStr(current, "test");
    Script::ReturnCode retval = RET_OK;

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
Script::ReturnCode Script::input(xmlNodePtr script, xmlNodePtr current) {
    string type = getPropAsStr(current, "type");
            
    this->currentScript = script;
    this->currentItem = current;

    if (xmlPropExists(current, "target"))
        this->target = getPropAsStr(current, "target");
    else this->target.erase();

    this->state = STATE_INPUT;
    this->inputName = "input";

    // Does the variable have a maximum length?
    if (xmlPropExists(current, "maxlen"))
        this->inputMaxLen = getPropAsInt(current, "maxlen");
    else this->inputMaxLen = Conversation::BUFFERLEN;

    // Should we name the variable something other than "input"
    if (xmlPropExists(current, "name"))
        this->inputName = getPropAsStr(current, "name");
    else {
        if (type == "choice")
            this->inputName = idPropName;
    }
        
    if (type == "number")
        this->inputType = INPUT_NUMBER;
    else if (type == "keypress")
        this->inputType = INPUT_KEYPRESS;
    else if (type == "choice") {
        this->inputType = INPUT_CHOICE;
        this->choices = getPropAsStr(current, "options");
        this->choices += " \015\033";
    }
    else if (type == "text")
        this->inputType = INPUT_STRING;
    else if (type == "direction")
        this->inputType = INPUT_DIRECTION;
    else if (type == "player")
        this->inputType = INPUT_PLAYER;        

    if (debug)
        fprintf(debug, "\nInput: %s", type.c_str());

    /* the script stops here, at least for now */
    return RET_STOP;    
}

/**
 * Add item to inventory
 */ 
Script::ReturnCode Script::add(xmlNodePtr script, xmlNodePtr current) {
    string type = getPropAsStr(current, "type");
    string subtype = getPropAsStr(current, "subtype");
    int quant = getPropAsInt(this->translationContext.back(), "quantity");
    if (quant == 0)
        quant = getPropAsInt(current, "quantity");
    else
        quant *= getPropAsInt(current, "quantity");

    if (debug) {
        fprintf(debug, "\nAdd: %s ", type.c_str());
        if (subtype.length())
            fprintf(debug, "- %s ", subtype.c_str());
    }

    if (type == "gold")        
        c->party->adjustGold(quant);    
    else if (type == "food") {
        quant *= 100;
        c->party->adjustFood(quant);
    }
    else if (type == "horse")
        c->party->setTransport(Tileset::findTileByName("horse")->getId());
    else if (type == "torch") {
        AdjustValueMax(c->saveGame->torches, quant, 99);
        c->party->notifyOfChange(0, PartyEvent::INVENTORY_ADDED);
    }
    else if (type == "gem") {
        AdjustValueMax(c->saveGame->gems, quant, 99);
        c->party->notifyOfChange(0, PartyEvent::INVENTORY_ADDED);
    }
    else if (type == "key") {
        AdjustValueMax(c->saveGame->keys, quant, 99);
        c->party->notifyOfChange(0, PartyEvent::INVENTORY_ADDED);
    }
    else if (type == "sextant") {
        AdjustValueMax(c->saveGame->sextants, quant, 99);
        c->party->notifyOfChange(0, PartyEvent::INVENTORY_ADDED);
    }
    else if (type == "weapon") {
        AdjustValueMax(c->saveGame->weapons[subtype[0] - 'a'], quant, 99);
        c->party->notifyOfChange(0, PartyEvent::INVENTORY_ADDED);
    }
    else if (type == "armor") {
        AdjustValueMax(c->saveGame->armor[subtype[0] - 'a'], quant, 99);
        c->party->notifyOfChange(0, PartyEvent::INVENTORY_ADDED);
    }
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
            c->party->notifyOfChange(0, PartyEvent::INVENTORY_ADDED);
            c->stats->resetReagentsMenu();
        }
        else errorWarning("Error: reagent '%s' not found", subtype.c_str());
    }

    if (debug)
        fprintf(debug, "(x%d)", quant);

    return RET_OK;
}

/**
 * Lose item
 */ 
Script::ReturnCode Script::lose(xmlNodePtr script, xmlNodePtr current) {
    string type = getPropAsStr(current, "type");
    string subtype = getPropAsStr(current, "subtype");
    int quant = getPropAsInt(current, "quantity");

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

    return RET_OK;
}

/**
 * Heals a party member
 */ 
Script::ReturnCode Script::heal(xmlNodePtr script, xmlNodePtr current) {
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

    return RET_OK;
}

/**
 * Performs all of the visual/audio effects of casting a spell
 */ 
Script::ReturnCode Script::castSpell(xmlNodePtr script, xmlNodePtr current) {
    extern SpellEffectCallback spellEffectCallback;
    (*spellEffectCallback)('r', -1, SOUND_MAGIC);
    if (debug)
        fprintf(debug, "\n<Spell effect>");

    return RET_OK;
}

/**
 * Apply damage to a player
 */ 
Script::ReturnCode Script::damage(xmlNodePtr script, xmlNodePtr current) {
    int player = getPropAsInt(current, "player") - 1;
    int pts = getPropAsInt(current, "pts");
    PartyMember *p;
    
    p = c->party->member(player);
    p->applyDamage(pts);

    if (debug)
        fprintf(debug, "\nDamage: %d damage to player %d", pts, player + 1);

    return RET_OK;
}

/**
 * Apply karma changes based on the action taken
 */ 
Script::ReturnCode Script::karma(xmlNodePtr script, xmlNodePtr current) {
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
        action_map["fled_good"]             = KA_FLED_GOOD;
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

    return RET_OK;

}

/**
 * Set the currently playing music
 */ 
Script::ReturnCode Script::music(xmlNodePtr script, xmlNodePtr current) {
    if (xmlGetPropAsBool(current, "reset"))        
        musicMgr->play();
    else {
        string type = getPropAsStr(current, "type");

        if (xmlGetPropAsBool(current, "play"))
            musicMgr->play();
        if (xmlGetPropAsBool(current, "stop"))
            musicMgr->stop();
        else if (type == "shopping")
            musicMgr->shopping();
        else if (type == "camp")
            musicMgr->camp();
    }

    return RET_OK;
}

/**
 * Sets a variable
 */ 
Script::ReturnCode Script::setVar(xmlNodePtr script, xmlNodePtr current) {
    string name = getPropAsStr(current, "name");
    string value = getPropAsStr(current, "value");

    if (name.empty()) {
        if (debug)
            fprintf(debug, "Variable name empty!");
        return RET_STOP;
    }
    
    variables[name] = new Variable(value);

    if (debug)
        fprintf(debug, "\nSet Variable: %s=%s", name.c_str(), variables[name]->getString().c_str());

    return RET_OK;
}

/**
 * Display a different ztats screen
 */ 
Script::ReturnCode Script::ztats(xmlNodePtr script, xmlNodePtr current) {
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
            c->stats->setView(view->second); /* change it! */
        else if (debug)
            fprintf(debug, " <FAILED - view could not be found>");
    }
    else c->stats->setView(STATS_PARTY_OVERVIEW);

    return RET_OK;
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
            
            mathParseChildren(current, &children_results);
            *result = to_string(mathValue(children_results));            
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
        if (pos >= contents->length())
            errorWarning("Error: No closing ) in function %s()", funcName->c_str());
        else *contents = contents->substr(0, pos);
    }
    else funcName->erase();
}
