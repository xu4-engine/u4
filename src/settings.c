/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "settings.h"
#include "error.h"

Settings *settings = NULL;

char *settingsFilename() {
    char *fname, *home;

    home = getenv("HOME");
    if (home && home[0]) {
        fname = (char *) malloc(strlen(home) + strlen("/.xu4rc") + 1);
        strcpy(fname, home);
        strcat(fname, "/.xu4rc");
    } else
        fname = strdup(".xu4rc");

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
    settings->filter = SCL_AdvanceMAME;
    settings->vol = 1;
    settings->germanKbd = 0;

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
            "germanKbd=%d\n",
            settings->scale,
            settings->fullscreen,
            settingsFilterToString(settings->filter),
            settings->vol,
            settings->germanKbd);

    fclose(settingsFile);
}

/**
 * Convert a filter enum into a readable string.
 */
const char *settingsFilterToString(FilterType filter) {
    static const char * const filterNames[] = {
        "point", "2xBi", "2xSaI", "AdvanceMAME"
    };

    if (filter >= SCL_MAX)
        assert(0);              /* shouldn't happen */

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
