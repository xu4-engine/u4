/*
 * $Id$
 */

#ifndef U4FILE_H
#define U4FILE_H

#include <stdio.h>

FILE *u4fopen(const char *fname);
void u4fclose(FILE *f);
long u4flength(FILE *f);

#endif
