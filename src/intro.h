/*
 * $Id$
 */

#ifndef INTRO_H
#define INTRO_H

int introInit(void);
void introDelete(void);
unsigned char *introGetSigData();
int introKeyHandler(int key, void *data);
void introUpdateScreen(void);
void introTimer(void *data);
int introBaseMenuKeyHandler(int key, void *data);

#endif
