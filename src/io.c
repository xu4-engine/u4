/*
 * $Id$
 */

#include <stdio.h>

#include "io.h"

int writeInt(unsigned int i, FILE *f) {
    if (fputc(i & 0xff, f) == EOF ||
        fputc((i >> 8) & 0xff, f) == EOF ||
        fputc((i >> 16) & 0xff, f) == EOF ||
        fputc((i >> 24) & 0xff, f) == EOF)
        return 0;
    return 1;
}

int writeShort(unsigned short s, FILE *f) {
    if (fputc(s & 0xff, f) == EOF ||
        fputc((s >> 8) & 0xff, f) == EOF)
        return 0;
    return 1;
}

int writeChar(unsigned char c, FILE *f) {
    if (fputc(c, f) == EOF)
        return 0;
    return 1;
}

int readInt(unsigned int *i, FILE *f) {
    *i = fgetc(f);
    *i |= (fgetc(f) << 8);
    *i |= (fgetc(f) << 16);
    *i |= (fgetc(f) << 24);
    
    return 1;
}

int readShort(unsigned short *s, FILE *f) {
    *s = fgetc(f);
    *s |= (fgetc(f) << 8);

    return 1;
}

int readChar(unsigned char *c, FILE *f) {
    *c = fgetc(f);

    return 1;
}

