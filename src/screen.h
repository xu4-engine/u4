/*
 * $Id$
 */

#ifndef SCREEN_H
#define SCREEN_H

void screenInit(int scale);
void screenDrawBorders();
void screenShowTile(int tile, int x, int y);
void screenShowChar(int chr, int x, int y);
void screenTextAt(int x, int y, char *fmt, ...);
void screenMessage(char *fmt, ...);
void screenScrollMessageArea();
void screenUpdate();
void screenForceRedraw();

#endif
