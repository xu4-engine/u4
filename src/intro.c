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
#include "player.h"

#define INTRO_TEXT_OFFSET 17445
#define INTRO_MAP_OFFSET 30339
#define INTRO_MAP_HEIGHT 5
#define INTRO_MAP_WIDTH 19
#define INTRO_FIXUPDATA_OFFSET 29806
#define INTRO_SCRIPT_TABLE_SIZE 548
#define INTRO_SCRIPT_TABLE_OFFSET 30434
#define INTRO_BASETILE_TABLE_SIZE 15
#define INTRO_BASETILE_TABLE_OFFSET 16584
#define BEASTIE1_FRAMES 0x80
#define BEASTIE2_FRAMES 0x40
#define BEASTIE_FRAME_TABLE_OFFSET 0x7380
#define BEASTIE1_FRAMES_OFFSET 0
#define BEASTIE2_FRAMES_OFFSET 0x78

#define INTRO_TEXT_X 0
#define INTRO_TEXT_Y 19
#define INTRO_TEXT_WIDTH 40
#define INTRO_TEXT_HEIGHT 6

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
    INTRO_INIT_SEGTOGAME,       /* displaying the text that segues to the game */
    INTRO_DONE
} IntroMode;

typedef struct _IntroObjectState {
    int x, y;
    unsigned char tile;  /* base tile + tile frame */
} IntroObjectState;

#define GYP_PLACES_FIRST 0
#define GYP_PLACES_TWOMORE 1
#define GYP_PLACES_LAST 2
#define GYP_UPON_TABLE 3
#define GYP_SEGUE1 13
#define GYP_SEGUE2 14

extern int quit;

/* introduction state */
IntroMode mode;

/* data loaded in from title.exe */
unsigned char *introMap[INTRO_MAP_HEIGHT];
char **introText;
char **introQuestions;
char **introGypsy;
unsigned char *scriptTable;
unsigned char *baseTileTable;
unsigned char *beastie1FrameTable;
unsigned char *beastie2FrameTable;
char *introErrorMessage;

/* additional introduction state data */
char nameBuffer[16];
SexType sex;
int storyInd;
int segueInd;
int answerInd;
int questionRound;
int introAskToggle;
int questionTree[15];
int beastie1Cycle;
int beastie2Cycle;
int sleepCycles;
int scrPos;  /* current position in the script table */
IntroObjectState *objectStateTable;

void introInitiateNewGame(void);
void introDrawMap(void);
void introDrawMapAnimated(void);
void introDrawBeasties(void);
void introStartQuestions(void);
int introHandleName(const char *message);
int introHandleSexChoice(char choice);
void introJourneyOnward();
void introShowText(const char *text);
void introInitQuestionTree(void);
const char *introGetQuestion(int v1, int v2);
int introDoQuestion(int answer);
int introHandleQuestionChoice(char choice);
void introInitPlayers(SaveGame *saveGame);

/**
 * Initializes intro state and loads in introduction graphics, text
 * and map data from title.exe.
 */
int introInit() {
    unsigned char screenFixData[533];
    FILE *title;
    int i, j;

    mode = INTRO_MAP;
    introAskToggle = 0;
    beastie1Cycle = 0;
    beastie2Cycle = 0;
    introErrorMessage = NULL;

    title = u4fopen("title.exe");
    if (!title)
        return 0;

    introQuestions = u4read_stringtable(title, INTRO_TEXT_OFFSET, 28);
    introText = u4read_stringtable(title, -1, 24);
    introGypsy = u4read_stringtable(title, -1, 15);

    fseek(title, INTRO_FIXUPDATA_OFFSET, SEEK_SET);
    fread(screenFixData, 1, sizeof(screenFixData), title);

    fseek(title, INTRO_MAP_OFFSET, SEEK_SET);
    introMap[0] = (unsigned char *) malloc(INTRO_MAP_WIDTH * INTRO_MAP_HEIGHT);
    for (i = 0; i < INTRO_MAP_HEIGHT; i++) {
        introMap[i] = introMap[0] + INTRO_MAP_WIDTH * i;
        for (j = 0; j < INTRO_MAP_WIDTH; j++) {
            introMap[i][j] = (unsigned char) fgetc(title);
        }
    }

    sleepCycles = 0;
    scrPos = 0;

    fseek(title, INTRO_SCRIPT_TABLE_OFFSET, SEEK_SET);
    scriptTable = (unsigned char *) malloc(INTRO_SCRIPT_TABLE_SIZE);
    for (i = 0; i < INTRO_SCRIPT_TABLE_SIZE; i++)
        scriptTable[i] = fgetc(title);

    fseek(title, INTRO_BASETILE_TABLE_OFFSET, SEEK_SET);
    baseTileTable = (unsigned char *) malloc(INTRO_BASETILE_TABLE_SIZE);
    for (i = 0; i < INTRO_BASETILE_TABLE_SIZE; i++)
        baseTileTable[i] = fgetc(title);

    objectStateTable = (IntroObjectState *) malloc(sizeof(IntroObjectState) * INTRO_BASETILE_TABLE_SIZE);
    memset(objectStateTable, 0, sizeof(IntroObjectState) * INTRO_BASETILE_TABLE_SIZE);

    /* --------------------------
       load beastie frame table 1
       -------------------------- */
    beastie1FrameTable = (unsigned char *) malloc(sizeof(unsigned char) * BEASTIE1_FRAMES);
    if (!beastie1FrameTable) {
        u4fclose(title);
        return(0);
    }
    fseek(title, BEASTIE_FRAME_TABLE_OFFSET + BEASTIE1_FRAMES_OFFSET, SEEK_SET);
    for (i = 0; i < BEASTIE1_FRAMES; i++) {
        beastie1FrameTable[i] = fgetc(title);
    }

    /* --------------------------
       load beastie frame table 2
       -------------------------- */
    beastie2FrameTable = (unsigned char *) malloc(sizeof(unsigned char) * BEASTIE2_FRAMES);
    if (!beastie2FrameTable) {
        u4fclose(title);
        return(0);
    }
    fseek(title, BEASTIE_FRAME_TABLE_OFFSET + BEASTIE2_FRAMES_OFFSET, SEEK_SET);
    for (i = 0; i < BEASTIE2_FRAMES; i++) {
        beastie2FrameTable[i] = fgetc(title);
    }

    u4fclose(title);

    screenLoadIntroAnimations();
    screenFixIntroScreen(screenFixData);

    introUpdateScreen();

    return 1;
}

/**
 * Frees up data not needed after introduction.
 */
void introDelete() {
    int i;

    for (i = 0; i < 28; i++)
        free(introQuestions[i]);
    free(introQuestions);
    introQuestions = NULL;
    for (i = 0; i < 24; i++)
        free(introText[i]);
    free(introText);
    introText = NULL;
    for (i = 0; i < 15; i++)
        free(introGypsy[i]);
    free(introGypsy);
    introGypsy = NULL;

    free(introMap[0]);

    free(scriptTable);
    scriptTable = NULL;
    free(baseTileTable);
    baseTileTable = NULL;
    free(objectStateTable);
    objectStateTable = NULL;

    free(beastie1FrameTable);
    free(beastie2FrameTable);

    screenFreeIntroAnimations();
    screenFreeIntroBackgrounds();
}

/**
 * Handles keystrokes during the introduction.
 */
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
            introErrorMessage = NULL;
            introInitiateNewGame();
            break;
        case 'j':
            introJourneyOnward();
            break;
        case 'r':
            introErrorMessage = NULL;
            mode = INTRO_MAP;
            introUpdateScreen();
            break;
        case 'q':
            quit = 1;
            eventHandlerSetExitFlag(1);
            break;
        default:
            valid = 0;
            break;
        }
        break;

    case INTRO_INIT_NAME:
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
        if (segueInd >= 2) {
            mode = INTRO_DONE;
            eventHandlerSetExitFlag(1);
        }
        else
            introUpdateScreen();
        return 1;

    case INTRO_DONE:
        return 0;
    }

    return valid || keyHandlerDefault(key, NULL);
}

/**
 * Draws the small map on the intro screen.
 */
void introDrawMap() {
    if (sleepCycles > 0) {
        introDrawMapAnimated();
        sleepCycles--;
    }
    else {
        unsigned char commandNibble;
        unsigned char dataNibble;

        do {
            commandNibble = scriptTable[scrPos] >> 4;

            switch(commandNibble) {
                /* 0-4 = set object position and tile frame */
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
                /* ----------------------------------------------------------
                   Set object position and tile frame
                   Format: yi [t(3); x(5)]
                   i = table index
                   x = x coordinate (5 least significant bits of second byte)
                   y = y coordinate
                   t = tile frame (3 most significant bits of second byte)
                   ---------------------------------------------------------- */
                dataNibble = scriptTable[scrPos] & 0xf;
                objectStateTable[dataNibble].x = scriptTable[scrPos+1] & 0x1f;
                objectStateTable[dataNibble].y = commandNibble;
                objectStateTable[dataNibble].tile = baseTileTable[dataNibble] + (scriptTable[scrPos+1] >> 5);
                scrPos += 2;
                break;
            case 7:
                /* ---------------
                   Delete object
                   Format: 7i
                   i = table index
                   --------------- */
                dataNibble = scriptTable[scrPos] & 0xf;
                objectStateTable[dataNibble].tile = 0;
                scrPos++;
                break;
            case 8:
                /* ----------------------------------------------
                   Redraw intro map and objects, then go to sleep
                   Format: 8c
                   c = cycles to sleep
                   ---------------------------------------------- */
                introDrawMapAnimated();

                /* set sleep cycles */
                sleepCycles = scriptTable[scrPos] & 0xf;
                scrPos++;
                break;
            case 0xf:
                /* -------------------------------------
                   Jump to the start of the script table
                   Format: f?
                   ? = doesn't matter
                   ------------------------------------- */
                scrPos = 0;
                break;
            default:
                /* invalid command */
                scrPos++;
                break;
            }

        } while (commandNibble != 8);
    }
}

void introDrawMapAnimated() {
    ScreenTileInfo tileInfo;
    int x, y, i;

    tileInfo.hasFocus = 0;

    /* draw unmodified map */
    for (y = 0; y < INTRO_MAP_HEIGHT; y++) {
        for (x = 0; x < INTRO_MAP_WIDTH; x++) {
            tileInfo.tile = introMap[y][x];
            screenShowTile(&tileInfo, x, y + 6);
        }
    }

    /* draw animated objects */
    for (i = 0; i < INTRO_BASETILE_TABLE_SIZE; i++) {
        if (objectStateTable[i].tile != 0) {
            tileInfo.tile = objectStateTable[i].tile;
            screenShowTile(&tileInfo, objectStateTable[i].x, objectStateTable[i].y + 6);
        }
    }
}

/**
 * Draws the animated beasts in the upper corners of the screen.
 */
void introDrawBeasties() {
    screenShowBeastie(0, beastie1FrameTable[beastie1Cycle]);
    screenShowBeastie(1, beastie2FrameTable[beastie2Cycle]);
}

/**
 * Paints the screen.
 */
void introUpdateScreen() {
    switch (mode) {
    case INTRO_MAP:
        screenDrawBackground(BKGD_INTRO);
        introDrawMap();
        introDrawBeasties();
        break;

    case INTRO_MENU:
        screenDrawBackground(BKGD_INTRO);
        screenTextAt(2, 14, "In another world, in a time to come.");
        screenTextAt(15, 16, "Options:");
        screenTextAt(11, 17, "Return to the view");
        screenTextAt(11, 18, "Journey Onward");
        screenTextAt(11, 19, "Initiate New Game");
        if (introErrorMessage)
            screenTextAt(11, 21, introErrorMessage);
        else {
            screenTextAt(3, 21, "xu4 is free software, see COPYING");
            screenTextAt(5, 22, "\011 Copyright 1987 Lord British");
        }
        introDrawBeasties();
        break;

    case INTRO_INIT_NAME:
        screenDrawBackground(BKGD_INTRO);
        screenTextAt(4, 16, "By what name shalt thou be known");
        screenTextAt(4, 17, "in this world and time?");
        introDrawBeasties();
        break;

    case INTRO_INIT_SEX:
        screenDrawBackground(BKGD_INTRO);
        screenTextAt(4, 16, "Art thou Male or Female?");
        introDrawBeasties();
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

            screenEraseTextArea(INTRO_TEXT_X, INTRO_TEXT_Y, INTRO_TEXT_WIDTH, INTRO_TEXT_HEIGHT);
        
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

    case INTRO_DONE:
        break;

    default:
        assert(0);
    }

    screenRedrawScreen();
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
    screenRedrawScreen();

    info = (ReadBufferActionInfo *) malloc(sizeof(ReadBufferActionInfo));
    info->handleBuffer = &introHandleName;
    info->buffer = nameBuffer;
    info->bufferLen = 16;
    info->screenX = 12;
    info->screenY = 20;
    nameBuffer[0] = '\0';

    eventHandlerPushKeyHandlerData(&keyHandlerReadBuffer, info);
}

/**
 * Starts the gypsys questioning that eventually determines the new
 * characters class.
 */
void introStartQuestions() {
    mode = INTRO_INIT_QUESTIONS;
    questionRound = 0;
    introAskToggle = 0;
    introInitQuestionTree();
}

/**
 * Callback to receive the read character name.
 */
int introHandleName(const char *message) {
    GetChoiceActionInfo *info;

    eventHandlerPopKeyHandler();

    if (message[0] == '\0') {
        mode = INTRO_MENU;

        introUpdateScreen();
        screenDisableCursor();
    } 

    else {
        mode = INTRO_INIT_SEX;

        introUpdateScreen();
        screenSetCursorPos(29, 16);
        screenEnableCursor();

        info = (GetChoiceActionInfo *) malloc(sizeof(GetChoiceActionInfo));
        info->choices = "mf";
        info->handleChoice = &introHandleSexChoice;
        eventHandlerPushKeyHandlerData(&keyHandlerGetChoice, info);
    }

    screenRedrawScreen();

    return 1;
}

/**
 * Callback to receive the character sex choice.
 */
int introHandleSexChoice(char choice) {

    if (choice == 'm')
        sex = SEX_MALE;
    else
        sex = SEX_FEMALE;

    eventHandlerPopKeyHandler();
    mode = INTRO_INIT_STORY;
    storyInd = 0;

    introUpdateScreen();
    screenRedrawScreen();

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

/**
 * Starts the game.
 */
void introJourneyOnward() {
    FILE *saveGameFile;

    saveGameFile = fopen("party.sav", "rb");
    if (!saveGameFile) {
        introErrorMessage = "Initiate game first!";
        introUpdateScreen();
        screenRedrawScreen();
        return;
    }

    fclose(saveGameFile);
    eventHandlerSetExitFlag(1);
}

/**
 * Shows text in the lower six lines of the screen.
 */
void introShowText(const char *text) {
    char line[INTRO_TEXT_WIDTH + 1];
    const char *p;
    int len, lineNo = INTRO_TEXT_Y;

    screenEraseTextArea(INTRO_TEXT_X, INTRO_TEXT_Y, INTRO_TEXT_WIDTH, INTRO_TEXT_HEIGHT);

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

/**
 * Timer callback for the intro sequence.  Handles animating the intro
 * map, the beasties, etc..
 */
void introTimer() {
    screenCycle();
    screenUpdateCursor();
    if (mode == INTRO_MAP)
        introDrawMap();
    if (mode == INTRO_MAP || mode == INTRO_MENU || 
        mode == INTRO_INIT_NAME || mode == INTRO_INIT_SEX)
        introDrawBeasties();
    screenRedrawScreen();

    if ((rand() % 2) && ++beastie1Cycle >= BEASTIE1_FRAMES)
        beastie1Cycle = 0;
    if ((rand() % 2) && ++beastie2Cycle >= BEASTIE2_FRAMES)
        beastie2Cycle = 0;
}

/**
 * Initializes the question tree.  The tree starts off with the first
 * eight entries set to the numbers 0-7 in a random order.
 */
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

/**
 * Updates the question tree with the given answer, and advances to
 * the next round.
 */
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

/**
 * Callback to receive question answer choice.
 */
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
            saveGameInit(&saveGame, &avatar);
            introInitPlayers(&saveGame);
            saveGame.food = 30000;
            saveGame.gold = 200;
            saveGame.reagents[REAG_GINSENG] = 3;
            saveGame.reagents[REAG_GARLIC] = 4;
            saveGame.torches = 2;
            saveGameWrite(&saveGame, saveGameFile);
            fclose(saveGameFile);
        }
    } else {
        introAskToggle = 0;
    }

    introUpdateScreen();

    return 1;
}

/**
 * Build the initial avatar player record from the answers to the
 * gypsy's questions.
 */
void introInitPlayers(SaveGame *saveGame) {
    int i, p;
    static const struct {
        WeaponType weapon;
        ArmorType armor;
        int xp, x, y;
    } initValuesForClass[] = {
        { WEAP_STAFF,  ARMR_CLOTH,   125, 231, 136 }, /* CLASS_MAGE */
        { WEAP_SLING,  ARMR_CLOTH,   240,  83, 105 }, /* CLASS_BARD */
        { WEAP_AXE,    ARMR_LEATHER, 205,  35, 221 }, /* CLASS_FIGHTER */
        { WEAP_DAGGER, ARMR_CLOTH,   175,  59,  44 }, /* CLASS_DRUID */
        { WEAP_MACE,   ARMR_LEATHER, 110, 158,  21 }, /* CLASS_TINKER */
        { WEAP_SWORD,  ARMR_CHAIN,   325, 105, 183 }, /* CLASS_PALADIN */
        { WEAP_SWORD,  ARMR_LEATHER, 150,  23, 129 }, /* CLASS_RANGER */
        { WEAP_STAFF,  ARMR_CLOTH,     5, 186, 171 }  /* CLASS_SHEPHERD */
    };
    static const struct {
        const char *name;
        int str, dex, intel;
        SexType sex;
    } initValuesForNpcClass[] = {
        { "Mariah",    9, 12, 20, SEX_FEMALE }, /* CLASS_MAGE */
        { "Iolo",     16, 19, 13, SEX_MALE },   /* CLASS_BARD */
        { "Geoffrey", 20, 15, 11, SEX_MALE },   /* CLASS_FIGHTER */
        { "Jaana",    17, 16, 13, SEX_FEMALE }, /* CLASS_DRUID */
        { "Julia",    15, 16, 12, SEX_FEMALE }, /* CLASS_TINKER */
        { "Dupre",    17, 14, 17, SEX_MALE },   /* CLASS_PALADIN */
        { "Shamino",  16, 15, 15, SEX_MALE },   /* CLASS_RANGER */
        { "Katrina",  11, 12, 10, SEX_FEMALE }  /* CLASS_SHEPHERD */
    };

    strcpy(saveGame->players[0].name, nameBuffer);
    saveGame->players[0].sex = sex;
    saveGame->players[0].klass = (ClassType) questionTree[14];

    assert(saveGame->players[0].klass < 8);
    
    saveGame->players[0].weapon = initValuesForClass[saveGame->players[0].klass].weapon;
    saveGame->players[0].armor = initValuesForClass[saveGame->players[0].klass].armor;
    saveGame->players[0].xp = initValuesForClass[saveGame->players[0].klass].xp;
    saveGame->x = initValuesForClass[saveGame->players[0].klass].x;
    saveGame->y = initValuesForClass[saveGame->players[0].klass].y;
    
    saveGame->players[0].str = 15;
    saveGame->players[0].dex = 15;
    saveGame->players[0].intel = 15;

    for (i = 8; i < 15; i++) {
        switch (questionTree[i]) {
        case VIRT_HONESTY:
            saveGame->players[0].intel += 3;
            break;
        case VIRT_COMPASSION:
            saveGame->players[0].dex += 3;
            break;
        case VIRT_VALOR:
            saveGame->players[0].str += 3;
            break;
        case VIRT_JUSTICE:
            saveGame->players[0].intel++;
            saveGame->players[0].dex++;
            break;
        case VIRT_SACRIFICE:
            saveGame->players[0].intel++;
            saveGame->players[0].str++;
            break;
        case VIRT_HONOR:
            saveGame->players[0].dex++;
            saveGame->players[0].str++;
            break;
        case VIRT_SPIRITUALITY:
            saveGame->players[0].intel++;
            saveGame->players[0].dex++;
            saveGame->players[0].str++;
            break;
        case VIRT_HUMILITY:
            /* no stats for you! */
            break;
        }
    }

    saveGame->players[0].hp = saveGame->players[0].hpMax = playerGetMaxLevel(&saveGame->players[0]) * 100;
    saveGame->players[0].mp = playerGetMaxMp(&saveGame->players[0]);

    p = 1;
    for (i = 0; i < VIRT_MAX; i++) {
        if (i != saveGame->players[0].klass) {
            saveGame->players[p].klass = (ClassType) i;
            saveGame->players[p].xp = initValuesForClass[i].xp;
            saveGame->players[p].str = initValuesForNpcClass[i].str;
            saveGame->players[p].dex = initValuesForNpcClass[i].dex;
            saveGame->players[p].intel = initValuesForNpcClass[i].intel;
            saveGame->players[p].weapon = initValuesForClass[i].weapon;
            saveGame->players[p].armor = initValuesForClass[i].armor;
            strcpy(saveGame->players[p].name, initValuesForNpcClass[i].name);
            saveGame->players[p].sex = initValuesForNpcClass[i].sex;
            saveGame->players[p].hp = saveGame->players[p].hpMax = playerGetMaxLevel(&saveGame->players[p]) * 100;
            saveGame->players[p].mp = playerGetMaxMp(&saveGame->players[p]);
            p++;
        }
    }

}
