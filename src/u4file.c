/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "u4file.h"

extern int verbose;

/* the possible paths where u4 for DOS can be installed */
static const char * const paths[] = {
    "./",
    "./ultima4/",
    "/usr/lib/u4/ultima4/",
    "/usr/local/lib/u4/ultima4/"
};

/* the possible paths where u4 for the music files */
static const char * const music_paths[] = {
    "./",
    "./ultima4/",
    "../mid/",
    "/usr/lib/u4/music/",
    "/usr/local/lib/u4/music/"
};


/**
 * Open a data file from the Ultima 4 for DOS installation.  This
 * function checks the various places where it can be installed, and
 * maps the filenames to uppercase if necessary.  The files are always
 * opened for reading only.
 *
 * Currently, it tries FILENAME, Filename and filename in up to four
 * paths, meaning up to twelve opens per file.  Seems to be ok for
 * performance, but could be getting excessive.
 */
FILE *u4fopen(const char *fname) {
    FILE *f = NULL;
    unsigned int i, j;
    char pathname[128];

    for (i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
        snprintf(pathname, sizeof(pathname), "%s%s", paths[i], fname);

        if (verbose)
            printf("trying to open %s\n", pathname);

        if ((f = fopen(pathname, "rb")) != NULL)
            break;

        if (islower(pathname[strlen(paths[i])]))
            pathname[strlen(paths[i])] = toupper(pathname[strlen(paths[i])]);
        
        if (verbose)
            printf("trying to open %s\n", pathname);

        if ((f = fopen(pathname, "rb")) != NULL)
            break;

        for (j = strlen(paths[i]); pathname[j] != '\0'; j++) {
            if (islower(pathname[j]))
                pathname[j] = toupper(pathname[j]);
        }
        
        if (verbose)
            printf("trying to open %s\n", pathname);

        if ((f = fopen(pathname, "rb")) != NULL)
            break;
    }

    if (verbose && f != NULL)
        printf("%s successfully opened\n", pathname);

    return f;
}

/**
 * Closes a data file from the Ultima 4 for DOS installation.
 */
void u4fclose(FILE *f) {
    fclose(f);
}

/**
 * Returns the length in bytes of a file.
 */
long u4flength(FILE *f) {
    long curr, len;

    curr = ftell(f);
    fseek(f, 0L, SEEK_END);
    len = ftell(f);
    fseek(f, curr, SEEK_SET);

    return len;
}

/**
 * Read a series of zero terminated strings from a file.  The strings
 * are read from the given offset, or the current file position if
 * offset is -1.
 */
char **u4read_stringtable(FILE *f, long offset, int nstrings) {
    char buffer[384];
    int i, j;
    char **strs = (char **) malloc(nstrings * sizeof(char *));
    if (!strs)
        return NULL;

    assert(offset < u4flength(f));

    if (offset != -1)
        fseek(f, offset, SEEK_SET);
    for (i = 0; i < nstrings; i++) {
        for (j = 0; j < sizeof(buffer) - 1; j++) {
            buffer[j] = fgetc(f);
            if (buffer[j] == '\0' &&
                (j < 2 || !(buffer[j - 2] == 10 && buffer[j - 1] == 8))) /* needed to handle weird characters in lb's abyss response */
                break;
        }
        while(j > 0 && !(isprint(buffer[j - 1]) || isspace(buffer[j - 1])))
            j--;
        buffer[j] = '\0';
        strs[i] = strdup(buffer);
    }

    return strs;
}

char *u4find_music(const char *fname) {
    FILE *f = NULL;
    unsigned int i;
    char pathname[128];

    for (i = 0; i < sizeof(music_paths) / sizeof(music_paths[0]); i++) {
        snprintf(pathname, sizeof(pathname), "%s%s", music_paths[i], fname);

        if (verbose)
            printf("trying to open %s\n", pathname);

        if ((f = fopen(pathname, "rb")) != NULL)
            break;
    }

    if (verbose && f != NULL)
        printf("%s successfully found\n", pathname);

    if (f) {
        fclose(f);
        return strdup(pathname);
    } else
        return NULL;
}
