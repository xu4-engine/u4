/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include "menu.h"

#include "error.h"
#include "screen.h"

/**
 * MenuItem class
 */
MenuItem::MenuItem(class Menu *m, MenuId i, string t, short xpos, short ypos, ActivateMenuItem af, ActivateAction ao) :
    menu(m),
    id(i),
    x(xpos),
    y(ypos),
    text(t),    
    highlighted(false),
    selected(false),
    visible(true),
    activateMenuItem(af),
    activateOn(ao)
{    
}

/**
 * Notifies the containing menu that the menu item has changed
 */
void MenuItem::notifyOfChange(string arg) {
    if (menu) {
        menu->setChanged();
        menu->notifyObservers(arg);
    }
}

MenuId MenuItem::getId() const                      { return id; }
short MenuItem::getX() const                        { return x; }
short MenuItem::getY() const                        { return y; }
string MenuItem::getText() const                    { return text; }
bool MenuItem::isHighlighted() const                { return highlighted; }
bool MenuItem::isSelected() const                   { return selected; }
bool MenuItem::isVisible() const                    { return visible; }
ActivateMenuItem MenuItem::getActivateFunc() const  { return activateMenuItem; }
ActivateAction MenuItem::getActivateAction() const  { return activateOn; }

void MenuItem::setId(MenuId i) {
    if (id != i) {
        id = i;
        notifyOfChange("MenuItem::setId");
    }
}

void MenuItem::setX(int xpos) {
    if (xpos != x) {
        x = xpos;
        notifyOfChange("MenuItem::setX");
    }
}

void MenuItem::setY(int ypos) {
    if (ypos != y) {
        y = ypos;
        notifyOfChange("MenuItem::setY");
    }
}

void MenuItem::setText(string t) {
    text = t;
    notifyOfChange("MenuItem::setText");
}

void MenuItem::setHighlighted(bool h) {
    if (h != highlighted) {
        highlighted = h;
        notifyOfChange("MenuItem::setHighlighted");
    }
}

void MenuItem::setSelected(bool s) {
    if (s != selected) {
        selected = s;
        notifyOfChange("MenuItem::setSelected");
    }
}

void MenuItem::setVisible(bool v) {
    if (v != visible) {
        visible = v;
        notifyOfChange("MenuItem::setVisible");
    }
}

void MenuItem::setActivateFunc(ActivateMenuItem ami) {
    if (ami != activateMenuItem) {
        activateMenuItem = ami;
        notifyOfChange("MenuItem::setActivateFunc");
    }
}

void MenuItem::setActivateAction(ActivateAction aa) {
    if (aa != activateOn) {
        activateOn = aa;
        notifyOfChange("MenuItem::setActivateAction");
    }
}

/**
 * Adds an item to the menu list and returns the menu
 */
void Menu::add(MenuId id, string text, short x, short y, ActivateMenuItem activate, ActivateAction activateOn) {
    MenuItem menuItem(this, id, text, x, y, activate, activateOn);

    items.push_back(menuItem);
    setChanged();
    notifyObservers("Menu::add");
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
    setChanged();
    notifyObservers("Menu::setCurrent");
}

void Menu::setCurrent(MenuId id) {
    selected = getById(id);
    setChanged();
    notifyObservers("Menu::setCurrent");
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
    if (isVisible()) {        
        if (++selected == items.end())
            selected = items.begin();
        while (!selected->isVisible()) {
            if (++selected == items.end())
                selected = items.begin();            
        }
    }

    highlight(&(*selected));
    setChanged();
    notifyObservers("Menu::next");
}

/**
 * Sets the selected iterator to the previous visible menu item and highlights it
 */
void Menu::prev() {
    if (isVisible()) {        
        if (selected == items.begin())
            selected = items.end();
        
        selected--;
        while (!selected->isVisible()) {
            if (selected == items.begin())
                selected = items.end();
            selected--;
        }
    }

    highlight(&(*selected));
    setChanged();
    notifyObservers("Menu::prev");
}

/**
 * Highlights a single menu item, un-highlighting any others
 */ 
void Menu::highlight(MenuItem *item) {
    for (current = items.begin(); current != items.end(); current++)
        current->highlighted = false;
    if (item)
        item->highlighted = true;
    
    setChanged();
    notifyObservers("Menu::highlight");
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

    setChanged();
    notifyObservers("Menu::reset");
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
void Menu::activateItem(MenuId id, ActivateAction action) {    
    MenuItem *mi;
    
    /* find the given menu item by id */
    if (id >= 0)
        mi = getItemById(id);
    /* or use the current item */
    else mi = &(*getCurrent());
       
    if (!mi)
        errorFatal("Error: Unable to find menu item with id '%d'", id);

    /* make sure the action given will activate the menu item */
    if ((mi->activateOn & action) && mi->activateMenuItem)
        (*mi->activateMenuItem)(mi, action);    

    setChanged();
    notifyObservers("Menu::activateItem");
}
