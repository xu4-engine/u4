/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "u4file.h"

#include "debug.h"

extern int verbose;

/* the possible paths where u4 for DOS can be installed */
static const char * const paths[] = {
    "./",
    "./ultima4/",
    "/usr/lib/u4/ultima4/",
    "/usr/local/lib/u4/ultima4/"
};

/* the possible paths where the u4 music files can be installed */
static const char * const music_paths[] = {
    "./",
    "./ultima4/",
    "../mid/",
    "/usr/lib/u4/music/",
    "/usr/local/lib/u4/music/"
};

/* the possible paths where the u4 config files can be installed */
static const char * const conf_paths[] = {
    "./",
    "../conf/",
    "/usr/lib/u4/",
    "/usr/local/lib/u4/"
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
U4FILE *u4fopen(const char *fname) {
    U4FILE *u4f;
    char *pathname = NULL;

    u4f = (U4FILE *) malloc(sizeof(U4FILE));
    if (!u4f)
        return NULL;

    if (getenv("U4ZIPFILE")) {
        unzFile f;

        f = unzOpen(getenv("U4ZIPFILE"));
        if (!f)
            return NULL;

        pathname = malloc(strlen("ultima4/") + strlen(fname) + 1);
        strcpy(pathname, "ultima4/");
        strcat(pathname, fname);

        unzLocateFile(f, pathname, 2);
        unzOpenCurrentFile(f);

        u4f->type = ZIP_FILE;
        u4f->zfile = f;
    }
    else {
        FILE *f = NULL;
        unsigned int i;
        char *fname_copy;

        fname_copy = strdup(fname);

        pathname = u4find_path(fname_copy, paths, sizeof(paths) / sizeof(paths[0]));
        if (!pathname) {
            if (islower(fname_copy[0])) {
                fname_copy[0] = toupper(fname_copy[0]);
                pathname = u4find_path(fname_copy, paths, sizeof(paths) / sizeof(paths[0]));
            }

            if (!pathname) {
                for (i = 0; fname_copy[i] != '\0'; i++) {
                    if (islower(fname_copy[i]))
                        fname_copy[i] = toupper(fname_copy[i]);
                }
                pathname = u4find_path(fname_copy, paths, sizeof(paths) / sizeof(paths[0]));
            }
        }

        if (pathname)
            f = fopen(pathname, "rb");

        if (verbose && f != NULL)
            printf("%s successfully opened\n", pathname);

        free(fname_copy);

        u4f->type = STDIO_FILE;
        u4f->file = f;
    }

    if (pathname)
        free(pathname);

    return u4f;
}

/**
 * Closes a data file from the Ultima 4 for DOS installation.
 */
void u4fclose(U4FILE *f) {
    switch (f->type) {
    case STDIO_FILE:
        fclose(f->file);
        break;
    case ZIP_FILE:
        unzClose(f->zfile);
        break;
    }
    free(f);
}

int u4fseek(U4FILE *f, long offset, int whence) {
    char *buf;
    long pos;

    switch (f->type) {
    case STDIO_FILE:
        return fseek(f->file, offset, whence);
    case ZIP_FILE:
        ASSERT(whence != SEEK_END, "seeking with whence == SEEK_END not allowed with zipfiles");
        pos = unztell(f->zfile);
        if (whence == SEEK_CUR)
            offset = pos + offset;
        if (offset == pos)
            return 0;
        if (offset < pos) {
            unzCloseCurrentFile(f->zfile);
            unzOpenCurrentFile(f->zfile);
            pos = 0;
        }
        ASSERT(offset - pos > 0, "error in u4fseek (zipfile)");
        buf = malloc(offset - pos);
        unzReadCurrentFile(f->zfile, buf, offset - pos);
        free(buf);
        return 0;
    }

    return 0;
}

size_t u4fread(void *ptr, size_t size, size_t nmemb, U4FILE *f) {
    size_t retval;

    switch(f->type) {
    case STDIO_FILE:
        retval = fread(ptr, size, nmemb, f->file);
        break;
    case ZIP_FILE:
        retval = unzReadCurrentFile(f->zfile, ptr, size * nmemb);
        if (retval > 0)
            retval = retval / size;
        break;
    }
    return retval;
}

int u4fgetc(U4FILE *f) {
    int retval;
    char c;

    switch(f->type) {
    case STDIO_FILE:
        retval = fgetc(f->file);
        break;
    case ZIP_FILE:
        if (unzReadCurrentFile(f->zfile, &c, 1) > 0)
            retval = c;
        else
            retval = EOF;
        break;
    }

    return retval;
}

int u4fgetshort(U4FILE *f) {
    return u4fgetc(f) | (u4fgetc(f) << 8);
}

int u4fputc(int c, U4FILE *f) {
    ASSERT(f->type == STDIO_FILE, "zipfiles must be read-only!");
    return fputc(c, f->file);
}

/**
 * Returns the length in bytes of a file.
 */
long u4flength(U4FILE *f) {
    long curr, len;
    unz_file_info fileinfo;

    switch (f->type) {
    case STDIO_FILE:
        curr = ftell(f->file);
        fseek(f->file, 0L, SEEK_END);
        len = ftell(f->file);
        fseek(f->file, curr, SEEK_SET);
        break;
    case ZIP_FILE:
        unzGetCurrentFileInfo(f->zfile, &fileinfo,
                              NULL, 0,
                              NULL, 0,
                              NULL, 0);
        len = fileinfo.uncompressed_size;
    }

    return len;
}

/**
 * Read a series of zero terminated strings from a file.  The strings
 * are read from the given offset, or the current file position if
 * offset is -1.
 */
char **u4read_stringtable(U4FILE *f, long offset, int nstrings) {
    char buffer[384];
    int i, j;
    char **strs = (char **) malloc(nstrings * sizeof(char *));
    if (!strs)
        return NULL;

    ASSERT(offset < u4flength(f), "offset begins beyond end of file");

    if (offset != -1)
        u4fseek(f, offset, SEEK_SET);
    for (i = 0; i < nstrings; i++) {
        for (j = 0; j < sizeof(buffer) - 1; j++) {
            buffer[j] = u4fgetc(f);
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

char *u4find_path(const char *fname, const char * const *pathent, int npathents) {
    FILE *f = NULL;
    unsigned int i;
    char pathname[128];

    for (i = 0; i < npathents; i++) {
        snprintf(pathname, sizeof(pathname), "%s%s", pathent[i], fname);

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

char *u4find_music(const char *fname) {
    return u4find_path(fname, music_paths, sizeof(music_paths) / sizeof(music_paths[0]));
}

char *u4find_conf(const char *fname) {
    return u4find_path(fname, conf_paths, sizeof(conf_paths) / sizeof(conf_paths[0]));
}
