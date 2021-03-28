/*
 * xu4.h
 */

class Settings;
class Config;
class EventHandler;
class CreatureMgr;
class IntroController;
class GameController;

enum XU4GameStage {
    StageExitGame,
    StageIntro,
    StagePlay
};

struct XU4GameServices {
    Settings* settings;
    Config* config;
    EventHandler* eventHandler;
    CreatureMgr* creatureMgr;
    IntroController* intro;
    GameController* game;
    int stage;
};

extern XU4GameServices xu4;
