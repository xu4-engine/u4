/*
 * $Id$
 */

#ifndef IO_H
#define IO_H

int writeInt(unsigned int i, FILE *f);
int writeShort(unsigned short s, FILE *f);
int writeChar(unsigned char c, FILE *f);
int readInt(unsigned int *i, FILE *f);
int readShort(unsigned short *s, FILE *f);
int readChar(unsigned char *c, FILE *f);

#endif
