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

/* these are for figuring out where to find files */
int u4zipExists = 0;
int u4upgradeZipExists = 0;
int u4upgradeExists = 0;
int u4upgradeInstalled = 0;

/* the possible paths where u4 for DOS can be installed */
static const char * const paths[] = {
    "./",
    "./ultima4/",
    "/usr/lib/u4/ultima4/",
    "/usr/local/lib/u4/ultima4/"
};

/* the possible paths where the u4 zipfiles can be installed */
static const char * const zip_paths[] = {
    "./",
    "/usr/lib/u4/",
    "/usr/local/lib/u4/"
};

/* the possible paths where the u4 music files can be installed */
static const char * const music_paths[] = {
    "./",
    "./ultima4/",
    "./mid/",
    "../mid/",
    "/usr/lib/u4/music/",
    "/usr/local/lib/u4/music/"
};

/* the possible paths where the u4 sound files can be installed */
static const char * const sound_paths[] = {
    "./",
    "./ultima4/",
    "./sound/",
    "../sound/",
    "/usr/lib/u4/sound/",
    "/usr/local/lib/u4/sound/"
};

/* the possible paths where the u4 config files can be installed */
static const char * const conf_paths[] = {
    "./",
    "./conf/",
    "../conf/",
    "/usr/lib/u4/",
    "/usr/local/lib/u4/"
};

/* the possible paths where the u4 graphics files can be installed */
static const char * const graphics_paths[] = {
    "./",
    "./ultima4/",
    "./graphics/",
    "../graphics/",
    "/usr/lib/u4/graphics/",
    "/usr/local/lib/u4/graphics/"
};


/**
 * Returns true if the upgrade is not only present, but is installed
 * (switch.bat or setup.bat has been run)
 */
int u4isUpgradeInstalled(void) {
    U4FILE *u4f = NULL;
    long int filelength;

    /* FIXME: Is there a better way to determine this? */
    u4f = u4fopen("ega.drv");
    if (!u4f)
        return 0;

    filelength = u4flength(u4f);
    u4fclose(u4f);

    /* see if (ega.drv > 5k).  If so, the upgrade is installed */
    if (filelength > (5 * 1024))
        return 1;
    else 
        return 0;

    return 0;
}

/**
 * Open a data file from the Ultima 4 for DOS installation.  This
 * function checks the various places where it can be installed, and
 * maps the filenames to uppercase if necessary.  The files are always
 * opened for reading only.
 *
 * First, it looks in the zipfiles.  Next, it tries FILENAME, Filename
 * and filename in up to four paths, meaning up to twelve or more
 * opens per file.  Seems to be ok for performance, but could be
 * getting excessive.  The presence of the zipfiles should probably be
 * cached.
 */
U4FILE *u4fopen(const char *fname) {
    U4FILE *u4f = NULL;
    char *fname_copy, *pathname;
    unsigned int i;

    if (verbose)
        printf("looking for %s\n", fname);

    /**
     * search for file within ultima4.zip or u4upgrad.zip
     */
    pathname = u4find_path("ultima4.zip", zip_paths, sizeof(zip_paths) / sizeof(zip_paths[0]));
    if (pathname) {
        char *upg_pathname;
        
        /* original u4 zip is present */
        u4zipExists = 1;
        
        upg_pathname = u4find_path("u4upgrad.zip", zip_paths, sizeof(zip_paths) / sizeof(zip_paths[0]));
        /* both zip files are present */
        if (upg_pathname)
            u4upgradeZipExists = 1;        

        /* look for the file in ultima4.zip */
        u4f = u4fopen_zip(fname, pathname, "ultima4/", 0);
        if (u4f) {
            free(pathname);
            free(upg_pathname);
            return u4f; /* file was found, return it! */
        }

        /* look for the file in u4upgrad.zip */
        if (u4upgradeZipExists) {
            u4f = u4fopen_zip(fname, upg_pathname, "", 1);
            free(upg_pathname);
            if (u4f) {
                free(pathname); 
                return u4f; /* file was found, return it! */ 
            }
        }
    }

    /*
     * file not in a zipfile; check if it has been unzipped
     */
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
    free(fname_copy);

    if (pathname) {
        u4f = u4fopen_stdio(pathname);
        if (verbose && u4f != NULL)
            printf("%s successfully opened\n", pathname);
        free(pathname);
    }

    return u4f;
}

/**
 * Opens a file with the standard C stdio facilities and wrap it in a
 * U4FILE struct.
 */
U4FILE *u4fopen_stdio(const char *fname) {
    U4FILE *u4f;
    FILE *f;

    f = fopen(fname, "rb");
    if (!f)
        return NULL;

    u4f = (U4FILE *) malloc(sizeof(U4FILE));
    if (!u4f)
        return NULL;

    u4f->type = STDIO_FILE;
    u4f->file = f;
    
    return u4f;
}

/**
 * Opens a file from within a zip archive.  fname is the filename
 * being searched for, zipfile is the name of the zip archive, zippath
 * is a path prefix prepended to each filename.  If translate is
 * non-zero, some special case translation is done to the filenames to
 * match up with the names in u4upgrad.zip.
 */
U4FILE *u4fopen_zip(const char *fname, const char *zipfile, const char *zippath, int translate) {
    U4FILE *u4f;
    unzFile f;
    char *pathname;

    f = unzOpen(zipfile);
    if (!f)
        return NULL;

    if (translate)
        fname = u4upgrade_translate_filename(fname);
    pathname = malloc(strlen(zippath) + strlen(fname) + 1);
    strcpy(pathname, zippath);
    strcat(pathname, fname);

    if (unzLocateFile(f, pathname, 2) == UNZ_END_OF_LIST_OF_FILE) {
        free(pathname);
        unzClose(f);
        return NULL;
    }
    unzOpenCurrentFile(f);

    u4f = (U4FILE *) malloc(sizeof(U4FILE));
    if (!u4f) {
        free(pathname);
        unzClose(f);
        return NULL;
    }

    u4f->type = ZIP_FILE;
    u4f->zfile = f;

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
    unsigned char c;

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
    int byteLow = u4fgetc(f);
    return byteLow | (u4fgetc(f) << 8);
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
            
            if ((buffer[j] == '\0') 
                /* These extra checks catch the null in the middle of
                 * LB's response about the Abyss 
                 */
                && (!((buffer[j-1] == 8) && (buffer[j-2] == 10))))                
                break;            
        }
        buffer[j] = '\0';
        strs[i] = strdup(buffer);
    }

    return strs;
}

char *u4find_path(const char *fname, const char * const *pathent, unsigned int npathents) {
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

    if (verbose) {
        if (f != NULL)
            printf("%s successfully found\n", pathname);
        else 
            printf("%s not found\n", fname);
    }

    if (f) {
        fclose(f);
        return strdup(pathname);
    } else
        return NULL;
}

char *u4find_music(const char *fname) {
    return u4find_path(fname, music_paths, sizeof(music_paths) / sizeof(music_paths[0]));
}

char *u4find_sound(const char *fname) {
    return u4find_path(fname, sound_paths, sizeof(sound_paths) / sizeof(sound_paths[0]));
}

char *u4find_conf(const char *fname) {
    return u4find_path(fname, conf_paths, sizeof(conf_paths) / sizeof(conf_paths[0]));
}

char *u4find_graphics(const char *fname) {
    return u4find_path(fname, graphics_paths, sizeof(graphics_paths) / sizeof(graphics_paths[0]));
}

const char *u4upgrade_translate_filename(const char *fname) {
    int i;
    const static struct {
        char *name;
        char *translation;
    } translations[] = {
        { "compassn.ega", "compassn.old" },
        { "courage.ega", "courage.old" },
        { "courage.ega", "courage.old" },
        { "cove.tlk", "cove.old" },
        { "ega.drv", "ega.old" }, /* not actually used */
        { "honesty.ega", "honesty.old" },
        { "honor.ega", "honor.old" },
        { "humility.ega", "humility.old" },
        { "key7.ega", "key7.old" },
        { "lcb.tlk", "lcb.old" },
        { "love.ega", "love.old" },
        { "love.ega", "love.old" },
        { "minoc.tlk", "minoc.old" },
        { "rune_0.ega", "rune_0.old" },
        { "rune_1.ega", "rune_1.old" },
        { "rune_2.ega", "rune_2.old" },
        { "rune_3.ega", "rune_3.old" },
        { "rune_4.ega", "rune_4.old" },
        { "rune_5.ega", "rune_5.old" },
        { "sacrific.ega", "sacrific.old" },
        { "skara.tlk", "skara.old" },
        { "spirit.ega", "spirit.old" },
        { "start.ega", "start.old" },
        { "stoncrcl.ega", "stoncrcl.old" },
        { "truth.ega", "truth.old" },
        { "ultima.com", "ultima.old" }, /* not actually used */
        { "valor.ega", "valor.old" },
        { "yew.tlk", "yew.old" }
    };

    for (i = 0; i < sizeof(translations) / sizeof(translations[0]); i++) {
        if (strcasecmp(translations[i].name, fname) == 0)
            return translations[i].translation;
    }
    return fname;
}

CompressionType u4GetCompTypeByStr(const char *comp) {
    const char *types[] = { "none", "rle", "lzw" };
    int i;

    for (i = COMP_NONE; i < COMP_MAX; i++) {
        if (strcasecmp(types[i], comp) == 0)
            return (CompressionType)i;
    }

    return COMP_NONE;
}
