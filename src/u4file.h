/*
 * $Id$
 */

#ifndef U4FILE_H
#define U4FILE_H

#include <stdio.h>

FILE *u4fopen(const char *fname);
void u4fclose(FILE *f);
long u4flength(FILE *f);
char **u4read_stringtable(FILE *f, long offset, int nstrings);
char *u4find_music(const char *fname);
char *u4find_conf(const char *fname);

#endif
