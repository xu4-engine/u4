/*
 * $Id$
 */

#ifndef MENU_H
#define MENU_H

#include <list>
#include <set>
#include <string>
#include "controller.h"
#include "observable.h"
#include "types.h"

using std::string;
using std::set;

typedef enum {
    ACTIVATE_NORMAL     = 0x1,
    ACTIVATE_INCREMENT  = 0x2,
    ACTIVATE_DECREMENT  = 0x4
} ActivateAction;

#define ACTIVATE_ANY    (ActivateAction)(0xFFFF)

class MenuItem;

typedef short MenuId;
typedef void (*ActivateMenuItem)(MenuItem*, ActivateAction);

class MenuItem {
    /* we want our menu to be able to modify us
       without notifying their observsers a bunch
       of unnecessary times */
    friend class Menu;

public:
    MenuItem(class Menu *m, MenuId id, string text, short x, short y, ActivateMenuItem activate, ActivateAction activateOn, int shortcutKey = 0);

    void notifyOfChange(string arg);

    // Accessor Methods
    MenuId getId() const;
    short getX() const;
    short getY() const;
    string getText() const;
    bool isHighlighted() const;
    bool isSelected() const;
    bool isVisible() const;
    ActivateMenuItem getActivateFunc() const;
    ActivateAction getActivateAction() const;
    const set<int> &getShortcutKeys() const;
    bool getClosesMenu() const;

    void setId(MenuId id);
    void setX(int xpos);
    void setY(int ypos);
    void setText(string text);
    void setHighlighted(bool h = true);
    void setSelected(bool s = true);
    void setVisible(bool v = true);
    void setActivateFunc(ActivateMenuItem ami);
    void setActivateAction(ActivateAction aa);
    void addShortcutKey(int shortcutKey);
    void setClosesMenu(bool closesMenu);
    
private:
    class Menu* menu;
    MenuId id;
    short x, y;
    string text;
    bool highlighted;
    bool selected;
    bool visible;    
    ActivateMenuItem activateMenuItem;
    ActivateAction activateOn;
    set<int> shortcutKeys;
    bool closesMenu;
};

/**
 * Menu class definition
 */
class Menu : public Observable<string> {
    friend class MenuItem;
public:
    typedef std::list<MenuItem> MenuItemList;

public:
    Menu() : closed(false) {}

    void                    add(MenuId id, string text, short x, short y, ActivateMenuItem activate, ActivateAction activateOn, int shortcutKey = 0);
    void                    addShortcutKey(MenuId id, int shortcutKey);
    void                    setClosesMenu(MenuId id);
    MenuItemList::iterator  getCurrent();
    void                    setCurrent(MenuItemList::iterator i);
    void                    setCurrent(MenuId id);
    void                    show();
    bool                    isVisible();
    void                    next();
    void                    prev();
    void                    highlight(MenuItem *item);
    MenuItemList::iterator  begin();
    MenuItemList::iterator  end();
    MenuItemList::iterator  begin_visible();
    void                    reset(bool highlightFirst = true);
    MenuItemList::iterator  getById(MenuId id);
    MenuItem*               getItemById(MenuId id);
    void                    activateItem(MenuId id, ActivateAction action);
    bool                    activateItemByShortcut(int key, ActivateAction action);
    bool                    getClosed() const;
    void                    setClosed(bool closed);

private:    
    MenuItemList items;
    MenuItemList::iterator current;
    MenuItemList::iterator selected;
    bool closed;
};

/**
 * This class controls a menu.
 */
class MenuController : public Controller {
public:
    MenuController(Menu *menu);
    bool keyPressed(int key);

    void waitFor();

protected:
    Menu *menu;
    bool exitWhenDone;
};

#endif
