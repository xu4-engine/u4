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
    extern int eventTimerGranularity;
    
    settings = (Settings *) malloc(sizeof(Settings));

    /* default settings */
    settings->scale                 = DEFAULT_SCALE;
    settings->fullscreen            = DEFAULT_FULLSCREEN;
    settings->filter                = DEFAULT_FILTER;
    settings->videoType             = DEFAULT_VIDEO_TYPE;
    settings->screenShakes          = DEFAULT_SCREEN_SHAKES;
    settings->vol                   = DEFAULT_VOLUME;
    settings->volumeFades           = DEFAULT_VOLUME_FADES;
    settings->germanKbd             = DEFAULT_GERMAN_KEYBOARD;
    settings->shortcutCommands      = DEFAULT_SHORTCUT_COMMANDS;
    settings->keydelay              = DEFAULT_KEY_DELAY;
    settings->keyinterval           = DEFAULT_KEY_INTERVAL;
    settings->filterMoveMessages    = DEFAULT_FILTER_MOVE_MESSAGES;
    settings->battleSpeed           = DEFAULT_BATTLE_SPEED;
    settings->enhancements          = DEFAULT_ENHANCEMENTS;    
    settings->gameCyclesPerSecond   = DEFAULT_CYCLES_PER_SECOND;
    settings->debug                 = DEFAULT_DEBUG;
    settings->battleDiff            = DEFAULT_BATTLE_DIFFICULTY;
    settings->validateXml           = DEFAULT_VALIDATE_XML;
    settings->spellEffectSpeed      = DEFAULT_SPELL_EFFECT_SPEED;
    settings->campTime              = DEFAULT_CAMP_TIME;
    settings->innTime               = DEFAULT_INN_TIME;
    settings->shrineTime            = DEFAULT_SHRINE_TIME;
    settings->shakeInterval         = DEFAULT_SHAKE_INTERVAL;

    /* all specific minor enhancements default to "on", any major enhancements default to "off" */
    settings->enhancementsOptions.activePlayer     = 1;
    settings->enhancementsOptions.u5spellMixing    = 1;
    settings->enhancementsOptions.u5shrines        = 1;
    settings->enhancementsOptions.slimeDivides     = 1;
    settings->enhancementsOptions.c64chestTraps    = 1;    
    settings->enhancementsOptions.smartEnterKey    = 1;
    settings->enhancementsOptions.u5combat         = 0;

    settings->innAlwaysCombat = 0;
    settings->campingAlwaysCombat = 0;

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
        else if (strstr(buffer, "video=") == buffer) {
            settings->videoType = settingsStringToVideoType(buffer + strlen("video="));
            if (settings->videoType == VIDEO_MAX) {
                errorWarning("invalid video type name in settings file: resetting to VGA");
                settings->videoType = VIDEO_VGA;
            }
        }
        else if (strstr(buffer, "screenShakes=") == buffer)
            settings->screenShakes = (int) strtoul(buffer + strlen("screenShakes="), NULL, 0);
        else if (strstr(buffer, "vol=") == buffer)
            settings->vol = (int) strtoul(buffer + strlen("vol="), NULL, 0);
        else if (strstr(buffer, "volumeFades=") == buffer)
            settings->volumeFades = (int) strtoul(buffer + strlen("volumeFades="), NULL, 0);        
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
        else if (strstr(buffer, "enhancements=") == buffer)
            settings->enhancements = (int) strtoul(buffer + strlen("enhancements="), NULL, 0);        
        else if (strstr(buffer, "gameCyclesPerSecond=") == buffer)
            settings->gameCyclesPerSecond = (int) strtoul(buffer + strlen("gameCyclesPerSecond="), NULL, 0);
        else if (strstr(buffer, "debug=") == buffer)
            settings->debug = (int) strtoul(buffer + strlen("debug="), NULL, 0);
        else if (strstr(buffer, "battleDiff=") == buffer) {
            settings->battleDiff = settingsStringToBattleDiff(buffer + strlen("battleDiff="));
            if (settings->battleDiff == DIFF_MAX) {
                errorWarning("invalid difficulty name in settings file: resetting to normal");
                settings->battleDiff = DIFF_NORMAL;
            }
        }
        else if (strstr(buffer, "validateXml=") == buffer)
            settings->validateXml = (int) strtoul(buffer + strlen("validateXml="), NULL, 0);
        else if (strstr(buffer, "spellEffectSpeed=") == buffer)
            settings->spellEffectSpeed = (int) strtoul(buffer + strlen("spellEffectSpeed="), NULL, 0);
        else if (strstr(buffer, "campTime=") == buffer)
            settings->campTime = (int) strtoul(buffer + strlen("campTime="), NULL, 0);
        else if (strstr(buffer, "innTime=") == buffer)
            settings->innTime = (int) strtoul(buffer + strlen("innTime="), NULL, 0);
        else if (strstr(buffer, "shrineTime=") == buffer)
            settings->shrineTime = (int) strtoul(buffer + strlen("shrineTime="), NULL, 0);
        else if (strstr(buffer, "shakeInterval=") == buffer)
            settings->shakeInterval = (int) strtoul(buffer + strlen("shakeInterval="), NULL, 0);
        
        /* minor enhancement options */
        else if (strstr(buffer, "activePlayer=") == buffer)
            settings->enhancementsOptions.activePlayer = (int) strtoul(buffer + strlen("activePlayer="), NULL, 0);
        else if (strstr(buffer, "u5spellMixing=") == buffer)
            settings->enhancementsOptions.u5spellMixing = (int) strtoul(buffer + strlen("u5spellMixing="), NULL, 0);
        else if (strstr(buffer, "u5shrines=") == buffer)
            settings->enhancementsOptions.u5shrines = (int) strtoul(buffer + strlen("u5shrines="), NULL, 0);
        else if (strstr(buffer, "slimeDivides=") == buffer)
            settings->enhancementsOptions.slimeDivides = (int) strtoul(buffer + strlen("slimeDivides="), NULL, 0);
        else if (strstr(buffer, "c64chestTraps=") == buffer)
            settings->enhancementsOptions.c64chestTraps = (int) strtoul(buffer + strlen("c64chestTraps="), NULL, 0);                
        else if (strstr(buffer, "smartEnterKey=") == buffer)
            settings->enhancementsOptions.smartEnterKey = (int) strtoul(buffer + strlen("smartEnterKey="), NULL, 0);
        
        /* major enhancement options */
        else if (strstr(buffer, "u5combat=") == buffer)
            settings->enhancementsOptions.u5combat = (int) strtoul(buffer + strlen("u5combat="), NULL, 0);
        else if (strstr(buffer, "innAlwaysCombat=") == buffer)
            settings->innAlwaysCombat = (int) strtoul(buffer + strlen("innAlwaysCombat="), NULL, 0);
        else if (strstr(buffer, "campingAlwaysCombat=") == buffer)
            settings->campingAlwaysCombat = (int) strtoul(buffer + strlen("campingAlwaysCombat="), NULL, 0);    

        /**
         * FIXME: this is just to avoid an error for those who have not written
         * a new xu4.cfg file since these items were removed.  Remove them after a reasonable
         * amount of time 
         *
         * remove:  attackspeed, minorEnhancements, majorEnhancements        
         */
        
        else if (strstr(buffer, "attackspeed=") == buffer);
        else if (strstr(buffer, "minorEnhancements=") == buffer)
            settings->enhancements = (int)strtoul(buffer + strlen("minorEnhancements="), NULL, 0);
        else if (strstr(buffer, "majorEnhancements=") == buffer);
        
        /***/

        else
            errorWarning("invalid line in settings file %s", buffer);
    }

    fclose(settingsFile);

    eventTimerGranularity = (1000 / settings->gameCyclesPerSecond);
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
            "video=%s\n"
            "screenShakes=%d\n"
            "vol=%d\n"
            "volumeFades=%d\n"
            "germanKbd=%d\n"
            "shortcutCommands=%d\n"
            "keydelay=%d\n"
            "keyinterval=%d\n"
            "filterMoveMessages=%d\n"
            "battlespeed=%d\n"
            "enhancements=%d\n"            
            "gameCyclesPerSecond=%d\n"
            "debug=%d\n"
            "battleDiff=%s\n"
            "validateXml=%d\n"
            "spellEffectSpeed=%d\n"
            "campTime=%d\n"
            "innTime=%d\n"
            "shrineTime=%d\n"
            "shakeInterval=%d\n"
            "activePlayer=%d\n"
            "u5spellMixing=%d\n"
            "u5shrines=%d\n"
            "slimeDivides=%d\n"
            "c64chestTraps=%d\n"            
            "smartEnterKey=%d\n"
            "u5combat=%d\n"
            "innAlwaysCombat=%d\n"
            "campingAlwaysCombat=%d\n",
            settings->scale,
            settings->fullscreen,
            settingsFilterToString(settings->filter),
            settingsVideoTypeToString(settings->videoType),
            settings->screenShakes,
            settings->vol,
            settings->volumeFades,
            settings->germanKbd,
            settings->shortcutCommands,
            settings->keydelay,
            settings->keyinterval,
            settings->filterMoveMessages,
            settings->battleSpeed,
            settings->enhancements,            
            settings->gameCyclesPerSecond,
            settings->debug,
            settingsBattleDiffToString(settings->battleDiff),            
            settings->validateXml,
            settings->spellEffectSpeed,
            settings->campTime,
            settings->innTime,
            settings->shrineTime,
            settings->shakeInterval,
            settings->enhancementsOptions.activePlayer,
            settings->enhancementsOptions.u5spellMixing,
            settings->enhancementsOptions.u5shrines,
            settings->enhancementsOptions.slimeDivides,
            settings->enhancementsOptions.c64chestTraps,            
            settings->enhancementsOptions.smartEnterKey,
            settings->enhancementsOptions.u5combat,
            settings->innAlwaysCombat,
            settings->campingAlwaysCombat);

    fclose(settingsFile);
}

/**
 * Convert a filter enum into a readable string.
 */
const char *settingsFilterToString(FilterType filter) {
    static const char * const filterNames[] = {
        "SCL_MIN", "point", "2xBi", "2xSaI", "Scale2x"
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
    for (f = (FilterType) (SCL_MIN+1); f < SCL_MAX; f++) {
        if (strcmp(str, settingsFilterToString((FilterType) f)) == 0) {
            result = (FilterType) f;
            break;
        }
    }

    return result;
}

/**
 * Convert a video type enum into a readable string.
 */
const char *settingsVideoTypeToString(VideoType type) {
    static const char * const videoNames[] = {
        "VIDEO_MIN", "VGA", "EGA"   //, "CGA"
    };

    ASSERT(type < VIDEO_MAX, "invalid video type %d\n", type);

    return videoNames[type];
}

/**
 * Convert a string into a video type enum
 */
VideoType settingsStringToVideoType(const char *str) {
    int v;
    VideoType result = VIDEO_MAX;
    for (v = (VideoType) (VIDEO_MIN+1); v < VIDEO_MAX; v++) {
        if (strcmp(str, settingsVideoTypeToString((VideoType) v)) == 0) {
            result = (VideoType) v;
            break;
        }
    }

    return result;
}

/**
 * Covert a battle difficulty enum into a readable string.
 */
const char *settingsBattleDiffToString(BattleDifficulty diff) {
    static const char * const diffNames[] = {
        "DIFF_MIN", "Normal", "Hard", "Expert"
    };

    ASSERT(diff < DIFF_MAX, "invalid difficulty %d\n", diff);

    return diffNames[diff];    
}

/**
 * Convert a string into a battle difficulty enum
 */
BattleDifficulty settingsStringToBattleDiff(const char *str) {
    int d;
    BattleDifficulty result = DIFF_MAX;
    for (d = (BattleDifficulty) (DIFF_MIN+1); d < DIFF_MAX; d++) {
        if (strcmp(str, settingsBattleDiffToString((BattleDifficulty) d)) == 0) {
            result = (BattleDifficulty) d;
            break;
        }
    }

    return result;
}
