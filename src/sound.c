/*
 * $Id$
 */

#include <stdlib.h>
#include <SDL.h>
#include <SDL_mixer.h>

#include "sound.h"

Mix_Chunk *step = NULL;

int soundInit() {
    return 0;
}

void soundDelete() {
}

void soundPlay(Sound sound) {
    /* FIXME: play the sound! */

#if 0
    if (step == NULL) {
        step = Mix_LoadWAV("walking.wav");
        if (!step) {
            errorWarning("unable to load sound effect file %s: %s", "walking.wav", Mix_GetError());
        }

    }
    Mix_PlayChannel(0, step, 1);
#endif
}
