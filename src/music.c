/*
 * $Id$
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include "u4.h"

#include "music.h"

#include "context.h"
#include "error.h"
#include "location.h"
#include "settings.h"
#include "u4file.h"

const char * const musicFilenames[] = {
    NULL,
    "Wanderer.mid",
    "Towns.mid",
    "Shrines.mid",
    "Shopping.mid",
    "Rule_Britannia.mid",
    "Fanfare_Of_Lord_British.mid",
    "Dungeon.mid",
    "Combat.mid",
    "Castles.mid"
};

int soundDisabled;
int toggle = 1;
Music introMid = MUSIC_TOWNS;
Mix_Music *playing = NULL;

void musicPlayMid(Music music) {
    static Music current = MUSIC_NONE;
    char *pathname;

    assert(music < sizeof(musicFilenames) / sizeof(musicFilenames[0]));

    if (soundDisabled || current == music)
        return;

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

    if (!soundDisabled) {
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
        n <= MUSIC_CASTLES) {
        introMid = n;
        musicIntro();
    }
}

void musicLordBritish(void){  /* Music when you talk to LB */
    musicPlayMid(MUSIC_FANFARE);
}

void musicCamp(void){  /* Music when camping */
    musicPlayMid(MUSIC_SHOPPING);
}

void musicPlay(void) {  /* Main music loop. */
    musicPlayMid(c->location->map->music);
}

void musicStop(void) {
    Mix_PauseMusic();
}

void musicFadeOut(int msecs) {
    if (!soundDisabled)
        Mix_FadeOutMusic(msecs);
}

void musicFadeIn(int msecs) {
    if (!soundDisabled && toggle)
        Mix_FadeInMusic(playing, -1, msecs);
}

int musicInit() {

    soundDisabled = settings->vol == 0;

    if (!soundDisabled) {
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


        Mix_AllocateChannels(1);
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
