/*
 * $Id$
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#define MAX_BATTLE_SPEED                10
#define MAX_KEY_DELAY                   1000
#define MAX_KEY_INTERVAL                100
#define MAX_CYCLES_PER_SECOND           20
#define MAX_SPELL_EFFECT_SPEED          5

#define DEFAULT_SCALE                   2
#define DEFAULT_FULLSCREEN              0
#define DEFAULT_FILTER                  SCL_Scale2x
#define DEFAULT_VOLUME                  1
#define DEFAULT_GERMAN_KEYBOARD         0
#define DEFAULT_SHORTCUT_COMMANDS       0
#define DEFAULT_KEY_DELAY               500
#define DEFAULT_KEY_INTERVAL            30
#define DEFAULT_FILTER_MOVE_MESSAGES    0
#define DEFAULT_BATTLE_SPEED            5
#define DEFAULT_MINOR_ENHANCEMENTS      1
#define DEFAULT_CYCLES_PER_SECOND       4
#define DEFAULT_DEBUG                   0
#define DEFAULT_VALIDATE_XML            1
#define DEFAULT_SPELL_EFFECT_SPEED      4

typedef enum {
    SCL_POINT,
    SCL_2xBi,
    SCL_2xSaI,
    SCL_Scale2x,
    SCL_MAX
} FilterType;

typedef struct _Settings {
    unsigned int scale;
    int fullscreen;
    FilterType filter;
    int vol;
    int germanKbd;
    int shortcutCommands;
    int keydelay;
    int keyinterval;
    int filterMoveMessages;
    int battleSpeed;
    int minorEnhancements;
    int gameCyclesPerSecond;
    int debug;
    int validateXml;
    int spellEffectSpeed;
} Settings;

char *settingsFilename(void);
void settingsRead(void);
void settingsWrite(void);
const char *settingsFilterToString(FilterType filter);
FilterType settingsStringToFilter(const char *str);

/* the global settings */
extern Settings *settings;

#endif
