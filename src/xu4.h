/*
 * xu4.h
 */

class Settings;
class EventHandler;
class IntroController;
class GameController;

enum XU4GameStage {
    StageExitGame,
    StageIntro,
    StagePlay
};

struct XU4GameServices {
    Settings* settings;
    EventHandler* eventHandler;
    IntroController* intro;
    GameController* game;
    int stage;
};

extern XU4GameServices xu4;
