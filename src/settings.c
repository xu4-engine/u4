/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "settings.h"

#include "debug.h"
#include "error.h"

Settings *settings = NULL;

#if defined(_WIN32) || defined(__CYGWIN__)
#define SETTINGS_BASE_FILENAME "xu4.cfg"
#else
#if defined(MACOSX)
#define SETTINGS_BASE_FILENAME "xu4rc"
#else
#define SETTINGS_BASE_FILENAME ".xu4rc"
#endif
#endif

char *settingsFilename() {
    char *fname, *home;

    home = getenv("HOME");
    if (home && home[0]) {
#if defined(MACOSX)
        fname = (char *) malloc(strlen(home) + 
strlen(MACOSX_USER_FILES_PATH) +
strlen(SETTINGS_BASE_FILENAME) + 2);
        strcpy(fname, home);
        strcat(fname, MACOSX_USER_FILES_PATH);
#else
        fname = (char *) malloc(strlen(home) + strlen(SETTINGS_BASE_FILENAME) + 2);
        strcpy(fname, home);
#endif
        strcat(fname, "/");
        strcat(fname, SETTINGS_BASE_FILENAME);
    } else
        fname = strdup(SETTINGS_BASE_FILENAME);

    return fname;
}

/**
 * Read settings in from the settings file.
 */
void settingsRead() {
    char buffer[256];
    char *settingsFname;
    FILE *settingsFile;
    
    settings = (Settings *) malloc(sizeof(Settings));

    /* default settings */
    settings->scale = 2;
    settings->fullscreen = 0;
    settings->filter = SCL_Scale2x;
    settings->vol = 1;
    settings->germanKbd = 0;
    settings->shortcutCommands = 0;
    settings->keydelay = 500;
    settings->keyinterval = 30;
    settings->filterMoveMessages = 0;
    settings->battleSpeed = 5;
    settings->minorEnhancements = 1;

    settingsFname = settingsFilename();
    settingsFile = fopen(settingsFname, "r");
    free(settingsFname);
    if (!settingsFile)
        return;

    while(fgets(buffer, sizeof(buffer), settingsFile) != NULL) {
        while (isspace(buffer[strlen(buffer) - 1]))
            buffer[strlen(buffer) - 1] = '\0';

        if (strstr(buffer, "scale=") == buffer)
            settings->scale = (unsigned int) strtoul(buffer + strlen("scale="), NULL, 0);
        else if (strstr(buffer, "fullscreen=") == buffer)
            settings->fullscreen = (int) strtoul(buffer + strlen("fullscreen="), NULL, 0);
        else if (strstr(buffer, "filter=") == buffer) {
            settings->filter = settingsStringToFilter(buffer + strlen("filter="));
            if (settings->filter == SCL_MAX) {
                errorWarning("invalid filter name in settings file: resetting to point scaler");
                settings->filter = SCL_POINT;
            }
        }
        else if (strstr(buffer, "vol=") == buffer)
            settings->vol = (int) strtoul(buffer + strlen("vol="), NULL, 0);
        else if (strstr(buffer, "germanKbd=") == buffer)
            settings->germanKbd = (int) strtoul(buffer + strlen("germanKbd="), NULL, 0);
        else if (strstr(buffer, "shortcutCommands=") == buffer)
            settings->shortcutCommands = (int) strtoul(buffer + strlen("shortcutCommands="), NULL, 0);
        else if (strstr(buffer, "keydelay=") == buffer)
            settings->keydelay = (int) strtoul(buffer + strlen("keydelay="), NULL, 0);
        else if (strstr(buffer, "keyinterval=") == buffer)
            settings->keyinterval = (int) strtoul(buffer + strlen("keyinterval="), NULL, 0);
        else if (strstr(buffer, "filterMoveMessages=") == buffer)
            settings->filterMoveMessages = (int) strtoul(buffer + strlen("filterMoveMessages="), NULL, 0);
        else if (strstr(buffer, "battlespeed=") == buffer)
            settings->battleSpeed = (int) strtoul(buffer + strlen("battlespeed="), NULL, 0);
        /* FIXME: this is just to avoid an error for those who have not written
           a new xu4.cfg file since attackspeed was removed.  Remove it after a reasonable
           amount of time */
        else if (strstr(buffer, "attackspeed=") == buffer)
            ;
        else if (strstr(buffer, "minorEnhancements=") == buffer)
            settings->minorEnhancements = (int) strtoul(buffer + strlen("minorEnhancements="), NULL, 0);
        else
            errorWarning("invalid line in settings file %s", buffer);
    }

    fclose(settingsFile);
}

/**
 * Read the settings out into a human readable file.
 */
void settingsWrite() {
    char *settingsFname;
    FILE *settingsFile;
    
    settingsFname = settingsFilename();
    settingsFile = fopen(settingsFname, "w");
    free(settingsFname);
    if (!settingsFile) {
        errorWarning("can't write settings file");
        return;
    }   

    fprintf(settingsFile, 
            "scale=%d\n"
            "fullscreen=%d\n"
            "filter=%s\n"
            "vol=%d\n"
            "germanKbd=%d\n"
            "shortcutCommands=%d\n"
            "keydelay=%d\n"
            "keyinterval=%d\n"
            "filterMoveMessages=%d\n"
            "battlespeed=%d\n"
            "minorEnhancements=%d\n",
            settings->scale,
            settings->fullscreen,
            settingsFilterToString(settings->filter),
            settings->vol,
            settings->germanKbd,
            settings->shortcutCommands,
            settings->keydelay,
            settings->keyinterval,
            settings->filterMoveMessages,
            settings->battleSpeed,
            settings->minorEnhancements);

    fclose(settingsFile);
}

/**
 * Convert a filter enum into a readable string.
 */
const char *settingsFilterToString(FilterType filter) {
    static const char * const filterNames[] = {
        "point", "2xBi", "2xSaI", "Scale2x"
    };

    ASSERT(filter < SCL_MAX, "invalid filter value %d\n", filter);

    return filterNames[filter];
}

/**
 * Convert a string to a filter enum.  Returns SCL_MAX if the string
 * doesn't match a filter.
 */
FilterType settingsStringToFilter(const char *str) {
    int f;
    FilterType result = SCL_MAX;
    for (f = (FilterType) 0; f < SCL_MAX; f++) {
        if (strcmp(str, settingsFilterToString((FilterType) f)) == 0) {
            result = (FilterType) f;
            break;
        }
    }

    return result;
}
