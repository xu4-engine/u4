/*
 * $Id$
 */

#ifndef SCREEN_H
#define SCREEN_H

void screenInit();
void screenDrawBorders();
void screenShowTile(int tile, int x, int y);
void screenShowChar(int chr, int x, int y);
void screenMessage(char *fmt, ...);
void screenUpdate();

extern SDL_Surface *screen;
extern SDL_Surface *tiles, *charset;

#endif
