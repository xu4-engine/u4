/*
 * $Id$
 */

#ifndef INTRO_H
#define INTRO_H

#ifdef __cplusplus
extern "C" {
#endif

#define FREE_MENUS      1
#define DONT_FREE_MENUS 0

int introInit(void);
void introDelete(int freeMenus);
unsigned char *introGetSigData();
void introUpdateScreen(void);
void introTimer(void *data);
int introBaseMenuKeyHandler(int key, void *data);

/**
 * Key handlers
 */ 
bool introKeyHandler(int key, void *data);

#ifdef __cplusplus
}
#endif

#endif
