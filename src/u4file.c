/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "u4file.h"

/* the possible paths where u4 for DOS can be installed */
const char *paths[] = {
    "./",
    "./ultima4/",
    "/usr/lib/u4/ultima4/",
    "/usr/local/lib/u4/ultima4/"
};


/**
 * Open a data file from the Ultima 4 for DOS installation.  This
 * function checks the various places where it can be installed, and
 * maps the filenames to uppercase if necessary.  The files are always
 * opened for reading only.
 */
FILE *u4fopen(const char *fname) {
    FILE *f = NULL;
    unsigned int i, j;
    char pathname[128];

    for (i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
        snprintf(pathname, sizeof(pathname), "%s%s", paths[i], fname);

        if ((f = fopen(pathname, "rb")) != NULL)
            break;

        for (j = strlen(paths[i]); pathname[j] != '\0'; j++) {
            if (islower(pathname[j]))
                pathname[j] = toupper(pathname[j]);
        }
        
        if ((f = fopen(pathname, "rb")) != NULL)
            break;
    }

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
    char **strs = malloc(nstrings * sizeof(char *));
    if (!strs)
        return NULL;

    assert(offset < u4flength(f));

    if (offset != -1)
        fseek(f, offset, SEEK_SET);
    for (i = 0; i < nstrings; i++) {
        for (j = 0; j < sizeof(buffer) - 1; j++) {
            buffer[j] = fgetc(f);
            if (buffer[j] == '\0' &&
                j > 0 && buffer[j - 1] != 8) /* needed to handle weird characters in lb's abyss response */
                break;
        }
        buffer[j] = '\0';
        strs[i] = strdup(buffer);
    }

    return strs;
}
