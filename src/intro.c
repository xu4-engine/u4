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
    INTRO_MENU
} IntroMode;

IntroMode mode = INTRO_MAP;

int introKeyHandler(int key, void *data) {
    int valid = 1;

    if (mode == INTRO_MAP) {
        mode = INTRO_MENU;
        introUpdateScreen();
        return 1;
    }

    else if (mode == INTRO_MENU) {
        switch (key) {
        case 'i':
            printf("FIXME\n");
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

    screenDrawBackground(BKGD_INTRO);

    if (mode == INTRO_MAP) {
        for (y = 0; y < (sizeof(map) / sizeof(map[0])); y++) {
            for (x = 0; x < (sizeof(map[0]) / sizeof(map[0][0])); x++) {
                screenShowTile(map[y][x], x, y + 6);
            }
        }
    }

    else if (mode == INTRO_MENU) {
        screenTextAt(2, 14, "In another world, in a time to come.");
        screenTextAt(15, 16, "Options:");
        screenTextAt(11, 17, "Return to the view");
        screenTextAt(11, 18, "Journey Onward");
        screenTextAt(11, 19, "Initiate New Game");
        screenTextAt(3, 21, "Conversion by James Van Artsdalen");
        screenTextAt(5, 22, "\011 Copyright 1987 Lord British");
    }

    else
        assert(0);

    screenForceRedraw();
}
