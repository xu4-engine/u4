/*
 * $Id$
 */

#ifndef MENU_H
#define MENU_H

#include <list>
#include <set>
#include <string>
#include "event.h"
#include "observable.h"
#include "types.h"

using std::string;
using std::set;

class Menu;
class MenuItem;

typedef short MenuId;

class MenuEvent {
public:
    enum Type {
        ACTIVATE,
        INCREMENT,
        DECREMENT,
        SELECT,
        RESET
    };

    MenuEvent(const Menu *menu, Type type, const MenuItem *item = NULL) {
        this->menu = menu;
        this->type = type;
        this->item = item;
    }

    const Menu *getMenu() { return menu; }
    Type getType() { return type; }
    const MenuItem *getMenuItem() { return item; }

private:
    const Menu *menu;
    Type type;
    const MenuItem *item;
};

class MenuItem {
    /* we want our menu to be able to modify us
       without notifying their observsers a bunch
       of unnecessary times */
    friend class Menu;

public:
    MenuItem(class Menu *m, MenuId id, string text, short x, short y, int shortcutKey = 0);

    // Accessor Methods
    MenuId getId() const;
    short getX() const;
    short getY() const;
    string getText() const;
    bool isHighlighted() const;
    bool isSelected() const;
    bool isVisible() const;
    const set<int> &getShortcutKeys() const;
    bool getClosesMenu() const;

    void setId(MenuId id);
    void setX(int xpos);
    void setY(int ypos);
    void setText(string text);
    void setHighlighted(bool h = true);
    void setSelected(bool s = true);
    void setVisible(bool v = true);
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
    set<int> shortcutKeys;
    bool closesMenu;
};

/**
 * Menu class definition
 */
class Menu : public Observable<MenuEvent &> {
public:
    typedef std::list<MenuItem> MenuItemList;

public:
    Menu() : closed(false) {}

    void                    add(MenuId id, string text, short x, short y, int shortcutKey = 0);
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
    void                    activateItem(MenuId id, MenuEvent::Type action);
    bool                    activateItemByShortcut(int key, MenuEvent::Type action);
    bool                    getClosed() const;
    void                    setClosed(bool closed);

private:    
    MenuItemList items;
    MenuItemList::iterator current;
    MenuItemList::iterator selected;
    bool closed;
};

/**
 * This class controls a menu.  The value field of WaitableController
 * isn't used.
 */
class MenuController : public WaitableController<void *> {
public:
    MenuController(Menu *menu);
    bool keyPressed(int key);

protected:
    Menu *menu;
};

#endif
