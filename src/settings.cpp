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
#include "event.h"
#include "utils.h"

Settings settings;

/**
 * Initialize static members
 */ 
const static string filterStrings[]     = {"SCL_MIN", "point", "2xBi", "2xSaI", "Scale2x", ""};
const static string battleDiffStrings[] = {"DIFF_MIN", "Normal", "Hard", "Expert", ""};

Settings::FilterTranslator      Settings::filters       = FilterTranslator(filterStrings);
Settings::BattleDiffTranslator  Settings::battleDiffs   = BattleDiffTranslator(battleDiffStrings);            

#if defined(_WIN32) || defined(__CYGWIN__)
#define SETTINGS_BASE_FILENAME "xu4.cfg"
#else
#if defined(MACOSX)
#define SETTINGS_BASE_FILENAME "xu4rc"
#else
#define SETTINGS_BASE_FILENAME ".xu4rc"
#endif
#endif

/**
 * Settings class implementation
 */ 

// Constructors
Settings::Settings() {    
    char *home;

    home = getenv("HOME");
    if (home && home[0]) {
        filename = home;
#if defined(MACOSX)
        filename += MACOSX_USER_FILES_PATH;
#endif
        filename += "/";
    }

    filename += SETTINGS_BASE_FILENAME;    
}

/**
 * Methods
 */ 

/**
 * Read settings in from the settings file.
 */
bool Settings::read() {
    char buffer[256];    
    FILE *settingsFile;
    extern int eventTimerGranularity;   

    /* default settings */
    scale                 = DEFAULT_SCALE;
    fullscreen            = DEFAULT_FULLSCREEN;
    filter                = DEFAULT_FILTER;
    videoType             = DEFAULT_VIDEO_TYPE;
    gemLayout             = DEFAULT_GEM_LAYOUT;
    screenShakes          = DEFAULT_SCREEN_SHAKES;
    musicVol              = DEFAULT_MUSIC_VOLUME;
    soundVol              = DEFAULT_SOUND_VOLUME;
    volumeFades           = DEFAULT_VOLUME_FADES;
    germanKbd             = DEFAULT_GERMAN_KEYBOARD;
    shortcutCommands      = DEFAULT_SHORTCUT_COMMANDS;
    keydelay              = DEFAULT_KEY_DELAY;
    keyinterval           = DEFAULT_KEY_INTERVAL;
    filterMoveMessages    = DEFAULT_FILTER_MOVE_MESSAGES;
    battleSpeed           = DEFAULT_BATTLE_SPEED;
    enhancements          = DEFAULT_ENHANCEMENTS;    
    gameCyclesPerSecond   = DEFAULT_CYCLES_PER_SECOND;
    debug                 = DEFAULT_DEBUG;
    battleDiff            = DEFAULT_BATTLE_DIFFICULTY;
    validateXml           = DEFAULT_VALIDATE_XML;
    spellEffectSpeed      = DEFAULT_SPELL_EFFECT_SPEED;
    campTime              = DEFAULT_CAMP_TIME;
    innTime               = DEFAULT_INN_TIME;
    shrineTime            = DEFAULT_SHRINE_TIME;
    shakeInterval         = DEFAULT_SHAKE_INTERVAL;

    /* all specific minor enhancements default to "on", any major enhancements default to "off" */
    enhancementsOptions.activePlayer     = 1;
    enhancementsOptions.u5spellMixing    = 1;
    enhancementsOptions.u5shrines        = 1;
    enhancementsOptions.slimeDivides     = 1;
    enhancementsOptions.c64chestTraps    = 1;    
    enhancementsOptions.smartEnterKey    = 1;
    enhancementsOptions.u5combat         = 0;

    innAlwaysCombat = 0;
    campingAlwaysCombat = 0;

    /* mouse defaults to on */
    mouseOptions.enabled = 1;
    
    settingsFile = fopen(filename.c_str(), "rt");    
    if (!settingsFile)
        return false;

    while(fgets(buffer, sizeof(buffer), settingsFile) != NULL) {
        while (isspace(buffer[strlen(buffer) - 1]))
            buffer[strlen(buffer) - 1] = '\0';

        if (strstr(buffer, "scale=") == buffer)
            scale = (unsigned int) strtoul(buffer + strlen("scale="), NULL, 0);
        else if (strstr(buffer, "fullscreen=") == buffer)
            fullscreen = (int) strtoul(buffer + strlen("fullscreen="), NULL, 0);
        else if (strstr(buffer, "filter=") == buffer) {
            filter = (FilterType)Settings::filters.getType(buffer + strlen("filter="));
            if (filter == -1) {
                errorWarning("invalid filter name in settings file: resetting to point scaler");
                filter = SCL_POINT;
            }
        }
        else if (strstr(buffer, "video=") == buffer)
            videoType = buffer + strlen("video=");
        else if (strstr(buffer, "gemLayout=") == buffer)
            gemLayout = buffer + strlen("gemLayout=");
        else if (strstr(buffer, "screenShakes=") == buffer)
            screenShakes = (int) strtoul(buffer + strlen("screenShakes="), NULL, 0);        
        else if (strstr(buffer, "musicVol=") == buffer)
            musicVol = (int) strtoul(buffer + strlen("musicVol="), NULL, 0);
        else if (strstr(buffer, "soundVol=") == buffer)
            soundVol = (int) strtoul(buffer + strlen("soundVol="), NULL, 0);
        else if (strstr(buffer, "volumeFades=") == buffer)
            volumeFades = (int) strtoul(buffer + strlen("volumeFades="), NULL, 0);        
        else if (strstr(buffer, "germanKbd=") == buffer)
            germanKbd = (int) strtoul(buffer + strlen("germanKbd="), NULL, 0);
        else if (strstr(buffer, "shortcutCommands=") == buffer)
            shortcutCommands = (int) strtoul(buffer + strlen("shortcutCommands="), NULL, 0);
        else if (strstr(buffer, "keydelay=") == buffer)
            keydelay = (int) strtoul(buffer + strlen("keydelay="), NULL, 0);
        else if (strstr(buffer, "keyinterval=") == buffer)
            keyinterval = (int) strtoul(buffer + strlen("keyinterval="), NULL, 0);
        else if (strstr(buffer, "filterMoveMessages=") == buffer)
            filterMoveMessages = (int) strtoul(buffer + strlen("filterMoveMessages="), NULL, 0);
        else if (strstr(buffer, "battlespeed=") == buffer)
            battleSpeed = (int) strtoul(buffer + strlen("battlespeed="), NULL, 0);
        else if (strstr(buffer, "enhancements=") == buffer)
            enhancements = (int) strtoul(buffer + strlen("enhancements="), NULL, 0);        
        else if (strstr(buffer, "gameCyclesPerSecond=") == buffer)
            gameCyclesPerSecond = (int) strtoul(buffer + strlen("gameCyclesPerSecond="), NULL, 0);
        else if (strstr(buffer, "debug=") == buffer)
            debug = (int) strtoul(buffer + strlen("debug="), NULL, 0);
        else if (strstr(buffer, "battleDiff=") == buffer) {
            battleDiff = (BattleDifficulty)Settings::battleDiffs.getType(buffer + strlen("battleDiff="));
            if (battleDiff == -1) {
                errorWarning("invalid difficulty name in settings file: resetting to normal");
                battleDiff = DIFF_NORMAL;
            }
        }
        else if (strstr(buffer, "validateXml=") == buffer)
            validateXml = (int) strtoul(buffer + strlen("validateXml="), NULL, 0);
        else if (strstr(buffer, "spellEffectSpeed=") == buffer)
            spellEffectSpeed = (int) strtoul(buffer + strlen("spellEffectSpeed="), NULL, 0);
        else if (strstr(buffer, "campTime=") == buffer)
            campTime = (int) strtoul(buffer + strlen("campTime="), NULL, 0);
        else if (strstr(buffer, "innTime=") == buffer)
            innTime = (int) strtoul(buffer + strlen("innTime="), NULL, 0);
        else if (strstr(buffer, "shrineTime=") == buffer)
            shrineTime = (int) strtoul(buffer + strlen("shrineTime="), NULL, 0);
        else if (strstr(buffer, "shakeInterval=") == buffer)
            shakeInterval = (int) strtoul(buffer + strlen("shakeInterval="), NULL, 0);
        
        /* minor enhancement options */
        else if (strstr(buffer, "activePlayer=") == buffer)
            enhancementsOptions.activePlayer = (int) strtoul(buffer + strlen("activePlayer="), NULL, 0);
        else if (strstr(buffer, "u5spellMixing=") == buffer)
            enhancementsOptions.u5spellMixing = (int) strtoul(buffer + strlen("u5spellMixing="), NULL, 0);
        else if (strstr(buffer, "u5shrines=") == buffer)
            enhancementsOptions.u5shrines = (int) strtoul(buffer + strlen("u5shrines="), NULL, 0);
        else if (strstr(buffer, "slimeDivides=") == buffer)
            enhancementsOptions.slimeDivides = (int) strtoul(buffer + strlen("slimeDivides="), NULL, 0);
        else if (strstr(buffer, "c64chestTraps=") == buffer)
            enhancementsOptions.c64chestTraps = (int) strtoul(buffer + strlen("c64chestTraps="), NULL, 0);                
        else if (strstr(buffer, "smartEnterKey=") == buffer)
            enhancementsOptions.smartEnterKey = (int) strtoul(buffer + strlen("smartEnterKey="), NULL, 0);
        
        /* major enhancement options */
        else if (strstr(buffer, "u5combat=") == buffer)
            enhancementsOptions.u5combat = (int) strtoul(buffer + strlen("u5combat="), NULL, 0);
        else if (strstr(buffer, "innAlwaysCombat=") == buffer)
            innAlwaysCombat = (int) strtoul(buffer + strlen("innAlwaysCombat="), NULL, 0);
        else if (strstr(buffer, "campingAlwaysCombat=") == buffer)
            campingAlwaysCombat = (int) strtoul(buffer + strlen("campingAlwaysCombat="), NULL, 0);    

        /* mouse options */
        else if (strstr(buffer, "mouseEnabled=") == buffer)
            mouseOptions.enabled = (int) strtoul(buffer + strlen("mouseEnabled="), NULL, 0);

        /**
         * FIXME: this is just to avoid an error for those who have not written
         * a new xu4.cfg file since these items were removed.  Remove them after a reasonable
         * amount of time 
         *
         * remove:  attackspeed, minorEnhancements, majorEnhancements, vol
         */
        
        else if (strstr(buffer, "attackspeed=") == buffer);
        else if (strstr(buffer, "minorEnhancements=") == buffer)
            enhancements = (int)strtoul(buffer + strlen("minorEnhancements="), NULL, 0);
        else if (strstr(buffer, "majorEnhancements=") == buffer);
        else if (strstr(buffer, "vol=") == buffer)
            musicVol = soundVol = (int) strtoul(buffer + strlen("vol="), NULL, 0);        
        
        /***/

        else
            errorWarning("invalid line in settings file %s", buffer);
    }

    fclose(settingsFile);

    eventTimerGranularity = (1000 / gameCyclesPerSecond);
    return true;
}

/**
 * Write the settings out into a human readable file.
 */
bool Settings::write() {    
    FILE *settingsFile;
        
    settingsFile = fopen(filename.c_str(), "wt");
    if (!settingsFile) {
        errorWarning("can't write settings file");
        return false;
    }    

    fprintf(settingsFile, 
            "scale=%d\n"
            "fullscreen=%d\n"
            "filter=%s\n"
            "video=%s\n"
            "gemLayout=%s\n"
            "screenShakes=%d\n"
            "musicVol=%d\n"
            "soundVol=%d\n"
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
            "campingAlwaysCombat=%d\n"
            "mouseEnabled=%d\n",
            scale,
            fullscreen,
            Settings::filters.getName(filter).c_str(),            
            videoType.c_str(),
            gemLayout.c_str(),
            screenShakes,
            musicVol,
            soundVol,
            volumeFades,
            germanKbd,
            shortcutCommands,
            keydelay,
            keyinterval,
            filterMoveMessages,
            battleSpeed,
            enhancements,            
            gameCyclesPerSecond,
            debug,
            Settings::battleDiffs.getName(battleDiff).c_str(),
            validateXml,
            spellEffectSpeed,
            campTime,
            innTime,
            shrineTime,
            shakeInterval,
            enhancementsOptions.activePlayer,
            enhancementsOptions.u5spellMixing,
            enhancementsOptions.u5shrines,
            enhancementsOptions.slimeDivides,
            enhancementsOptions.c64chestTraps,            
            enhancementsOptions.smartEnterKey,
            enhancementsOptions.u5combat,
            innAlwaysCombat,
            campingAlwaysCombat,
            mouseOptions.enabled);

    fclose(settingsFile);
    return true;
}

/**
 * Operators
 */ 
bool Settings::operator==(const Settings &s) const {    
    long offset = (long)&end_of_bitwise_comparators - (long)this;
    if (memcmp(this, &s, offset) != 0)
        return false;

    if (gemLayout != s.gemLayout)
        return false;
    if (videoType != s.videoType)
        return false;

    return true;
}

bool Settings::operator!=(const Settings &s) const {
    return !operator==(s);
}
