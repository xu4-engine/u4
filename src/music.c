/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <SDL.h>
#include <SDL_mixer.h>

#include "u4.h"
#include "map.h"
#include "context.h"
#include "u4file.h"
#include "music.h"

const char *musicFilenames[] = {
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

int toggle = 1;
Mix_Music *playing = NULL;

void musicPlayMid(Music music) {
    static Music current = MUSIC_NONE;
    char *pathname;

    assert(music < sizeof(musicFilenames) / sizeof(musicFilenames[0]));

    if (current == music)
        return;

    if (playing) {
        Mix_FreeMusic(playing);
        playing = NULL;
    }

    if (music == MUSIC_NONE) {
        Mix_FadeOutMusic(1000);
        return;
    }

    current = music;

    pathname = u4find_music(musicFilenames[music]);
    if (pathname) {
        playing = Mix_LoadMUS(pathname);
        if (toggle && playing)
            Mix_PlayMusic(playing, -1);
        free(pathname);
    }
}

int musicToggle() {
    toggle = !toggle;
    if (!toggle)
        Mix_FadeOutMusic(1000);
    else if (playing)
        Mix_FadeInMusic(playing, -1, 1000);

    return toggle;
}

void intro_music(void){    /* Intro Music on title loadup */
    musicPlayMid(MUSIC_TOWNS);
}

void lb_music(void){  /* Music when you talk to LB */
    musicPlayMid(MUSIC_FANFARE);
}

void play_music(void) {  /* Main music loop. */
    musicPlayMid(c->map->music);
}

int init_music(void) {

   int audio_rate = 22050;
   Uint16 audio_format = AUDIO_S16; /* 16-bit stereo */
   int audio_channels = 2;
   int audio_buffers = 4096;

   SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

#ifdef WIN32			
   SDL_AudioInit("waveout");
#endif

if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers)) {
       printf("Unable to open audio!\n");
       return(1);
   }


   Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);
   return(0);
}

