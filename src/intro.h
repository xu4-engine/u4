/*
 * $Id$
 */

#ifndef INTRO_H
#define INTRO_H

#include <vector>

#include "controller.h"
#include "menu.h"
#include "observer.h"
#include "savegame.h"

struct IntroObjectState;

class IntroController : public Controller, public Observer<MenuEvent &> {
public:
    IntroController();

    int init();
    void deleteIntro();
    bool keyPressed(int key);
    unsigned char *getSigData();
    void updateScreen();
    void timerFired();

    void update(Observable<MenuEvent &> *o, MenuEvent &event);
    void updateMainOptions(MenuEvent &event);
    void updateVideoOptions(MenuEvent &event);
    void updateSoundOptions(MenuEvent &event);
    void updateGameplayOptions(MenuEvent &event);
    void updateAdvancedOptions(MenuEvent &event);
    void updateEnhancementOptions(MenuEvent &event);
    void updateKeyboardOptions(MenuEvent &event);
    void updateSpeedOptions(MenuEvent &event);

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

    void runMenu(Menu *menu, bool withBeasties);

    /**
     * The states of the intro.
     */
    enum {
        INTRO_MAP,                          /* displaying the animated intro map */
        INTRO_MENU,                         /* displaying the main menu: journey onward, etc. */
        INTRO_CONFIG,                       /* the configuration screens */
        INTRO_ABOUT,                        /* about xu4 screen */
        INTRO_INIT,                         /* initializing a new game */
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
    bool beastiesVisible;
    int sleepCycles;
    int scrPos;  /* current position in the script table */
    IntroObjectState *objectStateTable;
};

extern IntroController *intro;

#endif
