/*
 * $Id$
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <map>
#include <string>
#include "types.h"

using std::string;

#define MIN_SHAKE_INTERVAL              50

#define MAX_BATTLE_SPEED                10
#define MAX_KEY_DELAY                   1000
#define MAX_KEY_INTERVAL                100
#define MAX_CYCLES_PER_SECOND           20
#define MAX_SPELL_EFFECT_SPEED          10
#define MAX_CAMP_TIME                   10
#define MAX_INN_TIME                    10
#define MAX_SHRINE_TIME                 20
#define MAX_SHAKE_INTERVAL              200

#define DEFAULT_SCALE                   2
#define DEFAULT_FULLSCREEN              0
#define DEFAULT_FILTER                  SCL_Scale2x
#define DEFAULT_VIDEO_TYPE              "VGA"
#define DEFAULT_GEM_LAYOUT              "Standard"
#define DEFAULT_SCREEN_SHAKES           1
#define DEFAULT_MUSIC_VOLUME            1
#define DEFAULT_SOUND_VOLUME            1
#define DEFAULT_VOLUME_FADES            1
#define DEFAULT_SHORTCUT_COMMANDS       0
#define DEFAULT_KEY_DELAY               500
#define DEFAULT_KEY_INTERVAL            30
#define DEFAULT_FILTER_MOVE_MESSAGES    0
#define DEFAULT_BATTLE_SPEED            5
#define DEFAULT_ENHANCEMENTS            1
#define DEFAULT_CYCLES_PER_SECOND       4
#define DEFAULT_DEBUG                   0
#define DEFAULT_VALIDATE_XML            1
#define DEFAULT_SPELL_EFFECT_SPEED      10
#define DEFAULT_CAMP_TIME               10
#define DEFAULT_INN_TIME                8
#define DEFAULT_SHRINE_TIME             16
#define DEFAULT_SHAKE_INTERVAL          100
#define DEFAULT_BATTLE_DIFFICULTY       DIFF_NORMAL

typedef enum {
    SCL_MIN,
    SCL_POINT,
    SCL_2xBi,
    SCL_2xSaI,
    SCL_Scale2x,
    SCL_MAX
} FilterType;

typedef enum {
    DIFF_MIN,
    DIFF_NORMAL,
    DIFF_HARD,
    DIFF_EXPERT,
    DIFF_MAX
} BattleDifficulty;

typedef struct _SettingsEnhancementOptions {
    int activePlayer;
    int u5spellMixing;
    int u5shrines;
    int u5combat;
    int slimeDivides;
    int c64chestTraps;    
    int smartEnterKey;
} SettingsEnhancementOptions;

typedef struct _MouseOptions {
    int enabled;
} MouseOptions;

/**
 * Translator Class -- used to provide translation between
 * a string value and a type.
 */ 
class Translator {
    typedef std::map<string, int, std::less<string> >    T_s;    
    typedef std::map<int, string, std::less<int> >		 T_t;
public:
    Translator() {}
    Translator(const string values[]) {
        /**
         * Initialize the maps
         */ 
        for (int i = 0; !values[i].empty(); i++) {
            s_map[values[i]] = i;
            t_map[i] = values[i];
        }
    }

    int getType(string name) {
        T_s::iterator found = s_map.find(name);
        if (found != s_map.end())
            return found->second;
        return -1;
    }
    
    string getName(int type) {
        T_t::iterator found = t_map.find(type);
        if (found != t_map.end())
            return found->second;
        return "";
    }

private:
    T_s s_map;
    T_t t_map;
};

/**
 * Settings class definition
 */ 
class Settings {
    typedef Translator FilterTranslator;
    typedef Translator BattleDiffTranslator;
    typedef std::map<string, int, std::less<string> > SettingsMap;

public:
    Settings();

    /* Methods */
    bool read();
    bool write();

    bool operator==(const Settings &) const;
    bool operator!=(const Settings &) const;

    /* Properties */    
    static FilterTranslator         filters;
    static BattleDiffTranslator     battleDiffs;    

    BattleDifficulty    battleDiff;
    int                 battleSpeed;
    bool                campingAlwaysCombat;
    int                 campTime;
    bool                debug;
    bool                enhancements;
    SettingsEnhancementOptions enhancementsOptions;    
    FilterType          filter;
    bool                filterMoveMessages;
    bool                fullscreen;
    int                 gameCyclesPerSecond;    
    bool                innAlwaysCombat;
    int                 innTime;
    int                 keydelay;
    int                 keyinterval;
    MouseOptions        mouseOptions;
    bool                musicVol;
    unsigned int        scale;
    bool                screenShakes;
    int                 shakeInterval;
    bool                shortcutCommands;
    int                 shrineTime;
    bool                soundVol;
    int                 spellEffectSpeed;
    bool                validateXml;    
    bool                volumeFades;

    /**
     * Strings, classes, and other objects that cannot
     * be bitwise-compared must be placed here at the
     * end of the list so that our == and != operators
     * function correctly
     */ 
    long                end_of_bitwise_comparators;

    string              gemLayout;
    string              videoType;

private:
    string filename;
};

/* the global settings */
extern Settings settings;

#endif
