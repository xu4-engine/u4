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
    ActivateMenuItem activeMenuItem;
} MenuItem;

Menu menuAddItem(Menu menu, unsigned char id, char *text, short x, short y, ActivateMenuItem activate);
int menuShow(Menu menu);
Menu menuGetNextItem(Menu current);
Menu menuGetPreviousItem(Menu current);
Menu menuGetRoot(Menu current);
Menu menuHighlightNew(Menu oldItem, Menu newItem);
void menuDelete(Menu menu);
Menu menuReset(Menu current);

#endif