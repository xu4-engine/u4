/*
 * $Id$
 */

#ifndef MENU_H
#define MENU_H

#include <list>
#include <string>
#include "types.h"

using std::string;

typedef enum {
    ACTIVATE_NORMAL     = 0x1,
    ACTIVATE_INCREMENT  = 0x2,
    ACTIVATE_DECREMENT  = 0x4
} ActivateAction;

#define ACTIVATE_ANY    (ActivateAction)(0xFFFF)

struct _MenuItem;

typedef short MenuId;
typedef void (*ActivateMenuItem)(struct _MenuItem*, ActivateAction);

typedef struct _MenuItem {    
    MenuId id;
    short x, y;
    string text;
    bool isHighlighted;
    bool isVisible;
    bool isSelected;
    ActivateMenuItem activateMenuItem;
    ActivateAction activateOn;    
} MenuItem;

/**
 * Menu class definition
 */
class Menu {
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
    void                    reset();
    MenuItemList::iterator  getById(MenuId id);
    MenuItem*               getItemById(MenuId id);
    void                    activateItem(MenuId id, ActivateAction action);    

private:    
    MenuItemList items;
    MenuItemList::iterator current;
    MenuItemList::iterator selected;
};

#endif
