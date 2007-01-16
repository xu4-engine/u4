/*
 * $Id$
 */

#ifndef MUSIC_H
#define MUSIC_H

#include <string>
#include <vector>

#include "debug.h"

#define musicMgr   (Music::getInstance())

#define CAMP_FADE_OUT_TIME          1000
#define CAMP_FADE_IN_TIME           0
#define INN_FADE_OUT_TIME           1000
#define INN_FADE_IN_TIME            5000

#if !defined(_MIXER_H_) && !defined(_SDL_MIXER_H)
    struct Mix_Music { int dummy; };
#endif 

class Music {
public:
    enum Type {
        NONE,
        OUTSIDE,
        TOWNS,
        SHRINES,
        SHOPPING,
        RULEBRIT,
        FANFARE,
        DUNGEON,
        COMBAT,
        CASTLES,
        MAX
    };

    Music();
    ~Music();

    static Music *getInstance();    
    static void callback(void *);    
    static bool isPlaying();

    void init() {};
    void play();
    void stop();
    void fadeOut(int msecs);
    void fadeIn(int msecs, bool loadFromMap);
    void lordBritish();
    void hawkwind();
    void camp();
    void shopping();
    void intro();
    void introSwitch(int n);
    bool toggle();

    int decreaseMusicVolume();
    int increaseMusicVolume();
    void setMusicVolume(int volume);

    int decreaseSoundVolume();
    int increaseSoundVolume();
    void setSoundVolume(int volume);

private:
    void playMid(Type music);
    bool load(Type music);

    /*
     * Static variables
     */
private:
    static Music *instance;
    static bool fading;
    static bool on;

public:
    static bool functional;

    /*
     * Properties
     */

    std::vector<std::string> filenames;
    Type introMid;    

    Mix_Music* playing;
    Debug *logger;
};

#endif
