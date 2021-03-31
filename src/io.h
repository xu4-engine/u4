/*
 * $Id$
 */

#ifndef IO_H
#define IO_H

#include <stdio.h>

/*
 * These are endian-independant routines for reading and writing
 * 4-byte (int), 2-byte (short), and 1-byte (char) values to and from
 * the ultima 4 data files.  If sizeof(int) != 4, all bets are off.
 */

int writeInt(unsigned int i, FILE *f);
int writeShort(unsigned short s, FILE *f);
int writeChar(unsigned char c, FILE *f);
int readInt(unsigned int *i, FILE *f);
int readShort(unsigned short *s, FILE *f);
int readChar(unsigned char *c, FILE *f);

#endif
