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

char *musicFilenames[MUSIC_MAX];

int toggle = 1;
Music introMid = MUSIC_TOWNS;
Mix_Music *playing = NULL;

void musicPlayMid(Music music) {
    static Music current = MUSIC_NONE;
    char *pathname;

    assert(music < MUSIC_MAX);

    if (settings->vol == 0 || current == music) {
        if (!settings->vol && playing)
            musicFadeOut(1000);
        return;
    }

    if (playing) {
        Mix_FreeMusic(playing);
        playing = NULL;
    }

    if (music == MUSIC_NONE) {
        musicFadeOut(1000);
        return;
    }

    current = music;

    pathname = u4find_music(musicFilenames[music]);
    if (pathname) {
        playing = Mix_LoadMUS(pathname);
        if (!playing)
            errorWarning("unable to load music file %s: %s", pathname, Mix_GetError());
        if (toggle && playing)
            Mix_PlayMusic(playing, -1);
        free(pathname);
    }
}

int musicToggle() {
    toggle = !toggle;

    if (settings->vol) {
        if (!toggle)
            musicFadeOut(1000);
        else if (playing)
            musicFadeIn(1000);
    }

    return toggle;
}

void musicIntro(void) {         /* Intro Music on title loadup */
    musicPlayMid(introMid);
}

void musicIntroSwitch(int n) {
    if (n > MUSIC_NONE &&
        n < MUSIC_MAX) {
        introMid = n;
        musicIntro();
    }
}

void musicLordBritish(void) {  /* Music when you talk to LB */
    musicPlayMid(MUSIC_FANFARE);
}

void musicHawkwind(void) { /* Music when you talk to Hawkwind */
    musicPlayMid(MUSIC_SHOPPING);
}

void musicCamp(void){  /* Music when camping */
    musicFadeOut(1000);    
}

void musicShopping(void){  /* Music when talking to a vendor */
    musicPlayMid(MUSIC_SHOPPING);
}

void musicPlay(void) {  /* Main music loop. */
    musicPlayMid(c->location->map->music);
}

void musicStop(void) {
    Mix_PauseMusic();
}

void musicFadeOut(int msecs) {
    if (settings->vol)
        Mix_FadeOutMusic(msecs);
}

void musicFadeIn(int msecs) {
    if (settings->vol && toggle)
        Mix_FadeInMusic(playing, -1, msecs);
}

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
    if (settings->vol) {
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

void musicDelete() {
    if (playing) {
        Mix_FreeMusic(playing);
        playing = NULL;
    }

    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}
