/*
 * $Id$
 */

#ifndef SETTINGS_H
#define SETTINGS_H

typedef enum {
    SCL_POINT,
    SCL_2xBi,
    SCL_2xSaI,
    SCL_AdvanceMAME,
    SCL_MAX
} FilterType;


typedef struct _Settings {
    unsigned int scale;
    int fullscreen;
    FilterType filter;
    int vol;
    int germanKbd;
} Settings;

char *settingsFilename(void);
void settingsRead(void);
void settingsWrite(void);
const char *settingsFilterToString(FilterType filter);
FilterType settingsStringToFilter(const char *str);

/* the global settings */
extern Settings *settings;

#endif
