/*
 * $Id$
 */

#ifndef U4FILE_H
#define U4FILE_H

#include <stdio.h>
#include "unzip.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    COMP_NONE,
    COMP_RLE,
    COMP_LZW,
    COMP_MAX
} CompressionType;

typedef struct {
    enum {
        STDIO_FILE,
        ZIP_FILE
    } type;
    union {
        FILE *file;
        unzFile zfile;
    };
} U4FILE;

int u4isUpgradeInstalled(void);
U4FILE *u4fopen(const char *fname);
U4FILE *u4fopen_stdio(const char *fname);
U4FILE *u4fopen_zip(const char *fname, const char *zipfile, const char *zippath, int translate);
void u4fclose(U4FILE *f);
int u4fseek(U4FILE *f, long offset, int whence);
size_t u4fread(void *ptr, size_t size, size_t nmemb, U4FILE *f);
int u4fgetc(U4FILE *f);
int u4fgetshort(U4FILE *f);
int u4fputc(int c, U4FILE *f);
long u4flength(U4FILE *f);
char **u4read_stringtable(U4FILE *f, long offset, int nstrings);
char *u4find_path(const char *fname, const char * const *pathent, unsigned int npathents);
char *u4find_music(const char *fname);
char *u4find_sound(const char *fname);
char *u4find_conf(const char *fname);
char *u4find_graphics(const char *fname);
const char *u4upgrade_translate_filename(const char *fname);
CompressionType u4GetCompTypeByStr(const char *comp);

extern int u4zipExists;
extern int u4upgradeZipExists;
extern int u4upgradeExists;
extern int u4upgradeInstalled;

#ifdef __cplusplus
}
#endif

#endif
