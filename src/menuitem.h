/*
 * $Id$
 */

#ifndef MENUITEM_H
#define MENUITEM_H

#include <string>
#include <set>
#include <vector>

using std::string;
using std::set;
using std::vector;

class MenuEvent;

class MenuItem {
public:
    MenuItem(string text, short x, short y, int shortcutKey = 0);
    virtual ~MenuItem() {}

    virtual void activate(MenuEvent &event) {}

    // Accessor Methods
    int getId() const;
    short getX() const;
    short getY() const;
    virtual string getText() const;
    bool isHighlighted() const;
    bool isSelected() const;
    bool isVisible() const;
    const set<int> &getShortcutKeys() const;
    bool getClosesMenu() const;

    void setId(int id);
    void setX(int xpos);
    void setY(int ypos);
    void setText(string text);
    void setHighlighted(bool h = true);
    void setSelected(bool s = true);
    void setVisible(bool v = true);
    void addShortcutKey(int shortcutKey);
    void setClosesMenu(bool closesMenu);
    
protected:
    int id;
    short x, y;
    string text;
    bool highlighted;
    bool selected;
    bool visible;    
    set<int> shortcutKeys;
    bool closesMenu;
};

/**
 * A menu item that toggles a boolean value, and displays the current
 * setting as part of the text.
 */
class BoolMenuItem : public MenuItem {
public:
    BoolMenuItem(string text, short x, short y, int shortcutKey, bool *val);

    BoolMenuItem *setValueStrings(const string &onString, const string &offString);

    virtual void activate(MenuEvent &event);
    virtual string getText() const;

protected:
    bool *val;
    string on, off;
};

/**
 * A menu item that cycles through a list of possible string values, and
 * displays the current setting as part of the text.
 */
class StringMenuItem : public MenuItem {
public:
    StringMenuItem(string text, short x, short y, int shortcutKey, string *val, const vector<string> &validSettings);

    virtual void activate(MenuEvent &event);
    virtual string getText() const;

protected:
    string *val;
    vector<string> validSettings;
};

/**
 * A menu item that cycles through a list of possible integer values,
 * and displays the current setting as part of the text.
 */
class IntMenuItem : public MenuItem {
public:
    IntMenuItem(string text, short x, short y, int shortcutKey, int *val, int min, int max, int increment);

    virtual void activate(MenuEvent &event);
    virtual string getText() const;

protected:
    int *val;
    int min, max, increment;
};

#endif
