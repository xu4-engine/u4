/*
 * $Id$
 */

#ifndef MUSIC_H
#define MUSIC_H

#include <string>
#include <vector>

#include "debug.h"

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
    Music();
    ~Music();

    /** Returns true if the mixer is playing any audio. */
    static void callback(void *);    

    void play();
    void stop()         {on = false; stopMid();} /**< Stop playing music */
    void fadeOut(int msecs);
    void fadeIn(int msecs, bool loadFromMap);
    bool toggle();
    bool isPlaying() {return isPlaying_sys();}

    int decreaseMusicVolume();
    int increaseMusicVolume();
    void setMusicVolume(int volume) {setMusicVolume_sys(volume);}
    int decreaseSoundVolume();
    int increaseSoundVolume();
    void setSoundVolume(int volume) {setSoundVolume_sys(volume);}

    void playMid(int music);
    void stopMid();

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
    bool load(int music);

public:
    static bool functional;

    /*
     * Properties
     */
    std::vector<std::string> filenames;
    int introMid;
    int current;
    OSMusicMixer *playing;
    Debug *logger;
};

extern Music* musicMgr;

#endif
