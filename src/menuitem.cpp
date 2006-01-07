/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include "debug.h"
#include "error.h"
#include "menu.h"
#include "menuitem.h"

/**
 * MenuItem class
 */
MenuItem::MenuItem(string t, short xpos, short ypos, int sc) :
    id(-1),
    x(xpos),
    y(ypos),
    text(t),
    highlighted(false),
    selected(false),
    visible(true),
    scOffset(sc),
    closesMenu(false)
{
    // if the sc/scOffset is outside the range of the text string, assert
    ASSERT(sc==-1 || (sc >= 0 && sc <= (int)text.length()), "sc value of %d out of range!", sc);
    addShortcutKey(tolower(text[sc]));
}

int MenuItem::getId() const                         { return id; }
short MenuItem::getX() const                        { return x; }
short MenuItem::getY() const                        { return y; }
int MenuItem::getScOffset() const                   { return scOffset; }
string MenuItem::getText() const                    { return text; }
bool MenuItem::isHighlighted() const                { return highlighted; }
bool MenuItem::isSelected() const                   { return selected; }
bool MenuItem::isVisible() const                    { return visible; }
const set<int> &MenuItem::getShortcutKeys() const   { return shortcutKeys; }
bool MenuItem::getClosesMenu() const                { return closesMenu; }

void MenuItem::setId(int i) {
    id = i;
}

void MenuItem::setX(int xpos) {
    x = xpos;
}

void MenuItem::setY(int ypos) {
    y = ypos;
}

void MenuItem::setText(string t) {
    text = t;
}

void MenuItem::setHighlighted(bool h) {
    highlighted = h;
}

void MenuItem::setSelected(bool s) {
    selected = s;
}

void MenuItem::setVisible(bool v) {
    visible = v;
}

void MenuItem::addShortcutKey(int sc) {
    shortcutKeys.insert(sc);
}

void MenuItem::setClosesMenu(bool closesMenu) {
    this->closesMenu = closesMenu;
}

BoolMenuItem::BoolMenuItem(string text, short x, short y, int shortcutKey, bool *val) : 
    MenuItem(text, x, y, shortcutKey),
    val(val),
    on("On"),
    off("Off")
{
}

BoolMenuItem *BoolMenuItem::setValueStrings(const string &onString, const string &offString) {
    on = onString;
    off = offString;
    return this;
}

string BoolMenuItem::getText() const { 
    char buffer[64];
    snprintf(buffer, sizeof(buffer), text.c_str(), *val ? on.c_str() : off.c_str());
    return buffer;
}

void BoolMenuItem::activate(MenuEvent &event) { 
    if (event.getType() == MenuEvent::DECREMENT || 
        event.getType() == MenuEvent::INCREMENT || 
        event.getType() == MenuEvent::ACTIVATE)
        *val = !(*val);
}

StringMenuItem::StringMenuItem(string text, short x, short y, int shortcutKey, string *val, const vector<string> &validSettings) : 
    MenuItem(text, x, y, shortcutKey),
    val(val),
    validSettings(validSettings)
{
}

string StringMenuItem::getText() const { 
    char buffer[64];
    snprintf(buffer, sizeof(buffer), text.c_str(), val->c_str());
    return buffer;
}

void StringMenuItem::activate(MenuEvent &event) { 
    vector<string>::const_iterator current = find(validSettings.begin(), validSettings.end(), *val);

    if (current == validSettings.end())
        errorFatal("Error: menu string '%s' not a valid choice", val->c_str());
            
    if (event.getType() == MenuEvent::INCREMENT || event.getType() == MenuEvent::ACTIVATE) {
        /* move to the next valid choice, wrapping if necessary */
        current++;
        if (current == validSettings.end())
            current = validSettings.begin();    
        *val = *current;

    } else if (event.getType() == MenuEvent::DECREMENT) {
        /* move back one, wrapping if necessary */
        if (current == validSettings.begin())
            current = validSettings.end();
        current--;
        *val = *current;
    }
}

IntMenuItem::IntMenuItem(string text, short x, short y, int shortcutKey, int *val, int min, int max, int increment, menuOutputType output) :
    MenuItem(text, x, y, shortcutKey),
    val(val),
    min(min),
    max(max),
    increment(increment),
    output(output)
{
}

string IntMenuItem::getText() const {
    // do custom formatting for some menu entries,
    // and generate a string of the results
    char outputBuffer[10];
    switch (output){
        case MENU_OUTPUT_GAMMA:
            snprintf(outputBuffer, sizeof(outputBuffer), "%.1f", static_cast<float>(*val) / 100);
            break;
        case MENU_OUTPUT_SPELL:
            snprintf(outputBuffer, sizeof(outputBuffer), "%3g sec", static_cast<double>(*val) / 5);
            break;
        case MENU_OUTPUT_SHRINE:
/*
 * is this code really necessary?  the increments/decrements can be handled by IntMenuItem(),
 * as well as the looping once the max is reached.  more importantly, the minimum value is
 * inconstant, and based upon another setting that can be changed independent of this one.
 * This variable could be set to it's minimum value, but when the gameCyclesPerSecond setting
 * is changed, the value of this setting could become out of bounds.
 *
 * settings.shrineTime is only used in one function within shrine.cpp, and that code appears
 * to handle the min value, caping the minimum interval at 1.
 *  
            // make sure that the setting we're trying for is even possible
            if (event.getType() == MenuEvent::INCREMENT || event.getType() == MenuEvent::ACTIVATE) {
                settingsChanged.shrineTime++;
                if (settingsChanged.shrineTime > MAX_SHRINE_TIME)
                    settingsChanged.shrineTime = MEDITATION_MANTRAS_PER_CYCLE / settingsChanged.gameCyclesPerSecond;
            } else if (event.getType() == MenuEvent::DECREMENT) {
                settingsChanged.shrineTime--;
                if (settingsChanged.shrineTime < (MEDITATION_MANTRAS_PER_CYCLE / settingsChanged.gameCyclesPerSecond))
                    settingsChanged.shrineTime = MAX_SHRINE_TIME;
            }
 *
 */
            snprintf(outputBuffer, sizeof(outputBuffer), "%d sec", *val);
            break;
        default:
            break;
    }

    // the buffer must contain a field character %d or %s depending
    // on the menuOutputType selected. MENU_OUTPUT_INT always uses
    // %d, whereas all others use %s
    char buffer[64];
    if (output != MENU_OUTPUT_INT)
        snprintf(buffer, sizeof(buffer), text.c_str(), outputBuffer);
    else
        snprintf(buffer, sizeof(buffer), text.c_str(), *val);
    return buffer;
}

void IntMenuItem::activate(MenuEvent &event) { 
    if (event.getType() == MenuEvent::INCREMENT || event.getType() == MenuEvent::ACTIVATE) {
        *val += increment;
        if (*val > max)
            *val = min;

    } else if (event.getType() == MenuEvent::DECREMENT) {
        *val -= increment;
        if (*val < min)
            *val = max;
    }
}
