/*
 * $Id$
 */

#ifndef U4FILE_H
#define U4FILE_H

#include <stdio.h>

typedef FILE U4FILE;

U4FILE *u4fopen(const char *fname);
void u4fclose(U4FILE *f);
int u4fseek(U4FILE *f, long offset, int whence);
size_t u4fread(void *ptr, size_t size, size_t nmemb, U4FILE *f);
int u4fgetc(U4FILE *f);
int u4fputc(int c, U4FILE *f);
long u4flength(U4FILE *f);
char **u4read_stringtable(U4FILE *f, long offset, int nstrings);
char *u4find_music(const char *fname);
char *u4find_conf(const char *fname);

#endif
