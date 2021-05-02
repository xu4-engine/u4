/*
 * xu4.h
 */

class Settings;
class Config;
class ImageMgr;
class Image;
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
    Config* config;
    ImageMgr* imageMgr;
    void* screen;
    void* gpu;
    Image* screenImage;
    EventHandler* eventHandler;
    IntroController* intro;
    GameController* game;
    int stage;
};

extern XU4GameServices xu4;
