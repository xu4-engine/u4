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

    static void callback(void *);    

    void stop()         {on = false; stopMid();} /**< Stop playing music */
    void fadeOut(int msecs);
    void fadeIn(int msecs, bool loadFromMap);
    bool toggle();
    bool isPlaying();
    void setMusicVolume(int volume);
    void setSoundVolume(int volume);
    void playMid(int music);
    void stopMid();

private:
    void create_sys();
    void destroy_sys();
    void fadeOut_sys(int msecs);
    void fadeIn_sys(int msecs, bool loadFromMap);
    bool load_sys(const string &pathname);
    bool load(int music);

    static bool fading;
    static bool on;

public:
    static bool functional;

    /*
     * Properties
     */
    std::vector<std::string> filenames;
    int current;
    OSMusicMixer *playing;
    Debug *logger;
};

extern Music* musicMgr;

#endif
