/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

/* FIXME: should this file have all SDL-related stuff extracted and put in music_sdl.c? */

#include <SDL.h>
#include <SDL_mixer.h>

#include <memory>
#include <string>
#include <vector>

#include "music.h"

#include "config.h"
#include "context.h"
#include "debug.h"
#include "error.h"
#include "event.h"
#include "location.h"
#include "settings.h"
#include "u4.h"
#include "u4_sdl.h"
#include "u4file.h"

using std::string;
using std::vector;

/* A bug in SDL_mixer 1.2.5 for MacOSX causes it to crash looping music */
#if defined(MACOSX)
#define NLOOPS 1
#else
#define NLOOPS -1
#endif

/*
 * Static variables
 */
Music *Music::instance = NULL;
bool Music::fading = false;
bool Music::on = false;
bool Music::functional = true;

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

    vector<ConfigElement> musicConfs = config->getElement("/config/music").getChildren();
    for (std::vector<ConfigElement>::iterator i = musicConfs.begin(); i != musicConfs.end(); i++) {

        if (i->getName() != "track")
            continue;

        filenames.push_back(i->getString("file"));
        TRACE_LOCAL(*logger, string("\tTrack file: ") + filenames.back());

    }
    filenames.resize(MAX, "");

    /*
     * initialize sound subsystem
     */
    {
        TRACE_LOCAL(*logger, "Initializing SDL sound subsystem");
        
        int audio_rate = 22050;
        Uint16 audio_format = AUDIO_S16LSB; /* 16-bit stereo */
        int audio_channels = 2;
        int audio_buffers = 4096;

        if (u4_SDL_InitSubSystem(SDL_INIT_AUDIO) == -1) {
            errorWarning("unable to init SDL audio subsystem: %s", SDL_GetError());
            functional = false;
            return;
        }

        TRACE_LOCAL(*logger, "Opening audio");

        if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers)) {            
            fprintf(stderr, "Unable to open audio!\n");
            functional = false;
            return;
        }

        TRACE_LOCAL(*logger, "Allocating channels");

        Mix_AllocateChannels(16);
    }

    on = settings.musicVol;
    TRACE(*logger, string("Music initialized: volume is ") + (on ? "on" : "off"));
}

/**
 * Stop playing the music and cleanup
 */
Music::~Music() {
    TRACE(*logger, "Uninitializing music");
    eventHandler->getTimer()->remove(&Music::callback);

    if (playing) {
        TRACE_LOCAL(*logger, "Stopping currently playing music");
        Mix_FreeMusic(playing);
        playing = NULL;
    }

    TRACE_LOCAL(*logger, "Closing audio");    
    Mix_CloseAudio();

    TRACE_LOCAL(*logger, "Quitting SDL audio subsystem");
    u4_SDL_QuitSubSystem(SDL_INIT_AUDIO);

    TRACE(*logger, "Music uninitialized");
    delete logger;
}

/**
 * Play a midi file
 */
void Music::playMid(Type music) {
    if (!functional)
        return;

    if (!settings.musicVol) {
        musicMgr->fadeOut(1000);
        return;    
    }    

    /* loaded a new piece of music */
    if (load(music))
        Mix_PlayMusic(playing, NLOOPS);    
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
        
        if (playing) {
            Mix_FreeMusic(playing);
            playing = NULL;
        }

        playing = Mix_LoadMUS(pathname.c_str());
        if (!playing) {
            errorWarning("unable to load music file %s: %s", pathname.c_str(), Mix_GetError());
            return 0;
        }
        
        current = music;
        return true;
    }
    else return false;
}

/**
 * Ensures that the music is playing if it is supposed to be, or off
 * if it is supposed to be turned off.
 */
void Music::callback(void *data) {    
    eventHandler->getTimer()->remove(&Music::callback);
    
    if (on && !isPlaying())
        musicMgr->play();
    else if (!on && isPlaying())
        musicMgr->stop();
    settings.musicVol = on;
}
    
/**
 * Returns true if the mixer is playing any audio
 */
bool Music::isPlaying() {
    return Mix_PlayingMusic();
}

/**
 * Main music loop
 */
void Music::play() {
    playMid(c->location->map->music);
}

/**
 * Stop playing music
 */
void Music::stop() {
    on = false;
    Mix_HaltMusic();    
}

/**
 * Fade out the music
 */
void Music::fadeOut(int msecs) {
    if (!functional)
        return;

    if (isPlaying()) {        
        if (!settings.volumeFades)
            stop();
        else {            
            if (Mix_FadeOutMusic(msecs) == -1)
                errorWarning("Mix_FadeOutMusic: %s\n", Mix_GetError());            
        }    
    }
}

/**
 * Fade in the music
 */
void Music::fadeIn(int msecs, bool loadFromMap) {
    if (!functional)
        return;

    if (!isPlaying() && settings.musicVol) {
        /* make sure we've got something loaded to play */
        if (loadFromMap || !playing)
            load(c->location->map->music);        

        if (!settings.volumeFades)
            play();
        else {            
            if(Mix_FadeInMusic(playing, NLOOPS, msecs) == -1)
                errorWarning("Mix_FadeInMusic: %s\n", Mix_GetError());            
        }
    }
}

/**
 * Music when you talk to Lord British
 */
void Music::lordBritish() {
    playMid(FANFARE);
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
    settings.musicVol = on;

    if (!on)
        fadeOut(1000);
    else fadeIn(1000, true);   

    eventHandler->getTimer()->add(&Music::callback, settings.gameCyclesPerSecond);
    
    return on;    
}
