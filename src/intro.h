/*
 * $Id$
 */

#ifndef INTRO_H
#define INTRO_H

int introInit(void);
void introDelete(void);
int introKeyHandler(int key, void *data);
void introUpdateScreen(void);
void introTimer(void *data);

#endif
