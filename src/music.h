/*
 * $Id$
 */

#ifndef MUSIC_H
#define MUSIC_H

#include <string>
#include <vector>

#include "debug.h"

#define musicMgr   (Music::GET_MUSIC_INSTANCE())

#define CAMP_FADE_OUT_TIME          1000
#define CAMP_FADE_IN_TIME           0
#define INN_FADE_OUT_TIME           1000
#define INN_FADE_IN_TIME            5000
#define NLOOPS -1

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
    virtual bool isActuallyPlaying();


    void init() {};
    virtual void play(){};
    virtual void stop(){};
    virtual void fadeOut(int msecs){};
    virtual void fadeIn(int msecs, bool loadFromMap){};
    void lordBritish();
    void hawkwind();
    void camp();
    void shopping();
    void intro();
    void introSwitch(int n);
    bool toggle();

    virtual int decreaseMusicVolume();
    virtual int increaseMusicVolume();
    virtual void setMusicVolume(int volume){};
    int decreaseSoundVolume();
    int increaseSoundVolume();
    virtual void setSoundVolume(int volume){};


    /*
     * Static variables
     */
protected:
    static Music *instance;
    bool fading;
    bool on;


    virtual bool doLoad(Type music, string pathname, Type & current){return false;};
    virtual void playMid(Type music){};
    bool load(Type music);

public:
    bool functional;

    /*
     * Properties
     */

    std::vector<std::string> filenames;
    Type introMid;    

    Mix_Music* playing;
    Debug *logger;

    static Music * (*GET_MUSIC_INSTANCE)(void);
};
;





#endif
