/*
 * $Id$
 */

#ifndef MENU_H
#define MENU_H

#include "list.h"

typedef enum {
    ACTIVATE_NORMAL     = 0x1,
    ACTIVATE_INCREMENT  = 0x2,
    ACTIVATE_DECREMENT  = 0x4
} ActivateAction;

#define ACTIVATE_ANY    (0xFFFF)

typedef ListNode* Menu;
typedef void (*ActivateMenuItem)(Menu, ActivateAction);

typedef struct _MenuItem {
    unsigned char id;
    short x, y;
    char *text;
    unsigned char isHighlighted;
    unsigned char isVisible;
    unsigned char isSelected;
    ActivateMenuItem activateMenuItem;
    ActivateAction activateOn;    
} MenuItem;

Menu menuAddItem(Menu menu, unsigned char id, char *text, short x, short y, ActivateMenuItem activate, ActivateAction activateOn);
MenuItem *menuGetItem(Menu current);
int menuShow(Menu menu);
int menuCheckVisible(Menu menu);
Menu menuGetNextItem(Menu current);
Menu menuGetPreviousItem(Menu current);
Menu menuGetRoot(Menu current);
Menu menuHighlightNew(Menu oldItem, Menu newItem);
void menuItemSetVisible(Menu item, int visible);
void menuDelete(Menu menu);
Menu menuReset(Menu current);
Menu menuGetItemById(Menu menu, unsigned char id);
int menuCompareFindItemById(void *val1, void *val2);
Menu menuActivateItem(Menu menu, short id, ActivateAction action);

#endif