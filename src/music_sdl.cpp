#include <SDL.h>
#include <SDL_mixer.h>

#include "music.h"
#include "sound.h"

#include "u4_sdl.h"
#include "error.h"
#include "config.h"
#include "context.h"
#include "debug.h"
#include "error.h"
#include "event.h"
#include "location.h"
#include "settings.h"
#include "u4.h"
#include "u4file.h"

void Music::create_sys() {
	/*
	 * initialize sound subsystem
	 */
	TRACE_LOCAL(*logger, "Initializing SDL sound subsystem");

	int audio_rate = 22050;
	Uint16 audio_format = AUDIO_S16LSB; /* 16-bit stereo */
	int audio_channels = 2;
	int audio_buffers = 1024;

	if (u4_SDL_InitSubSystem(SDL_INIT_AUDIO) == -1) {
		errorWarning("unable to init SDL audio subsystem: %s",
				SDL_GetError());
		this->functional = false;
		return;
	}

	TRACE_LOCAL(*logger, "Opening audio");

	if (Mix_OpenAudio(audio_rate, audio_format, audio_channels,
			audio_buffers)) {
		fprintf(stderr, "Unable to open audio!\n");
		this->functional = false;
		return;
	}
	this->functional = true;
	TRACE_LOCAL(*logger, "Allocating channels");

	Mix_AllocateChannels(16);
}

void Music::destroy_sys() {
    if (playing) {
        TRACE_LOCAL(*logger, "Stopping currently playing music");
        Mix_FreeMusic(playing);
        playing = NULL;
    }

    TRACE_LOCAL(*logger, "Closing audio");
    Mix_CloseAudio();

    TRACE_LOCAL(*logger, "Quitting SDL audio subsystem");
    u4_SDL_QuitSubSystem(SDL_INIT_AUDIO);

}

bool Music::load_sys(const string &pathname) {

	if (playing) {
		Mix_FreeMusic(playing);
		playing = NULL;
	}

	playing = Mix_LoadMUS(pathname.c_str());
	if (!playing) {
		errorWarning("unable to load music file %s: %s", pathname.c_str(),
				Mix_GetError());
		return false;
	}
	return true;
}

/**
 * Play a midi file
 */
void Music::playMid(int music) {
    if (!functional || !on)
        return;

    /* loaded a new piece of music */
    if (load(music)) {
        Mix_PlayMusic(playing, NLOOPS);
        //Mix_SetMusicPosition(0.0);  //Could be useful if music was stored on different 'it/mod' patterns
    }
}


/**
 * Stop playing a MIDI file.
 */
void Music::stopMid()
{
    Mix_HaltMusic();
}
/**
 * Set, increase, and decrease sound volume
 */
void Music::setSoundVolume_sys(int volume) {
    /**
     * Use Channel 1 for sound effects
     */
    Mix_Volume(1, int((float)MIX_MAX_VOLUME / MAX_VOLUME * volume));
}

/**
 * System specific version to check if the version is still playing.
 */
bool Music::isPlaying_sys()
{
    return Mix_PlayingMusic();
} /**< Returns true if the mixer is playing any audio */


/**
 * Set, increase, and decrease music volume
 */
void Music::setMusicVolume_sys(int volume) {
    Mix_VolumeMusic(int((float)MIX_MAX_VOLUME / MAX_VOLUME * volume));
}

void Music::fadeIn_sys(int msecs, bool loadFromMap) {
	if (Mix_FadeInMusic(playing, NLOOPS, msecs) == -1)
		errorWarning("Mix_FadeInMusic: %s\n", Mix_GetError());
}

void Music::fadeOut_sys(int msecs) {
	if (Mix_FadeOutMusic(msecs) == -1)
		errorWarning("Mix_FadeOutMusic: %s\n", Mix_GetError());
}
