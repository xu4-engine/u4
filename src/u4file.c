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
    char *pathname = NULL;

    for (i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
        pathname = malloc(strlen(paths[i]) + strlen(fname) + 1);
        if (!pathname)
            continue;
        strcpy(pathname, paths[i]);
        strcat(pathname, fname);

        if ((f = fopen(pathname, "rb")) != NULL)
            break;

        for (j = strlen(paths[i]); pathname[j] != '\0'; j++) {
            if (islower(pathname[j]))
                pathname[j] = toupper(pathname[j]);
        }
        
        if ((f = fopen(pathname, "rb")) != NULL)
            break;

        free(pathname);
        pathname = NULL;
    }

    if (pathname)
        free(pathname);

    return f;
}

/**
 * Closes a data file from the Ultima 4 for DOS installation.
 */
void u4fclose(FILE *f) {
    fclose(f);
}

long u4flength(FILE *f) {
    long curr, len;

    curr = ftell(f);
    fseek(f, 0L, SEEK_END);
    len = ftell(f);
    fseek(f, curr, SEEK_SET);

    return len;
}
