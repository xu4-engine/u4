/*
 * $Id$
 */

#ifndef INTRO_H
#define INTRO_H

#include <string>
#include <vector>

#include "controller.h"
#include "menu.h"
#include "observer.h"
#include "savegame.h"
#include "textview.h"
#include "tileview.h"

struct IntroObjectState;
class Tile;

/**
 * Controls the intro sequence, including the traditional animated map
 * and "Journey Onward" menu, plus the xu4 specific configuration
 * menu.
 * @todo
 * <ul>
 *      <li>menus need id's instead of magic numbers</li>
 *      <li>make initial menu a Menu too</li>
 *      <li>get rid of mode and switch(mode) statements</li>
 *      <li>get rid global intro instance -- should only need to be accessed from u4.cpp</li>
 * </ul>
 */
class IntroController : public Controller, public Observer<Menu *, MenuEvent &> {
public:
    IntroController();

    bool init();
    void deleteIntro();
    bool keyPressed(int key);
    unsigned char *getSigData();
    void updateScreen();
    void timerFired();

    void update(Menu *menu, MenuEvent &event);
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
    bool doQuestion(int answer);
    void initPlayers(SaveGame *saveGame);
    std::string getQuestion(int v1, int v2);

    void initiateNewGame();
    void startQuestions();
    void showStory();
    void journeyOnward();
    void about();

    void showText(const string &text);

    void runMenu(Menu *menu, TextView *view, bool withBeasties);

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

    enum MenuConstants {
        USE_SETTINGS = 0xFE,
        CANCEL = 0xFF
    };

    TextView menuArea;
    TextView extendedMenuArea;
    TextView questionArea;
    TileView mapArea;

    /* data loaded in from title.exe */
    MapTile *introMap;
    unsigned char *sigData;
    unsigned char *scriptTable;
    Tile **baseTileTable;
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
