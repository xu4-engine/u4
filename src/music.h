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
#define NLOOPS -1

#ifdef IOS
# if __OBJC__
@class U4AudioController;
# else
typedef void U4AudioController;
# endif
typedef U4AudioController OSMusicMixer;
#else // SDL
struct _Mix_Music;
typedef _Mix_Music OSMusicMixer;
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

    void init() {}
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


    /*
     * Static variables
     */
private:
	void create_sys();
	void destroy_sys();
	void setMusicVolume_sys(int volume);
	void setSoundVolume_sys(int volume);
	void fadeOut_sys(int msecs);
	void fadeIn_sys(int msecs, bool loadFromMap);
    bool isPlaying_sys();

    static Music *instance;
    static bool fading;
    static bool on;


	bool load_sys(const string &pathname);
    void playMid(Type music);
	void stopMid();
    bool load(Type music);

public:
    static bool functional;

    /*
     * Properties
     */
    std::vector<std::string> filenames;
    Type introMid;
	Type current;
    OSMusicMixer *playing;
    Debug *logger;
};






#endif
