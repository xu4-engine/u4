/*
 * $Id$
 */

#ifndef TTYPE_H
#define TTYPE_H

int iswalkable(unsigned char tile);
int isslow(unsigned char tile);
int isvslow(unsigned char tile);
int isdoor(unsigned char tile);
int islockeddoor(unsigned char tile);
int tileCanTalkOver(unsigned char tile);
int tileIsAnimated(unsigned char tile);

#endif
