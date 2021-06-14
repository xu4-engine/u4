/*
 * xu4.h
 */

class Settings;
class Config;
class ImageMgr;
struct Screen;
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
    Screen* screen;
    void* screenSys;
    void* gpu;
    Image* screenImage;
    EventHandler* eventHandler;
    IntroController* intro;
    GameController* game;
    int stage;
};

extern XU4GameServices xu4;
