/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

/* FIXME: should this file have all SDL-related stuff extracted and put in music_sdl.c? */

#include <SDL.h>
#include <SDL_mixer.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "music.h"

#include "context.h"
#include "debug.h"
#include "error.h"
#include "location.h"
#include "settings.h"
#include "u4.h"
#include "u4_sdl.h"
#include "u4file.h"
#include "xml.h"

/* A bug in SDL_mixer 1.2.5 for MacOSX causes it to crash looping music */
#if defined(MACOSX)
#define NLOOPS 1
#else
#define NLOOPS -1
#endif

void musicPlayMid(Music music);
int musicLoad(Music music);

char *musicFilenames[MUSIC_MAX];

Music introMid = MUSIC_TOWNS;
Mix_Music *playing = NULL;

/**
 * Play a midi file
 */
void musicPlayMid(Music music) {    

    if (settings.musicVol == 0) {        
        musicFadeOut(1000);
        return;
    }    

    /* loaded a new piece of music */
    if (musicLoad(music))
        Mix_PlayMusic(playing, NLOOPS);
}

int musicLoad(Music music) {
    static Music current = MUSIC_NONE;
    char *pathname;

    ASSERT(music < MUSIC_MAX, "Attempted to load an invalid piece of music in musicLoad()");

    /* music already loaded */
    if (music == current) {
        /* tell calling function it didn't load correctly (because it's already playing) */
        if (musicIsPlaying())
            return 0;
        /* it loaded correctly */
        else 
            return 1;
    }

    pathname = u4find_music(musicFilenames[music]);
    if (pathname) {
        
        if (playing) {
            Mix_FreeMusic(playing);
            playing = NULL;
        }

        playing = Mix_LoadMUS(pathname);
        if (!playing) {
            errorWarning("unable to load music file %s: %s", pathname, Mix_GetError());
            return 0;
        }
        
        delete pathname;
        current = music;
        return 1;
    }
    else return 0;
}

/**
 * Initiliaze the music
 */
int musicInit() {
    Music musicTrack;
    xmlDocPtr doc;
    xmlNodePtr root, node;

    /*
     * load music track filenames from xml config file
     */

    doc = xmlParse("music.xml");
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "music") != 0)
        errorFatal("malformed music.xml");

    musicTrack = MUSIC_NONE;
    musicFilenames[musicTrack] = NULL;
    musicTrack = (Music)(musicTrack + 1);

    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (musicTrack >= MUSIC_MAX)
            break;

        if (xmlNodeIsText(node) ||
            xmlStrcmp(node->name, (const xmlChar *) "track") != 0)
            continue;

        musicFilenames[musicTrack] = xmlGetPropAsStr(node, "file");
        musicTrack = (Music)(musicTrack + 1);
    }
    xmlFreeDoc(doc);

    /*
     * initialize sound subsystem
     */
    {        
        int audio_rate = 22050;
        Uint16 audio_format = AUDIO_S16LSB; /* 16-bit stereo */
        int audio_channels = 2;
        int audio_buffers = 4096;

        if (u4_SDL_InitSubSystem(SDL_INIT_AUDIO) == -1) {
            errorWarning("unable to init SDL audio subsystem: %s", SDL_GetError());
            return 1;
        }

        if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers)) {
            errorWarning("unable to open audio!");
            return 1;
        }

        Mix_AllocateChannels(16);
    }

    return 0;
}

/**
 * Stop playing the music and cleanup
 */
void musicDelete() {
    if (playing) {
        Mix_FreeMusic(playing);
        playing = NULL;
    }
    
    Mix_CloseAudio();
    u4_SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

/**
 * Returns true if the mixer is playing any audio
 */
int musicIsPlaying(void) {
    return Mix_PlayingMusic();    
}

/**
 * Main music loop
 */
void musicPlay(void) {
    musicPlayMid(c->location->map->music);
}

/**
 * Stop playing music
 */
void musicStop(void) {
    Mix_HaltMusic();
}

/**
 * Fade out the music
 */
void musicFadeOut(int msecs) {
    if (musicIsPlaying()) {
        if (!settings.volumeFades)
            musicStop();
        else {
            if (Mix_FadeOutMusic(msecs) == -1)
                errorWarning("Mix_FadeOutMusic: %s\n", Mix_GetError());
        }
    }
}

/**
 * Fade in the music
 */
void musicFadeIn(int msecs, int loadFromMap) {
    if (!musicIsPlaying() && settings.musicVol) {

        /* make sure we've got something loaded to play */
        if (loadFromMap || !playing)
            musicLoad(c->location->map->music);        

        if (!settings.volumeFades)
            Mix_PlayMusic(playing, NLOOPS);
        else {        
            if(Mix_FadeInMusic(playing, NLOOPS, msecs) == -1)
                errorWarning("Mix_FadeInMusic: %s\n", Mix_GetError());
        }
    }
}

/**
 * Music when you talk to Lord British
 */
void musicLordBritish(void) {
    musicPlayMid(MUSIC_FANFARE);
}

/**
 * Music when you talk to Hawkwind
 */
void musicHawkwind(void) {
    musicPlayMid(MUSIC_SHOPPING);
}

/**
 * Music that plays while camping
 */
void musicCamp(void) {
    musicFadeOut(1000);    
}

/**
 * Music when talking to a vendor
 */
void musicShopping(void) {
    musicPlayMid(MUSIC_SHOPPING);
}

/**
 * Play the introduction music on title loadup
 */
void musicIntro(void) {
    musicPlayMid(introMid);
}

/**
 * Cycle through the introduction music
 */
void musicIntroSwitch(int n) {
    if (n > MUSIC_NONE &&
        n < MUSIC_MAX) {
        introMid = (Music)n;
        musicIntro();
    }
}

/**
 * Toggle the music on/off (usually by pressing 'v')
 */
int musicToggle() {
    settings.musicVol = settings.musicVol ? 0 : 1;

    if (!settings.musicVol)
        musicFadeOut(1000);
    else
        musicFadeIn(1000, 1);    

    return settings.musicVol;
}
