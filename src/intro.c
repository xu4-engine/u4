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
#include "u4file.h"

typedef enum {
    INTRO_MAP,
    INTRO_MENU,
    INTRO_INIT_NAME,
    INTRO_INIT_SEX,
    INTRO_INIT_STORY,
    INTRO_INIT_QUESTIONS
} IntroMode;

char storyInd;
char *introText[24];
char *introQuestions[28];
char *introGypsy[4];
int questionRound;
int answerInd;
int questionTree[15];

IntroMode mode = INTRO_MAP;
char buffer[16];

int introHandleName(const char *message);
int introHandleSex(const char *message);
void introShowText(int text);
void introInitQuestionTree();
int introDoQuestion(int answer);

int introInit() {
    FILE *title;
    int i, j;
    char buffer[256];

    title = u4fopen("title.exe");
    if (!title)
        return 0;

    fseek(title, 17445, SEEK_SET);
    for (i = 0; i < sizeof(introQuestions) / sizeof(introQuestions[0]); i++) {
        for (j = 0; j < sizeof(buffer) - 1; j++) {
            buffer[j] = fgetc(title);
            if (buffer[j] == '\0')
                break;
        }
        introQuestions[i] = strdup(buffer);
    }

    for (i = 0; i < sizeof(introText) / sizeof(introText[0]); i++) {
        for (j = 0; j < sizeof(buffer) - 1; j++) {
            buffer[j] = fgetc(title);
            if (buffer[j] == '\0')
                break;
        }
        introText[i] = strdup(buffer);
    }

    for (i = 0; i < sizeof(introGypsy) / sizeof(introGypsy[0]); i++) {
        for (j = 0; j < sizeof(buffer) - 1; j++) {
            buffer[j] = fgetc(title);
            if (buffer[j] == '\0')
                break;
        }
        introGypsy[i] = strdup(buffer);
    }

    u4fclose(title);

    screenLoadCards();

    return 1;
}


void introDelete() {
    int i;

    for (i = 0; i < sizeof(introQuestions) / sizeof(introQuestions[0]); i++)
        free(introQuestions[i]);
    for (i = 0; i < sizeof(introText) / sizeof(introText[0]); i++)
        free(introText[i]);
    for (i = 0; i < sizeof(introGypsy) / sizeof(introGypsy[0]); i++)
        free(introGypsy[i]);

    screenFreeCards();
    screenFreeIntroBackgrounds();
}

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
            buffer[0] = '\0';
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
        assert(0);              /* shouldn't happen */
        return 1;

    case INTRO_INIT_SEX:
        assert(0);              /* shouldn't happen */
        return 1;

    case INTRO_INIT_STORY:
        storyInd++;
        if (storyInd >= 24) {
            mode = INTRO_INIT_QUESTIONS;
            questionRound = 0;
            introInitQuestionTree();
        }
        introUpdateScreen();
        return 1;

    case INTRO_INIT_QUESTIONS:
        if (introDoQuestion(0)) {
            screenDisableCursor();
            mode = INTRO_MENU;
        }
        introUpdateScreen();
        return 1;
    }


    return valid || keyHandlerDefault(key, NULL);
}

void introDrawMap() {
    int x, y;
    const int map[][19] = {
        { 6, 6, 6, 4, 4, 4, 1, 1, 0, 0, 0, 0, 1, 4, 4, 13,14,15,4 },
        { 6, 6, 4, 4, 4, 1, 1, 1, 1, 0, 0, 1, 1, 4, 4, 4, 4, 4, 4 },
        { 6, 4, 4, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 10,4, 4, 4, 6 },
        { 6, 4, 4, 1, 1, 2, 2, 1, 1, 9, 8, 1, 1, 1, 1, 4, 6, 6, 6 },
        { 4, 4, 4, 4, 1, 1, 1, 1, 4, 4, 8, 8, 1, 1, 1, 1, 1, 6, 6 }
    };
    
    for (y = 0; y < (sizeof(map) / sizeof(map[0])); y++) {
        for (x = 0; x < (sizeof(map[0]) / sizeof(map[0][0])); x++) {
            screenShowTile(map[y][x], x, y + 6);
        }
    }
}

void introUpdateScreen() {
    switch (mode) {
    case INTRO_MAP:
        screenDrawBackground(BKGD_INTRO);
        introDrawMap();
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

    case INTRO_INIT_STORY:
        if (storyInd == 0)
            screenDrawBackground(BKGD_TREE);
        else if (storyInd == 3)
            screenAnimateIntro(0);
        else if (storyInd == 5)
            screenAnimateIntro(1);
        else if (storyInd == 6)
            screenDrawBackground(BKGD_PORTAL);
        else if (storyInd == 11)
            screenDrawBackground(BKGD_TREE);
        else if (storyInd == 15)
            screenDrawBackground(BKGD_OUTSIDE);
        else if (storyInd == 17)
            screenDrawBackground(BKGD_INSIDE);
        else if (storyInd == 20)
            screenDrawBackground(BKGD_WAGON);
        else if (storyInd == 21)
            screenDrawBackground(BKGD_GYPSY);
        else if (storyInd == 23)
            screenDrawBackground(BKGD_ABACUS);
        introShowText(storyInd);
        break;

    case INTRO_INIT_QUESTIONS:
        screenDrawBackground(BKGD_ABACUS);
        screenShowCard(0, questionTree[questionRound * 2]);
        screenShowCard(1, questionTree[questionRound * 2 + 1]);
        break;

    default:
        assert(0);
    }

    screenForceRedraw();
}

int introHandleName(const char *message) {
    ReadBufferActionInfo *info;

    printf("name = %s\n", message);

    eventHandlerPopKeyHandler();
    mode = INTRO_INIT_SEX;

    introUpdateScreen();
    screenSetCursorPos(29, 16);
    screenEnableCursor();
    screenForceRedraw();

    info = (ReadBufferActionInfo *) malloc(sizeof(ReadBufferActionInfo));
    info->handleBuffer = &introHandleSex;
    info->buffer = buffer;
    info->bufferLen = 2;
    info->screenX = 29;
    info->screenY = 16;
    buffer[0] = '\0';
    eventHandlerPushKeyHandlerData(&keyHandlerReadBuffer, info);

    return 1;
}

int introHandleSex(const char *message) {

    printf("sex = %s\n", message);

    eventHandlerPopKeyHandler();
    mode = INTRO_INIT_STORY;
    storyInd = 23;

    introUpdateScreen();
    screenForceRedraw();

    return 1;
}

void introGetQuestion() {
    
}

void introShowText(int text) {
    char line[41];
    char *p;
    int len, lineNo = 19;

    screenEraseIntroText();

    p = introText[text];
    while (*p) {
        len = strcspn(p, "\n");
        strncpy(line, p, len);
        line[len] = '\0';
        p += len;
        if (*p == '\n')
            p++;
        screenTextAt(0, lineNo, "%s", line);
        lineNo++;
    }
    screenSetCursorPos(strlen(line), lineNo - 1);
}

void introTimer() {
    screenCycle();
    screenUpdateCursor();
    if (mode == INTRO_MAP)
        introDrawMap();
    screenForceRedraw();
}

void introInitQuestionTree() {
    int i, tmp, r;

    for (i = 0; i < 8; i++)
        questionTree[i] = i;

    for (i = 0; i < 8; i++) {
        r = rand() % 8;
        tmp = questionTree[r];
        questionTree[r] = questionTree[i];
        questionTree[i] = tmp;
    }
    answerInd = 8;

    if (questionTree[0] > questionTree[1]) {
        tmp = questionTree[0];
        questionTree[0] = questionTree[1];
        questionTree[1] = tmp;
    }
        
}

int introDoQuestion(int answer) {
    int tmp;

    if (!answer)
        questionTree[answerInd] = questionTree[questionRound * 2];
    else
        questionTree[answerInd] = questionTree[questionRound * 2 + 1];

    answerInd++;
    questionRound++;

    if (questionRound > 6)
        return 1;

    if (questionTree[questionRound * 2] > questionTree[questionRound * 2 + 1]) {
        tmp = questionTree[questionRound * 2];
        questionTree[questionRound * 2] = questionTree[questionRound * 2 + 1];
        questionTree[questionRound * 2 + 1] = tmp;
    }

    return 0;
}
