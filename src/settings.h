/*
 * $Id$
 */

#ifndef SETTINGS_H
#define SETTINGS_H

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
#define DEFAULT_VIDEO_TYPE              VIDEO_VGA
#define DEFAULT_SCREEN_SHAKES           1
#define DEFAULT_VOLUME                  1
#define DEFAULT_VOLUME_FADES            1
#define DEFAULT_GERMAN_KEYBOARD         0
#define DEFAULT_SHORTCUT_COMMANDS       0
#define DEFAULT_KEY_DELAY               500
#define DEFAULT_KEY_INTERVAL            30
#define DEFAULT_FILTER_MOVE_MESSAGES    0
#define DEFAULT_BATTLE_SPEED            5
#define DEFAULT_MINOR_ENHANCEMENTS      1
#define DEFAULT_MAJOR_ENHANCEMENTS      0
#define DEFAULT_CYCLES_PER_SECOND       4
#define DEFAULT_DEBUG                   0
#define DEFAULT_VALIDATE_XML            1
#define DEFAULT_SPELL_EFFECT_SPEED      5
#define DEFAULT_CAMP_TIME               10
#define DEFAULT_INN_TIME                8
#define DEFAULT_SHRINE_TIME             16
#define DEFAULT_SHAKE_INTERVAL          100

typedef enum {
    SCL_MIN,
    SCL_POINT,
    SCL_2xBi,
    SCL_2xSaI,
    SCL_Scale2x,
    SCL_MAX
} FilterType;

typedef enum {
    VIDEO_MIN,
    VIDEO_VGA,
    VIDEO_EGA,
    //VIDEO_CGA,
    VIDEO_MAX
} VideoType;

typedef struct _SettingsMinorOptions {
    int u5shrines;
    int slimeDivides;
    int c64chestTraps;    
    int u5spellMixing;
} SettingsMinorOptions;

typedef struct _SettingsMajorOptions {
    int u5combat;
} SettingsMajorOptions;

typedef struct _Settings {
    unsigned int scale;
    int fullscreen;
    FilterType filter;
    VideoType videoType;
    int screenShakes;
    int vol;
    int volumeFades;
    int germanKbd;
    int shortcutCommands;
    int keydelay;
    int keyinterval;
    int filterMoveMessages;
    int battleSpeed;
    int minorEnhancements;
    int majorEnhancements;
    int gameCyclesPerSecond;
    int debug;
    int validateXml;
    int spellEffectSpeed;
    int campTime;
    int innTime;
    int shrineTime;
    int shakeInterval;
    SettingsMinorOptions minorEnhancementsOptions;
    SettingsMajorOptions majorEnhancementsOptions;
} Settings;

char *settingsFilename(void);
void settingsRead(void);
void settingsWrite(void);
const char *settingsFilterToString(FilterType filter);
FilterType settingsStringToFilter(const char *str);
const char *settingsVideoTypeToString(VideoType type);
VideoType settingsStringToVideoType(const char *str);

/* the global settings */
extern Settings *settings;

#endif
