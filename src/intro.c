/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "u4.h"
#include "intro.h"
#include "event.h"
#include "screen.h"

typedef enum {
    INTRO_MAP,
    INTRO_MENU,
    INTRO_INIT_NAME,
    INTRO_INIT_SEX,
} IntroMode;

IntroMode mode = INTRO_MAP;
char buffer[16];

int introHandleName(const char *message);

int introKeyHandler(int key, void *data) {
    ReadBufferActionInfo *info;
    int valid = 1;

    switch (mode) {

    case INTRO_MAP:
        mode = INTRO_MENU;
        introUpdateScreen();
        return 1;

    case INTRO_MENU:
        switch (key) {
        case 'i':
            mode = INTRO_INIT_NAME;
            introUpdateScreen();
            screenSetCursorPos(12, 20);
            screenEnableCursor();
            screenForceRedraw();
            info = (ReadBufferActionInfo *) malloc(sizeof(ReadBufferActionInfo));
            info->handleBuffer = &introHandleName;
            info->buffer = buffer;
            info->bufferLen = 16;
            info->screenX = 12;
            info->screenY = 20;
            eventHandlerPushKeyHandlerData(&keyHandlerReadBuffer, info);
            break;
        case 'j':
            eventHandlerSetExitFlag(1);
            break;
        case 'r':
            mode = INTRO_MAP;
            introUpdateScreen();
            break;
        default:
            valid = 0;
            break;
        }
        break;

    case INTRO_INIT_NAME:
        mode = INTRO_MENU;
        introUpdateScreen();
        return 1;

    case INTRO_INIT_SEX:
        mode = INTRO_MENU;
        introUpdateScreen();
        return 1;
    }


    return valid || keyHandlerDefault(key, NULL);
}


void introUpdateScreen() {
    int x, y;
    const int map[][19] = {
        { 6, 6, 6, 4, 4, 4, 1, 1, 0, 0, 0, 0, 1, 4, 4, 13,14,15,4 },
        { 6, 6, 4, 4, 4, 1, 1, 1, 1, 0, 0, 1, 1, 4, 4, 4, 4, 4, 4 },
        { 6, 4, 4, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 10,4, 4, 4, 6 },
        { 6, 4, 4, 1, 1, 2, 2, 1, 1, 9, 8, 1, 1, 1, 1, 4, 6, 6, 6 },
        { 4, 4, 4, 4, 1, 1, 1, 1, 4, 4, 8, 8, 1, 1, 1, 1, 1, 6, 6 }
    };


    switch (mode) {
    case INTRO_MAP:
        screenDrawBackground(BKGD_INTRO);
        for (y = 0; y < (sizeof(map) / sizeof(map[0])); y++) {
            for (x = 0; x < (sizeof(map[0]) / sizeof(map[0][0])); x++) {
                screenShowTile(map[y][x], x, y + 6);
            }
        }
        break;

    case INTRO_MENU:
        screenDrawBackground(BKGD_INTRO);
        screenTextAt(2, 14, "In another world, in a time to come.");
        screenTextAt(15, 16, "Options:");
        screenTextAt(11, 17, "Return to the view");
        screenTextAt(11, 18, "Journey Onward");
        screenTextAt(11, 19, "Initiate New Game");
        screenTextAt(3, 21, "xu4 is free software, see COPYING");
        screenTextAt(5, 22, "\011 Copyright 1987 Lord British");
        break;

    case INTRO_INIT_NAME:
        screenDrawBackground(BKGD_INTRO);
        screenTextAt(4, 16, "By what name shalt thou be known");
        screenTextAt(4, 17, "in this world and time?");
        break;

    case INTRO_INIT_SEX:
        screenDrawBackground(BKGD_INTRO);
        screenTextAt(4, 16, "Art thou Male or Female?");
        break;

    default:
        assert(0);
    }

    screenForceRedraw();
}

int introHandleName(const char *message) {
    mode = INTRO_INIT_SEX;
    printf("%s\n", message);
    return 1;
}

