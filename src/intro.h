/*
 * $Id$
 */

#ifndef INTRO_H
#define INTRO_H

#define FREE_MENUS      1
#define DONT_FREE_MENUS 0

int introInit(void);
void introDelete(int freeMenus);
unsigned char *introGetSigData();
int introKeyHandler(int key, void *data);
void introUpdateScreen(void);
void introTimer(void *data);
int introBaseMenuKeyHandler(int key, void *data);

#endif
