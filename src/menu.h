/*
 * $Id$
 */

#ifndef MENU_H
#define MENU_H

#include <list>
#include <string>
#include "observable.h"
#include "types.h"

using std::string;

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
    MenuItem(class Menu *m, MenuId id, string text, short x, short y, ActivateMenuItem activate, ActivateAction activateOn);

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

    void setId(MenuId id);
    void setX(int xpos);
    void setY(int ypos);
    void setText(string text);
    void setHighlighted(bool h = true);
    void setSelected(bool s = true);
    void setVisible(bool v = true);
    void setActivateFunc(ActivateMenuItem ami);
    void setActivateAction(ActivateAction aa);
    
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
};

/**
 * Menu class definition
 */
class Menu : public Observable<string> {
    friend class MenuItem;
public:
    typedef std::list<MenuItem> MenuItemList;

public:
    Menu() {}

    void                    add(MenuId id, string text, short x, short y, ActivateMenuItem activate, ActivateAction activateOn);
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

private:    
    MenuItemList items;
    MenuItemList::iterator current;
    MenuItemList::iterator selected;
};

#endif
