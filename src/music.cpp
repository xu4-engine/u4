/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

/* FIXME: should this file have all SDL-related stuff extracted and put in music_sdl.c? */
// Yes! :)


#include <memory>
#include <string>
#include <vector>

#include "music.h"
#include "sound.h"

#include "config.h"
#include "context.h"
#include "debug.h"
#include "error.h"
#include "event.h"
#include "location.h"
#include "settings.h"
#include "u4.h"
#include "u4file.h"

using std::string;
using std::vector;

/*
 * Static variables
 */
Music *Music::instance = NULL;
Music * (*Music::GET_MUSIC_INSTANCE)(void) = &Music::getInstance;

/**
 * Returns an instance of the Music class
 */
Music *Music::getInstance() {
    if (!instance)
        instance = new Music();
    return instance;
}


/*
 * Constructors/Destructors
 */

/**
 * Initiliaze the music
 */
Music::Music() : introMid(TOWNS), playing(NULL), logger(new Debug("debug/music.txt", "Music")) {
    filenames.push_back("");    // filename for MUSIC_NONE;

    TRACE(*logger, "Initializing music");

    /*
     * load music track filenames from xml config file
     */
    const Config *config = Config::getInstance();

    TRACE_LOCAL(*logger, "Loading music tracks");

    vector<ConfigElement> musicConfs = config->getElement("music").getChildren();
    for (std::vector<ConfigElement>::iterator i = musicConfs.begin(); i != musicConfs.end(); i++) {

        if (i->getName() != "track")
            continue;

        filenames.push_back(i->getString("file"));
        TRACE_LOCAL(*logger, string("\tTrack file: ") + filenames.back());

    }
    filenames.resize(MAX, "");


    TRACE(*logger, string("Music initialized: volume is ") + (on ? "on" : "off"));
}

/**
 * Stop playing the music and cleanup
 */
Music::~Music() {
    TRACE(*logger, "Uninitializing music");
    eventHandler->getTimer()->remove(&Music::callback);


    TRACE(*logger, "Music uninitialized");
    delete logger;
}


bool Music::load(Type music) {
    static Type current = NONE;

    ASSERT(music < MAX, "Attempted to load an invalid piece of music in Music::load()");

    /* music already loaded */
    if (music == current) {
        /* tell calling function it didn't load correctly (because it's already playing) */
        if (isPlaying())
            return false;
        /* it loaded correctly */
        else 
            return true;
    }

    string pathname(u4find_music(filenames[music]));
    if (!pathname.empty()) {
    	return doLoad(music, pathname, current);
    }
    else return false;
}

/**
 * Ensures that the music is playing if it is supposed to be, or off
 * if it is supposed to be turned off.
 */
void Music::callback(void *data) {    
    eventHandler->getTimer()->remove(&Music::callback);

    if (musicMgr->on && !isPlaying())
        musicMgr->play();
    else if (!musicMgr->on && isPlaying())
        musicMgr->stop();
}
    
/**
 * Returns true if the mixer is playing any audio
 */
bool Music::isPlaying() {
    return getInstance()->isActuallyPlaying();
}

bool Music::isActuallyPlaying() {
	return false;
}

/**
 * Music when you talk to Lord British
 */
void Music::lordBritish() {
    playMid(RULEBRIT);
}

/**
 * Music when you talk to Hawkwind
 */
void Music::hawkwind() {
    playMid(SHOPPING);
}

/**
 * Music that plays while camping
 */
void Music::camp() {
    fadeOut(1000);
}

/**
 * Music when talking to a vendor
 */
void Music::shopping() {
    playMid(SHOPPING);
}

/**
 * Play the introduction music on title loadup
 */
void Music::intro() {
    playMid(introMid);
}

/**
 * Cycle through the introduction music
 */
void Music::introSwitch(int n) {
    if (n > NONE && n < MAX) {
        introMid = static_cast<Type>(n);
        intro();
    }
}

/**
 * Toggle the music on/off (usually by pressing 'v')
 */
bool Music::toggle() {
    eventHandler->getTimer()->remove(&Music::callback);

    on = !on;
    if (!on)
        fadeOut(1000);
    else
        fadeIn(1000, true);

    eventHandler->getTimer()->add(&Music::callback, settings.gameCyclesPerSecond);
    return on;    
}



int Music::increaseMusicVolume() {
    if (++settings.musicVol > MAX_VOLUME)
        settings.musicVol = MAX_VOLUME;
    else
        setMusicVolume(settings.musicVol);
    return (settings.musicVol * 100 / MAX_VOLUME);  // percentage
}

int Music::decreaseMusicVolume() {
    if (--settings.musicVol < 0)
        settings.musicVol = 0;
    else
        setMusicVolume(settings.musicVol);
    return (settings.musicVol * 100 / MAX_VOLUME);  // percentage
}



int Music::increaseSoundVolume() {
    if (++settings.soundVol > MAX_VOLUME)
        settings.soundVol = MAX_VOLUME;
    else
        setSoundVolume(settings.soundVol);
    return (settings.soundVol * 100 / MAX_VOLUME);  // percentage
}

int Music::decreaseSoundVolume() {
    if (--settings.soundVol < 0)
        settings.soundVol = 0;
    else
        setSoundVolume(settings.soundVol);
    return (settings.soundVol * 100 / MAX_VOLUME);  // percentage
}
