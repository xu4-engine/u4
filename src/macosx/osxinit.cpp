
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/param.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cerrno>
#include "error.h"
#include "u4file.h"

char macOSX_AppBundle_Resource_Path[MAXPATHLEN];

void osxInit(char *binpath)
{
    char parentdir[MAXPATHLEN];
    char *c, *dirname, *home;
    struct stat sb;
    mode_t mask;
    int result;
    
    // Figure out and store the path to the application bundle's
    // 'Resources' directory, so that it can be searched in
    // u4file.cpp:u4find_path()
    strncpy ( parentdir, binpath, sizeof(parentdir) );
    c = (char*) parentdir;
    while (*c != '\0') c++;    /* go to end */
    while (*c != '/') c--;     /* back up to parent */
    do c--; while (*c != '/'); /* One more level up */
    *c++ = '\0';               /* cut off remainder */
    snprintf(macOSX_AppBundle_Resource_Path, MAXPATHLEN, "%s/Resources/",
        parentdir);
    
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
      /* Include the application bundle's 'Resources' directory in Mac OS X */
        free(dirname);
    }
    u4Path.rootResourcePaths.push_back(std::string(macOSX_AppBundle_Resource_Path));
    u4Path.rootResourcePaths.push_back(std::string(MACOSX_USER_FILES_PATH));
}

