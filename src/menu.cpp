/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include "menu.h"

#include "error.h"
#include "event.h"
#include "screen.h"

/**
 * MenuItem class
 */
MenuItem::MenuItem(class Menu *m, MenuId i, string t, short xpos, short ypos, int sc) :
    menu(m),
    id(i),
    x(xpos),
    y(ypos),
    text(t),
    highlighted(false),
    selected(false),
    visible(true),
    closesMenu(false)
{    
    addShortcutKey(sc);
}

MenuId MenuItem::getId() const                      { return id; }
short MenuItem::getX() const                        { return x; }
short MenuItem::getY() const                        { return y; }
string MenuItem::getText() const                    { return text; }
bool MenuItem::isHighlighted() const                { return highlighted; }
bool MenuItem::isSelected() const                   { return selected; }
bool MenuItem::isVisible() const                    { return visible; }
const set<int> &MenuItem::getShortcutKeys() const   { return shortcutKeys; }
bool MenuItem::getClosesMenu() const                { return closesMenu; }

void MenuItem::setId(MenuId i) {
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

/**
 * Adds an item to the menu list and returns the menu
 */
void Menu::add(MenuId id, string text, short x, short y, int sc) {
    items.push_back(MenuItem(this, id, text, x, y, sc));
}

void Menu::addShortcutKey(MenuId id, int shortcutKey) {
    for (MenuItemList::iterator i = items.begin(); i != items.end(); i++) {
        if (i->getId() == id) {
            i->addShortcutKey(shortcutKey);
            break;
        }
    }    
}

void Menu::setClosesMenu(MenuId id) {
    for (MenuItemList::iterator i = items.begin(); i != items.end(); i++) {
        if (i->getId() == id) {
            i->setClosesMenu(true);
            break;
        }
    }
}

/**
 * Returns the menu item that is currently selected/highlighted
 */ 
Menu::MenuItemList::iterator Menu::getCurrent() {    
    return selected;
}

/**
 * Sets the current menu item to the one indicated by the iterator
 */ 
void Menu::setCurrent(MenuItemList::iterator i) {
    selected = i;
    highlight(&(*selected));

    MenuEvent event(this, MenuEvent::SELECT);
    setChanged();
    notifyObservers(event);
}

void Menu::setCurrent(MenuId id) {
    setCurrent(getById(id));
}

/**
 * Displays the menu
 */
void Menu::show() {
    for (current = items.begin(); current != items.end(); current++) {
        MenuItem *mi = &(*current);

        if (mi->isVisible()) {
            if (mi->isSelected())
                screenTextAt(mi->x-1, mi->y, "\010%s", mi->text.c_str());
            else screenTextAt(mi->x, mi->y, mi->text.c_str());

            if (mi->isHighlighted()) {
                screenEnableCursor();
                screenSetCursorPos(mi->x - 2, mi->y);
                screenShowCursor();
            }
        }
    }
}

/**
 * Checks the menu to ensure that there is at least 1 visible
 * item in the list.  Returns true if there is at least 1 visible
 * item, false if nothing is visible. 
 */
bool Menu::isVisible() {
    bool visible = false;

    for (current = items.begin(); current != items.end(); current++) {
        if (current->isVisible())
            visible = true;
    }

    return visible;
}

/**
 * Sets the selected iterator to the next visible menu item and highlights it
 */
void Menu::next() {
    MenuItemList::iterator i = selected;
    if (isVisible()) {
        if (++i == items.end())
            i = items.begin();
        while (!i->isVisible()) {
            if (++i == items.end())
                i = items.begin();            
        }
    }

    setCurrent(i);
}

/**
 * Sets the selected iterator to the previous visible menu item and highlights it
 */
void Menu::prev() {
    MenuItemList::iterator i = selected;
    if (isVisible()) {        
        if (i == items.begin())
            i = items.end();
        i--;
        while (!i->isVisible()) {
            if (i == items.begin())
                i = items.end();
            i--;
        }
    }

    setCurrent(i);
}

/**
 * Highlights a single menu item, un-highlighting any others
 */ 
void Menu::highlight(MenuItem *item) {
    for (current = items.begin(); current != items.end(); current++)
        current->highlighted = false;
    if (item)
        item->highlighted = true;
}

/**
 * Returns an iterator pointing to the first menu item
 */ 
Menu::MenuItemList::iterator Menu::begin() {
    return items.begin();
}

/**
 * Returns an iterator pointing just past the last menu item
 */ 
Menu::MenuItemList::iterator Menu::end() {
    return items.end();
}

/**
 * Returns an iterator pointing to the first visible menu item
 */ 
Menu::MenuItemList::iterator Menu::begin_visible() {
    if (!isVisible())
        return items.end();

    current = items.begin();
    while (!current->isVisible() && current != items.end())
        current++;

    return current;
}

/**
 * 'Resets' the menu.  This does the following:
 *      - un-highlights all menu items
 *      - highlights the first menu item
 *      - selects the first visible menu item
 */
void Menu::reset(bool highlightFirst) {
    closed = false;

    /* get the first visible menu item */    
    selected = begin_visible();
    
    /* un-highlight and deselect each menu item */
    for (current = items.begin(); current != items.end(); current++) {
        current->highlighted = false;
        current->selected = false;
    }

    /* highlight the first visible menu item */
    if (highlightFirst)
        highlight(&(*selected));  

    MenuEvent event(this, MenuEvent::RESET);
    setChanged();
    notifyObservers(event);
}

/**
 * Returns an iterator pointing to the item associated with the given 'id'
 */ 
Menu::MenuItemList::iterator Menu::getById(MenuId id) {
    if (id == -1)
        return getCurrent();
    
    for (current = items.begin(); current != items.end(); current++) {
        if (current->id == id)
            return current;
    }   
    return items.end();
}

/**
 * Returns the menu item associated with the given 'id'
 */
MenuItem *Menu::getItemById(MenuId id) {
    current = getById(id);
    if (current != items.end())
        return &(*current);
    return NULL;
}

/**
 * Activates the menu item given by 'id', using 'action' to
 * activate it.  If the menu item cannot be activated using
 * 'action', then it is not activated.  This also un-highlights
 * the menu item given by 'menu' and highlights the new menu
 * item that was found for 'id'.
 */
void Menu::activateItem(MenuId id, MenuEvent::Type action) {
    MenuItem *mi;
    
    /* find the given menu item by id */
    if (id >= 0)
        mi = getItemById(id);
    /* or use the current item */
    else mi = &(*getCurrent());
       
    if (!mi)
        errorFatal("Error: Unable to find menu item with id '%d'", id);

    /* make sure the action given will activate the menu item */
    if (mi->getClosesMenu())
        setClosed(true);

    MenuEvent event(this, (MenuEvent::Type) action, mi);
    setChanged();
    notifyObservers(event);
}

/**
 * Activates a menu item by it's shortcut key.  True is returned if a
 * menu item get activated, false otherwise.
 */
bool Menu::activateItemByShortcut(int key, MenuEvent::Type action) {
    for (MenuItemList::iterator i = items.begin(); i != items.end(); i++) {
        const set<int> &shortcuts = i->getShortcutKeys();
        if (shortcuts.find(key) != shortcuts.end()) {
            activateItem(i->getId(), action);
            return true;
        }
    }
    return false;
}

/**
 * Returns true if the menu has been closed.
 */
bool Menu::getClosed() const {
    return closed;
}

/**
 * Update whether the menu has been closed.
 */
void Menu::setClosed(bool closed) {
    this->closed = closed;
}

MenuController::MenuController(Menu *menu) {
    this->menu = menu;
}

bool MenuController::keyPressed(int key) {
    bool handled = true;

    switch(key) {
    case U4_UP:
        menu->prev();        
        break;
    case U4_DOWN:
        menu->next();
        break;
    case U4_LEFT:
    case U4_RIGHT:
    case U4_ENTER:
        {
            MenuEvent::Type action = MenuEvent::ACTIVATE;

            if (key == U4_LEFT)
                action = MenuEvent::DECREMENT;
            else if (key == U4_RIGHT)
                action = MenuEvent::INCREMENT;
            menu->activateItem(-1, action);
        }
        break;
    default:
        handled = menu->activateItemByShortcut(key, MenuEvent::ACTIVATE);
    }    

    screenHideCursor();
    menu->show();
    screenUpdateCursor();
    screenRedrawScreen();

    if (menu->getClosed())
        doneWaiting();

    return handled;
}

