/*
 * $Id$
 */

#ifndef INTRO_H
#define INTRO_H

#include <vector>

#include "controller.h"
#include "menu.h"
#include "savegame.h"

class IntroObjectState;

class IntroController : public Controller {
public:
    IntroController();

    int init();
    void deleteIntro();
    bool keyPressed(int key);
    unsigned char *getSigData();
    void updateScreen();
    void timerFired();

private:
    void drawMap();
    void drawMapAnimated();
    void drawBeasties();

    void initQuestionTree();
    int doQuestion(int answer);
    void initPlayers(SaveGame *saveGame);
    std::string getQuestion(int v1, int v2);

    void initiateNewGame();
    void startQuestions();
    void showStory();
    void journeyOnward();
    void about();

    void showText(const string &text);

    int baseMenuKeyHandler(int key, void *data);

    static void introMainOptionsMenuItemActivate(MenuItem *menuItem, ActivateAction action);
    static void introVideoOptionsMenuItemActivate(MenuItem *menuItem, ActivateAction action);
    static void introSoundOptionsMenuItemActivate(MenuItem *menuItem, ActivateAction action);
    static void introGameplayOptionsMenuItemActivate(MenuItem *menuItem, ActivateAction action);
    static void introAdvancedOptionsMenuItemActivate(MenuItem *menuItem, ActivateAction action);
    static void introKeyboardOptionsMenuItemActivate(MenuItem *menuItem, ActivateAction action);
    static void introSpeedOptionsMenuItemActivate(MenuItem *menuItem, ActivateAction action);
    static void introEnhancementOptionsMenuItemActivate(MenuItem *menuItem, ActivateAction action);

    /**
     * The states of the intro.
     */
    enum {
        INTRO_MAP,                          /* displaying the animated intro map */
        INTRO_MENU,                         /* displaying the main menu: journey onward, etc. */
        INTRO_CONFIG,                       /* the configuration screen */
        INTRO_CONFIG_VIDEO,                 /* video configuration */
        INTRO_CONFIG_SOUND,                 /* sound configuration */
        INTRO_CONFIG_GAMEPLAY,              /* gameplay configuration */
        INTRO_CONFIG_ADVANCED,              /* advanced gameplay config */
        INTRO_CONFIG_KEYBOARD,              /* keyboard config */
        INTRO_CONFIG_SPEED,                 /* speed config */
        INTRO_CONFIG_ENHANCEMENT_OPTIONS,   /* game enhancement options */    
        INTRO_ABOUT,                        /* about xu4 screen */
        INTRO_INIT,                         /* prompting for character name and sex */
        INTRO_INIT_STORY                    /* displaying the intro story leading up the gypsy */
    } mode;

    /* data loaded in from title.exe */
    unsigned char **introMap;
    unsigned char *sigData;
    unsigned char *scriptTable;
    unsigned char *baseTileTable;
    unsigned char *beastie1FrameTable;
    unsigned char *beastie2FrameTable;
    std::vector<std::string> introText;
    std::vector<std::string> introQuestions;
    std::vector<std::string> introGypsy;

    /* additional introduction state data */
    std::string errorMessage;
    std::string nameBuffer;
    SexType sex;
    int answerInd;
    int questionRound;
    int questionTree[15];
    int beastie1Cycle;
    int beastie2Cycle;
    int beastieOffset;
    int sleepCycles;
    int scrPos;  /* current position in the script table */
    IntroObjectState *objectStateTable;
};

extern IntroController *intro;

#endif
