/*
 * $Id$
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

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

typedef struct _Settings {
    unsigned int scale;
    int fullscreen;
    FilterType filter;
    char *videoType;
    char *gemLayout;
    int screenShakes;
    int musicVol;
    int soundVol;
    int volumeFades;
    int shortcutCommands;
    int keydelay;
    int keyinterval;
    int filterMoveMessages;
    int battleSpeed;
    int enhancements;    
    int gameCyclesPerSecond;
    int debug;
    BattleDifficulty battleDiff;
    int validateXml;
    int spellEffectSpeed;
    int campTime;
    int innTime;
    int shrineTime;
    int shakeInterval;
    SettingsEnhancementOptions enhancementsOptions;
    int innAlwaysCombat;
    int campingAlwaysCombat;
    MouseOptions mouseOptions;
} Settings;

char *settingsFilename(void);
void settingsRead(void);
void settingsWrite(void);
void settingsCopy(Settings *to, const Settings *from);
int settingsCompare(const Settings *s1, const Settings *s2);
const char *settingsFilterToString(FilterType filter);
FilterType settingsStringToFilter(const char *str);
const char *settingsBattleDiffToString(BattleDifficulty diff);
BattleDifficulty settingsStringToBattleDiff(const char *str);

/* the global settings */
extern Settings *settings;

#ifdef __cplusplus
}
#endif

#endif
