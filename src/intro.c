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
#include "savegame.h"

#define INTRO_TEXT_OFFSET 17445
#define INTRO_MAP_OFFSET 30339
#define INTRO_MAP_HEIGHT 5
#define INTRO_MAP_WIDTH 19
#define INTRO_FIXUPDATA_OFFSET 29806

/**
 * The states of the intro.
 */
typedef enum {
    INTRO_MAP,                  /* displaying the animated intro map */
    INTRO_MENU,                 /* displaying the main menu: journey onward, etc. */
    INTRO_INIT_NAME,            /* prompting for character name */
    INTRO_INIT_SEX,             /* prompting for character sex */
    INTRO_INIT_STORY,           /* displaying the intro story leading up the gypsy */
    INTRO_INIT_QUESTIONS,       /* prompting for the questions that determine class */
    INTRO_INIT_SEGTOGAME        /* displaying the text that segues to the game */
} IntroMode;

#define GYP_PLACES_FIRST 0
#define GYP_PLACES_TWOMORE 1
#define GYP_PLACES_LAST 2
#define GYP_UPON_TABLE 3
#define GYP_SEGUE1 13
#define GYP_SEGUE2 14

int storyInd;
int segueInd;
unsigned char *introMap[INTRO_MAP_HEIGHT];
char *introText[24];
char *introQuestions[28];
char *introGypsy[15];
int questionRound;
int answerInd;
int introAskToggle = 0;
int questionTree[15];

IntroMode mode = INTRO_MAP;
char nameBuffer[16];
char sex;

void introInitiateNewGame();
void introStartQuestions();
int introHandleName(const char *message);
int introHandleSexChoice(char choice);
void introShowText(const char *text);
void introInitQuestionTree();
const char *introGetQuestion(int v1, int v2);
int introDoQuestion(int answer);
int introHandleQuestionChoice(char choice);

int introInit() {
    unsigned char screenFixData[533];
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
        if (buffer[j-1] == '\n')
            buffer[j-1] = '\0';
        introGypsy[i] = strdup(buffer);
    }

    fseek(title, INTRO_FIXUPDATA_OFFSET, SEEK_SET);
    fread(screenFixData, 1, sizeof(screenFixData), title);

    fseek(title, 30339, SEEK_SET);
    introMap[0] = (unsigned char *) malloc(INTRO_MAP_WIDTH * INTRO_MAP_HEIGHT);
    for (i = 0; i < INTRO_MAP_HEIGHT; i++) {
        introMap[i] = introMap[0] + INTRO_MAP_WIDTH * i;
        for (j = 0; j < INTRO_MAP_WIDTH; j++) {
            introMap[i][j] = (unsigned char) fgetc(title);
        }
    }

    u4fclose(title);

    screenLoadCards();
    screenFixIntroScreen(screenFixData);

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

    free(introMap[0]);

    screenFreeCards();
    screenFreeIntroBackgrounds();
}

int introKeyHandler(int key, void *data) {
    int valid = 1;
    GetChoiceActionInfo *info;

    switch (mode) {

    case INTRO_MAP:
        mode = INTRO_MENU;
        introUpdateScreen();
        return 1;

    case INTRO_MENU:
        switch (key) {
        case 'i':
            introInitiateNewGame();
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
        if (storyInd >= 24)
            introStartQuestions();
        introUpdateScreen();
        return 1;

    case INTRO_INIT_QUESTIONS:
        introAskToggle = 1;
        info = (GetChoiceActionInfo *) malloc(sizeof(GetChoiceActionInfo));
        info->choices = "ab";
        info->handleChoice = &introHandleQuestionChoice;
        eventHandlerPushKeyHandlerData(&keyHandlerGetChoice, info);
        introUpdateScreen();
        return 1;

    case INTRO_INIT_SEGTOGAME:
        segueInd++;
        if (segueInd >= 2)
            eventHandlerSetExitFlag(1);
        else
            introUpdateScreen();
        return 1;
    }


    return valid || keyHandlerDefault(key, NULL);
}

void introDrawMap() {
    int x, y;

    for (y = 0; y < INTRO_MAP_HEIGHT; y++) {
        for (x = 0; x < INTRO_MAP_WIDTH; x++) {
            screenShowTile(introMap[y][x], x, y + 6);
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
        introShowText(introText[storyInd]);
        break;

    case INTRO_INIT_QUESTIONS:
        if (introAskToggle == 0) {
            if (questionRound == 0)
                screenDrawBackground(BKGD_ABACUS);
            screenShowCard(0, questionTree[questionRound * 2]);
            screenShowCard(1, questionTree[questionRound * 2 + 1]);

            screenEraseIntroText();
        
            screenTextAt(0, 19, "%s", introGypsy[questionRound == 0 ? GYP_PLACES_FIRST : (questionRound == 6 ? GYP_PLACES_LAST : GYP_PLACES_TWOMORE)]);
            screenTextAt(0, 20, "%s", introGypsy[GYP_UPON_TABLE]);
            screenTextAt(0, 21, "%s and %s.  She says", introGypsy[questionTree[questionRound * 2] + 4], introGypsy[questionTree[questionRound * 2 + 1] + 4]);
            screenTextAt(0, 22, "\"Consider this:\"");
            screenSetCursorPos(16, 22);
        } else
            introShowText(introGetQuestion(questionTree[questionRound * 2], questionTree[questionRound * 2 + 1]));
        break;

    case INTRO_INIT_SEGTOGAME:
        introShowText(introGypsy[GYP_SEGUE1 + segueInd]);
        break;

    default:
        assert(0);
    }

    screenForceRedraw();
}

/**
 * Initiate a new savegame by reading the name, sex, then presenting a
 * series of questions to determine the class of the new character.
 */
void introInitiateNewGame() {
    ReadBufferActionInfo *info;

    /* display name  prompt and read name from keyboard */
    mode = INTRO_INIT_NAME;
    introUpdateScreen();
    screenSetCursorPos(12, 20);
    screenEnableCursor();
    screenForceRedraw();

    info = (ReadBufferActionInfo *) malloc(sizeof(ReadBufferActionInfo));
    info->handleBuffer = &introHandleName;
    info->buffer = nameBuffer;
    info->bufferLen = 16;
    info->screenX = 12;
    info->screenY = 20;
    nameBuffer[0] = '\0';

    eventHandlerPushKeyHandlerData(&keyHandlerReadBuffer, info);
}

void introStartQuestions() {
    mode = INTRO_INIT_QUESTIONS;
    questionRound = 0;
    introAskToggle = 0;
    introInitQuestionTree();
}

int introHandleName(const char *message) {
    GetChoiceActionInfo *info;

    eventHandlerPopKeyHandler();
    mode = INTRO_INIT_SEX;

    introUpdateScreen();
    screenSetCursorPos(29, 16);
    screenEnableCursor();
    screenForceRedraw();

    info = (GetChoiceActionInfo *) malloc(sizeof(GetChoiceActionInfo));
    info->choices = "mf";
    info->handleChoice = &introHandleSexChoice;
    eventHandlerPushKeyHandlerData(&keyHandlerGetChoice, info);

    return 1;
}

int introHandleSexChoice(char choice) {

    if (choice == 'm')
        sex = SEX_MALE;
    else
        sex = SEX_FEMALE;

    eventHandlerPopKeyHandler();
    mode = INTRO_INIT_STORY;
    storyInd = 23;

    introUpdateScreen();
    screenForceRedraw();

    return 1;
}

/**
 * Get the text for the question giving a choice between virtue v1 and
 * virtue v2 (zero based virtue index, starting at honesty).
 */
const char *introGetQuestion(int v1, int v2) {
    int i = 0;
    int d = 7;

    assert (v1 < v2);

    while (v1 > 0) {
        i += d;
        d--;
        v1--;
        v2--;
    }
        
    assert((i + v2 - 1) < 28);

    return introQuestions[i + v2 - 1];
}

void introShowText(const char *text) {
    char line[41];
    const char *p;
    int len, lineNo = 19;

    screenEraseIntroText();

    p = text;
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

int introHandleQuestionChoice(char choice) {
    FILE *saveGameFile;
    SaveGame saveGame;
    eventHandlerPopKeyHandler();

    if (introDoQuestion(choice == 'a' ? 0 : 1)) {
        mode = INTRO_INIT_SEGTOGAME;
        segueInd = 0;

        saveGameFile = fopen("party.sav", "wb");
        if (saveGameFile) {
            SaveGamePlayerRecord avatar;
            saveGamePlayerRecordInit(&avatar);
            strcpy(avatar.name, nameBuffer);
            avatar.hp = 100;
            avatar.hpMax = 100;
            avatar.xp = 0;
            avatar.str = 20;
            avatar.dex = 20;
            avatar.intel = 20;
            avatar.mp = 20;
            avatar.sex = sex;
            avatar.klass = questionTree[14];
            saveGameInit(&saveGame, 86, 109, &avatar);
            saveGameWrite(&saveGame, saveGameFile);
            fclose(saveGameFile);
        }
    } else {
        introAskToggle = 0;
    }

    introUpdateScreen();

    return 1;
}
