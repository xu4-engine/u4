
#include <cstdlib>
#include <cstring>
#include <sys/param.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cerrno>
#include "error.h"

void osxInit(char *binpath)
{
    char parentdir[MAXPATHLEN];
    char *c, *dirname, *home;
    struct stat sb;
    mode_t mask;
    int result;
    
    // Change working directory to the location of the XU4 resources
    strncpy ( parentdir, binpath, sizeof(parentdir) );
    c = (char*) parentdir;
    while (*c != '\0') c++;    /* go to end */
    while (*c != '/') c--;     /* back up to parent */
    do c--; while (*c != '/'); /* One more level up */
    *c++ = '\0';               /* cut off remainder */
    if ((chdir(parentdir) != 0) || (chdir("Resources") != 0)) {
        errorFatal("Cannot access application bundle's Resources directory.");
    }
    
    // On the first run, the directory for user files must be created.
    // This code checks if it has been created, and creates it if not.
    home = getenv("HOME");
    if (home && home[0]) {
        dirname = (char *) malloc(strlen(home) +
        strlen(MACOSX_USER_FILES_PATH) + 1);
        strcpy(dirname, home);
        strcat(dirname, MACOSX_USER_FILES_PATH);

        /* Check if directory exists */
        result = stat(dirname, &sb);
        if ((result != 0) && (errno == ENOENT)) {
            /* Doesn't exist. Create it */
            mask = umask(0); /* Get current umask */
            umask(mask); /* Restore old umask */
            mkdir(dirname, S_IRWXU | mask);
        }

        free(dirname);
    }
}

