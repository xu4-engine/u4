/*
 * $Id$
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "u4.h"
#include "music.h"
#include "context.h"
#include "error.h"
#include "location.h"
#include "settings.h"
#include "u4file.h"

void musicPlayMid(Music music);

char *musicFilenames[MUSIC_MAX];

Music introMid = MUSIC_TOWNS;
Mix_Music *playing = NULL;

/**
 * Play a midi file
 */
void musicPlayMid(Music music) {
    static Music current = MUSIC_NONE;
    char *pathname;

    assert(music < MUSIC_MAX);

    if (settings->vol == 0 || music == MUSIC_NONE) {
        if (musicIsPlaying())
            musicFadeOut(1000);
        return;
    }

    if (playing) {
        Mix_FreeMusic(playing);
        playing = NULL;
    }

    current = music;

    pathname = u4find_music(musicFilenames[music]);
    if (pathname) {
        playing = Mix_LoadMUS(pathname);
        if (!playing)
            errorWarning("unable to load music file %s: %s", pathname, Mix_GetError());
        
        Mix_PlayMusic(playing, -1);            
        free(pathname);
    }
}

/**
 * Initiliaze the music
 */
int musicInit() {
    char *fname;
    Music musicTrack;
    xmlDocPtr doc;
    xmlNodePtr root, node;

    /*
     * load music track filenames from xml config file
     */

    fname = u4find_conf("music.xml");
    if (!fname)
        errorFatal("unable to open file music.xml");
    doc = xmlParseFile(fname);
    if (!doc)
        errorFatal("error parsing music.xml");

    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "music") != 0)
        errorFatal("malformed music.xml");

    musicTrack = MUSIC_NONE;
    musicFilenames[musicTrack] = NULL;
    musicTrack++;

    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (musicTrack >= MUSIC_MAX)
            break;

        if (xmlNodeIsText(node) ||
            xmlStrcmp(node->name, (const xmlChar *) "track") != 0)
            continue;

        musicFilenames[musicTrack] = (char *)xmlGetProp(node, (const xmlChar *) "file");
        musicTrack++;
    }
    xmlFreeDoc(doc);
    free(fname);

    /*
     * initialize sound subsystem
     */
    {
        int audio_rate = 22050;
        Uint16 audio_format = AUDIO_S16; /* 16-bit stereo */
        int audio_channels = 2;
        int audio_buffers = 4096;

        if (SDL_InitSubSystem(SDL_INIT_AUDIO) == -1) {
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

    SDL_QuitSubSystem(SDL_INIT_AUDIO);
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
        if (Mix_FadeOutMusic(msecs) == -1)
            errorWarning("Mix_FadeOutMusic: %s\n", Mix_GetError());
    }
}

/**
 * Fade in the music
 */
void musicFadeIn(int msecs) {
    if (!musicIsPlaying()) {
        /* make sure we've got something loaded to play */
        if (!playing)
            musicPlay();        
        
        if(Mix_FadeInMusic(playing, -1, msecs) == -1)
            errorWarning("Mix_FadeInMusic: %s\n", Mix_GetError());
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
        introMid = n;
        musicIntro();
    }
}

/**
 * Toggle the music on/off (usually by pressing 'v')
 */
int musicToggle() {
    settings->vol = settings->vol ? 0 : 1;

    if (!settings->vol)
        musicFadeOut(1000);
    else
        musicFadeIn(1000);    

    return settings->vol;
}
