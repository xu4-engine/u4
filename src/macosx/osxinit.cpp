
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
#include <CoreServices/CoreServices.h>

char macOSX_AppBundle_Resource_Path[MAXPATHLEN];

void osxInit(char *binpath)
{
    char *dirname, *home;
    struct stat sb;
    mode_t mask;
    int result;
    
    home = getenv("HOME");

    std::list<std::string> mac_roots;
    mac_roots.push_back(std::string("."));
    mac_roots.push_back(std::string(home));

    std::list<std::string> mac_app_support;
    mac_app_support.push_back(std::string("/Library/Application Support/xu4"));
    mac_app_support.push_back(std::string(MACOSX_USER_FILES_PATH));

    // Figure out and store the path to the application bundle's
    // 'Resources' directory, so that it can be searched in
    // u4file.cpp:u4find_path()
    CFURLRef resourcePath = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
    CFURLGetFileSystemRepresentation(resourcePath, true, reinterpret_cast<UInt8 *>(macOSX_AppBundle_Resource_Path),
                                     MAXPATHLEN);
    CFRelease(resourcePath);
    
    u4Path.rootResourcePaths.push_back(macOSX_AppBundle_Resource_Path);

    // On the first run, the directory for user files must be created.
    // This code checks if it has been created, and creates it if not.
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
        u4Path.rootResourcePaths.push_back(std::string(dirname));
        free(dirname);
    }

    for (std::list<std::string>::iterator root = mac_roots.begin();
    		root != mac_roots.end();
    		++root)
    	for (std::list<std::string>::iterator app_supp = mac_app_support.begin();
    		app_supp != mac_app_support.end();
    		++app_supp)
    	{
    		u4Path.rootResourcePaths.push_back((*root).append(*app_supp));
    	}

}

