/*
 * $Id$
 */

#ifndef MENU_H
#define MENU_H

#include "list.h"

typedef ListNode* Menu;
typedef void (*ActivateMenuItem)(Menu);

typedef struct _MenuItem {
    unsigned char id;
    short x, y;
    char *text;
    unsigned char isHighlighted;
    ActivateMenuItem activateMenuItem;
} MenuItem;

Menu menuAddItem(Menu menu, unsigned char id, char *text, short x, short y, ActivateMenuItem activate);
int menuShow(Menu menu);
Menu menuGetNextItem(Menu current);
Menu menuGetPreviousItem(Menu current);
Menu menuGetRoot(Menu current);
Menu menuHighlightNew(Menu oldItem, Menu newItem);
void menuDelete(Menu menu);
Menu menuReset(Menu current);
Menu menuGetItemById(Menu menu, unsigned char id);
int menuCompareFindItemById(void *val1, void *val2);

#endif