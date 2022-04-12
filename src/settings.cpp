/*
 * $Id$
 */

#include <cctype>
#include <cstring>

#include "settings.h"

#include "error.h"
#include "filesystem.h"
#include "screen.h"
#include "xu4.h"

#if defined(_WIN32) || defined(__CYGWIN__)
#include <windows.h>
#include <shlobj.h>
#elif defined(MACOSX)
#include <CoreServices/CoreServices.h>
#elif defined(IOS)
#include <CoreFoundation/CoreFoundation.h>
#include "U4CFHelper.h"
#include "ios_helpers.h"
#endif

using namespace std;

#if defined(_WIN32) || defined(__CYGWIN__)
#define SETTINGS_BASE_FILENAME "xu4.cfg"
#else
#define SETTINGS_BASE_FILENAME "xu4rc"
#endif

bool SettingsData::operator==(const SettingsData &s) const {
    intptr_t offset = (intptr_t)&end_of_bitwise_comparators - (intptr_t)this;
    if (memcmp(this, &s, offset) != 0)
        return false;

    if (gemLayout != s.gemLayout)
        return false;
    if (videoType != s.videoType)
        return false;
    if (logging != s.logging)
        return false;
    if (game != s.game)
        return false;

    return true;
}

bool SettingsData::operator!=(const SettingsData &s) const {
    return !operator==(s);
}


/**
 * Initialize the settings.
 */
void Settings::init(const char* profileName) {
    if (profileName && profileName[0]) {
        userPath = "./profiles/";
        userPath += profileName;
        userPath += "/";

        profile = profileName;
        if (profile.length() > 20)
            errorFatal("Profile name must be no more than 20 characters.");
    } else {
        profile.clear();

#if defined(MACOSX)
            FSRef folder;
            OSErr err = FSFindFolder(kUserDomain, kApplicationSupportFolderType, kCreateFolder, &folder);
            if (err == noErr) {
                UInt8 path[2048];
                if (FSRefMakePath(&folder, path, 2048) == noErr) {
                    userPath.append(reinterpret_cast<const char *>(path));
                    userPath.append("/xu4/");
                }
            }
            if (userPath.empty()) {
                char *home = getenv("HOME");
                if (home && home[0]) {
                    if (userPath.size() == 0) {
                        userPath += home;
                        userPath += "/.xu4";
                        userPath += "/";
                    }
                } else {
                    userPath = "./";
                }
            }
#elif defined(IOS)
        boost::intrusive_ptr<CFURL> urlForLocation = cftypeFromCreateOrCopy(U4IOS::copyAppSupportDirectoryLocation());
        if (urlForLocation != 0) {
            char path[2048];
            boost::intrusive_ptr<CFString> tmpStr = cftypeFromCreateOrCopy(CFURLCopyFileSystemPath(urlForLocation.get(), kCFURLPOSIXPathStyle));
            if (CFStringGetFileSystemRepresentation(tmpStr.get(), path, 2048) != false) {
                userPath.append(path);
                userPath += "/";
            }
        }
#elif defined(__unix__)
        char *home = getenv("HOME");
        if (home && home[0]) {
            userPath += home;
#ifdef __linux__
            userPath += "/.config/xu4/";
#else
            userPath += "/.xu4/";
#endif
        } else
            userPath = "./";
#elif defined(_WIN32) || defined(__CYGWIN__)
        char* appdata = getenv("APPDATA");
        if (appdata) {
            userPath = appdata;
            userPath += "\\xu4\\";
        } else
            userPath = ".\\";
#else
        userPath = "./";
#endif

    }
    FileSystem::createDirectory(userPath);

    filename = userPath + SETTINGS_BASE_FILENAME;

    read();
}

void Settings::setData(const SettingsData &data) {
    // bitwise copy is safe
    *(SettingsData *)this = data;
}

/*
 * Return index of value in the names string list, or zero (the first name) if
 * there is no matching name.
 */
uint8_t Settings::settingEnum(const char** names, const char* value) {
    const char** it = names;
    while (*it) {
        if (strcasecmp(*it, value) == 0)
            return it - names;
        ++it;
    }
    return 0;
}

// Return pointer to start of trimmed line.
static char* trimLine(char* buf) {
    char* end;
    int ch;

    // Trim spaces & tabs from start.
    while ((ch = *buf)) {
        if (ch != ' ' && ch != '\t')
            break;
        ++buf;
    }

    // Find end of line.
    end = buf;
    while (*end)
        ++end;

    // Remove whitespace from end of line.
    while (end != buf && isspace(end[-1])) {
        --end;
        *end = '\0';
    }
    return buf;
}

// Return pointer to setting value or NULL if buf does not start with name.
static const char* settingValue(const char* buf, const char* name) {
    // Check the initial character first to provide a fast fail path.
    if (name[0] == buf[0]) {
        int n = strlen(name);
        if (strncmp(buf, name, n) == 0)
            return buf + n;
    }
    return NULL;
}

static int toInt(const char* str) {
    return (int) strtoul(str, NULL, 0);
}

/**
 * Read settings in from the settings file.
 */
bool Settings::read() {
    char buffer[256];
    const char* cp;
    const char* val;
    FILE *settingsFile;

    /* default settings */
    scale                 = DEFAULT_SCALE;
    fullscreen            = DEFAULT_FULLSCREEN;
    filter                = DEFAULT_FILTER;
    videoType             = DEFAULT_VIDEO_TYPE;
    gemLayout             = DEFAULT_GEM_LAYOUT;
    lineOfSight           = DEFAULT_LINEOFSIGHT;
    screenShakes          = DEFAULT_SCREEN_SHAKES;
    gamma                 = DEFAULT_GAMMA;
    musicVol              = DEFAULT_MUSIC_VOLUME;
    soundVol              = DEFAULT_SOUND_VOLUME;
    volumeFades           = DEFAULT_VOLUME_FADES;
    shortcutCommands      = DEFAULT_SHORTCUT_COMMANDS;
    keydelay              = DEFAULT_KEY_DELAY;
    keyinterval           = DEFAULT_KEY_INTERVAL;
    filterMoveMessages    = DEFAULT_FILTER_MOVE_MESSAGES;
    battleSpeed           = DEFAULT_BATTLE_SPEED;
    enhancements          = DEFAULT_ENHANCEMENTS;
    gameCyclesPerSecond   = DEFAULT_CYCLES_PER_SECOND;
    screenAnimationFramesPerSecond = DEFAULT_ANIMATION_FRAMES_PER_SECOND;
    debug                 = DEFAULT_DEBUG;
    battleDiff            = DEFAULT_BATTLE_DIFFICULTY;
    spellEffectSpeed      = DEFAULT_SPELL_EFFECT_SPEED;
    campTime              = DEFAULT_CAMP_TIME;
    innTime               = DEFAULT_INN_TIME;
    shrineTime            = DEFAULT_SHRINE_TIME;
    shakeInterval         = DEFAULT_SHAKE_INTERVAL;
    titleSpeedRandom      = DEFAULT_TITLE_SPEED_RANDOM;
    titleSpeedOther       = DEFAULT_TITLE_SPEED_OTHER;

#if 0
    pauseForEachMovement  = DEFAULT_PAUSE_FOR_EACH_MOVEMENT;
    pauseForEachTurn      = DEFAULT_PAUSE_FOR_EACH_TURN;
#endif

    /* all specific minor enhancements default to "on", any major enhancements default to "off" */
    enhancementsOptions.activePlayer     = true;
    enhancementsOptions.u5spellMixing    = true;
    enhancementsOptions.u5shrines        = true;
    enhancementsOptions.slimeDivides     = true;
    enhancementsOptions.gazerSpawnsInsects = true;
    enhancementsOptions.textColorization = false;
    enhancementsOptions.c64chestTraps    = true;
    enhancementsOptions.smartEnterKey    = true;
    enhancementsOptions.peerShowsObjects = false;
    enhancementsOptions.u5combat         = false;
    enhancementsOptions.u4TileTransparencyHack = true;
    enhancementsOptions.u4TileTransparencyHackPixelShadowOpacity = DEFAULT_SHADOW_PIXEL_OPACITY;
    enhancementsOptions.u4TrileTransparencyHackShadowBreadth = DEFAULT_SHADOW_PIXEL_SIZE;

    innAlwaysCombat = 0;
    campingAlwaysCombat = 0;

    /* mouse defaults to on */
    mouseOptions.enabled = 1;

    logging = DEFAULT_LOGGING;
    game = "Ultima-IV";

    settingsFile = fopen(filename.c_str(), "rt");
    if (!settingsFile)
        return false;

#define VALUE(name) (val = settingValue(cp, name))

    while(fgets(buffer, sizeof(buffer), settingsFile) != NULL) {
        cp = trimLine(buffer);

        // Skip empty and comment lines.
        if (cp[0] == '\n' || cp[0] == '#' || cp[0] == ';')
            continue;

        if (VALUE("scale="))
            scale = (unsigned int) toInt(val);
        else if (VALUE("fullscreen="))
            fullscreen = toInt(val);
        else if (VALUE("filter="))
            filter = settingEnum(screenGetFilterNames(), val);
        else if (VALUE("lineOfSight="))
            lineOfSight = settingEnum(screenGetLineOfSightStyles(), val);
        else if (VALUE("video="))
            videoType = val;
        else if (VALUE("gemLayout="))
            gemLayout = val;
        else if (VALUE("screenShakes="))
            screenShakes = toInt(val);
        else if (VALUE("gamma="))
            gamma = toInt(val);
        else if (VALUE("musicVol="))
            musicVol = toInt(val);
        else if (VALUE("soundVol="))
            soundVol = toInt(val);
        else if (VALUE("volumeFades="))
            volumeFades = toInt(val);
        else if (VALUE("shortcutCommands="))
            shortcutCommands = toInt(val);
        else if (VALUE("keydelay="))
            keydelay = toInt(val);
        else if (VALUE("keyinterval="))
            keyinterval = toInt(val);
        else if (VALUE("filterMoveMessages="))
            filterMoveMessages = toInt(val);
        else if (VALUE("battlespeed="))
            battleSpeed = toInt(val);
        else if (VALUE("enhancements="))
            enhancements = toInt(val);
        else if (VALUE("gameCyclesPerSecond="))
            gameCyclesPerSecond = toInt(val);
        else if (VALUE("debug="))
            debug = toInt(val);
        else if (VALUE("battleDiff="))
            battleDiff = settingEnum(battleDiffStrings(), val);
        else if (VALUE("spellEffectSpeed="))
            spellEffectSpeed = toInt(val);
        else if (VALUE("campTime="))
            campTime = toInt(val);
        else if (VALUE("innTime="))
            innTime = toInt(val);
        else if (VALUE("shrineTime="))
            shrineTime = toInt(val);
        else if (VALUE("shakeInterval="))
            shakeInterval = toInt(val);
        else if (VALUE("titleSpeedRandom="))
            titleSpeedRandom = toInt(val);
        else if (VALUE("titleSpeedOther="))
            titleSpeedOther = toInt(val);

        /* minor enhancement options */
        else if (VALUE("activePlayer="))
            enhancementsOptions.activePlayer = toInt(val);
        else if (VALUE("u5spellMixing="))
            enhancementsOptions.u5spellMixing = toInt(val);
        else if (VALUE("u5shrines="))
            enhancementsOptions.u5shrines = toInt(val);
        else if (VALUE("slimeDivides="))
            enhancementsOptions.slimeDivides = toInt(val);
        else if (VALUE("gazerSpawnsInsects="))
            enhancementsOptions.gazerSpawnsInsects = toInt(val);
        else if (VALUE("textColorization="))
            enhancementsOptions.textColorization = toInt(val);
        else if (VALUE("c64chestTraps="))
            enhancementsOptions.c64chestTraps = toInt(val);
        else if (VALUE("smartEnterKey="))
            enhancementsOptions.smartEnterKey = toInt(val);

        /* major enhancement options */
        else if (VALUE("peerShowsObjects="))
            enhancementsOptions.peerShowsObjects = toInt(val);
        else if (VALUE("u5combat="))
            enhancementsOptions.u5combat = toInt(val);
        else if (VALUE("innAlwaysCombat="))
            innAlwaysCombat = toInt(val);
        else if (VALUE("campingAlwaysCombat="))
            campingAlwaysCombat = toInt(val);

        /* mouse options */
        else if (VALUE("mouseEnabled="))
            mouseOptions.enabled = toInt(val);
        else if (VALUE("logging="))
            logging = val;
        else if (VALUE("game="))
            game = val;

        /* graphics enhancements options */
        else if (VALUE("renderTileTransparency="))
            enhancementsOptions.u4TileTransparencyHack = toInt(val);
        else if (VALUE("transparentTilePixelShadowOpacity="))
            enhancementsOptions.u4TileTransparencyHackPixelShadowOpacity = toInt(val);
        else if (VALUE("transparentTileShadowSize="))
            enhancementsOptions.u4TrileTransparencyHackShadowBreadth = toInt(val);
        else
            errorWarning("invalid line in settings file %s", buffer);
    }

    fclose(settingsFile);
    return true;
}

/**
 * Write the settings out into a human readable file.  This also
 * notifies observers that changes have been commited.
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
            "lineOfSight=%s\n"
            "screenShakes=%d\n"
            "gamma=%d\n"
            "musicVol=%d\n"
            "soundVol=%d\n"
            "volumeFades=%d\n"
            "shortcutCommands=%d\n"
            "keydelay=%d\n"
            "keyinterval=%d\n"
            "filterMoveMessages=%d\n"
            "battlespeed=%d\n"
            "enhancements=%d\n"
            "gameCyclesPerSecond=%d\n"
            "debug=%d\n"
            "battleDiff=%s\n"
            "spellEffectSpeed=%d\n"
            "campTime=%d\n"
            "innTime=%d\n"
            "shrineTime=%d\n"
            "shakeInterval=%d\n"
            "titleSpeedRandom=%d\n"
            "titleSpeedOther=%d\n",
            scale,
            fullscreen,
            screenGetFilterNames()[ filter ],
            videoType.c_str(),
            gemLayout.c_str(),
            screenGetLineOfSightStyles()[ lineOfSight ],
            screenShakes,
            gamma,
            musicVol,
            soundVol,
            volumeFades,
            shortcutCommands,
            keydelay,
            keyinterval,
            filterMoveMessages,
            battleSpeed,
            enhancements,
            gameCyclesPerSecond,
            debug,
            battleDiffStrings()[ battleDiff ],
            spellEffectSpeed,
            campTime,
            innTime,
            shrineTime,
            shakeInterval,
            titleSpeedRandom,
            titleSpeedOther);

    // Enhancements Options
    fprintf(settingsFile,
            "activePlayer=%d\n"
            "u5spellMixing=%d\n"
            "u5shrines=%d\n"
            "slimeDivides=%d\n"
            "gazerSpawnsInsects=%d\n"
            "textColorization=%d\n"
            "c64chestTraps=%d\n"
            "smartEnterKey=%d\n"
            "peerShowsObjects=%d\n"
            "u5combat=%d\n"
            "innAlwaysCombat=%d\n"
            "campingAlwaysCombat=%d\n"
            "mouseEnabled=%d\n"
            "logging=%s\n"
            "game=%s\n"
            "renderTileTransparency=%d\n"
            "transparentTilePixelShadowOpacity=%d\n"
            "transparentTileShadowSize=%d\n",
            enhancementsOptions.activePlayer,
            enhancementsOptions.u5spellMixing,
            enhancementsOptions.u5shrines,
            enhancementsOptions.slimeDivides,
            enhancementsOptions.gazerSpawnsInsects,
            enhancementsOptions.textColorization,
            enhancementsOptions.c64chestTraps,
            enhancementsOptions.smartEnterKey,
            enhancementsOptions.peerShowsObjects,
            enhancementsOptions.u5combat,
            innAlwaysCombat,
            campingAlwaysCombat,
            mouseOptions.enabled,
            logging.c_str(),
            game.c_str(),
            enhancementsOptions.u4TileTransparencyHack,
            enhancementsOptions.u4TileTransparencyHackPixelShadowOpacity,
            enhancementsOptions.u4TrileTransparencyHackShadowBreadth);

    fclose(settingsFile);

    gs_emitMessage(SENDER_SETTINGS, this);

    return true;
}

const char** Settings::battleDiffStrings() {
    static const char* difficulty[] = {"Normal", "Hard", "Expert", NULL};
    return difficulty;
}
