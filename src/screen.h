/*
 * $Id$
 */

#ifndef SCREEN_H
#define SCREEN_H

void screenInit(int scale);
void screenDrawBorders();
void screenShowTile(int tile, int x, int y);
void screenShowChar(int chr, int x, int y);
void screenMessage(char *fmt, ...);
void screenUpdate();
void screenForceRedraw();

#endif
