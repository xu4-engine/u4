/*
 * $Id$
 */

#include <stdlib.h>
#include "menu.h"

#include "list.h"
#include "screen.h"

Menu menuAddItem(Menu menu, unsigned char id, char *text, short x, short y, ActivateMenuItem activate) {
    MenuItem *menuItem = (MenuItem *)malloc(sizeof(MenuItem));

    menuItem->id = id;
    menuItem->text = text;
    menuItem->x = x;
    menuItem->y = y;
    menuItem->isHighlighted = 0;
    menuItem->activateMenuItem = activate;
    
    return listAppend(menu, menuItem);
} 

int menuShow(Menu menu) {
    Menu current;

    for (current = menu; current; current = current->next) {
        MenuItem *menuItem = (MenuItem *)current->data;
        
        screenTextAt(menuItem->x, menuItem->y, menuItem->text);
        if (menuItem->isHighlighted) {
            screenEnableCursor();
            screenSetCursorPos(menuItem->x - 2, menuItem->y);
            screenShowCursor();
        }
    }
    return 1;
}

Menu menuGetNextItem(Menu current) {
    if (current->next != NULL)
        return current->next;
    else return current;
}

Menu menuGetPreviousItem(Menu current) {
    if (current->prev != NULL)
        return current->prev;
    else return current;
}

Menu menuGetRoot(Menu current) {
    Menu m = current;
    while (m->prev)
        m = m->prev;
    return m;
}

Menu menuHighlightNew(Menu oldItem, Menu newItem) {
    if (oldItem) ((MenuItem *)(oldItem->data))->isHighlighted = 0;
    if (newItem) ((MenuItem *)(newItem->data))->isHighlighted = 1;
    return newItem;
}

void menuDelete(Menu menu) {
    Menu current;

    for (current = menu; current; current = current->next) {
        if (current->data)
            free(current->data);
    }

    listDelete(menu);
}

Menu menuReset(Menu current) {
    return menuHighlightNew(current, menuGetRoot(current));
}

Menu menuGetItemById(Menu menu, unsigned char id) {        
    return listFind(menu, (void *)id, &menuCompareFindItemById);
}

int menuCompareFindItemById(void *val1, void *val2) {
    MenuItem *menuItem = (MenuItem *)val1;
    unsigned char id = (unsigned char)val2;

    if (menuItem->id > id)
        return 1;
    else if (menuItem->id < id)
        return -1;
    return 0;
}