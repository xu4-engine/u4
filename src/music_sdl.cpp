#include <SDL.h>
#include <SDL_mixer.h>

#include "music.h"
#include "sound.h"

#include "music_sdl.h"
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

MusicSDL::MusicSDL() : Music() {
	/*
	 * initialize sound subsystem
	 */
	{
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


    on = settings.musicVol;
    setMusicVolume(settings.musicVol);
    setSoundVolume(settings.soundVol);

}



MusicSDL::~MusicSDL()
{
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

bool MusicSDL::doLoad(Type music, string pathname, Type & current) {

	if (playing) {
		Mix_FreeMusic(playing);
		playing = NULL;
	}

	playing = Mix_LoadMUS(pathname.c_str());
	if (!playing) {
		errorWarning("unable to load music file %s: %s", pathname.c_str(),
				Mix_GetError());
		return 0;
	}

	current = music;
	return true;

}

/**
 * Main music loop
 */
void MusicSDL::play() {
	playMid(c->location->map->music);
}

/**
 * Stop playing music
 */
void MusicSDL::stop() {
	on = false;
	Mix_HaltMusic();
}

/**
 * Fade out the music
 */
void MusicSDL::fadeOut(int msecs) {
	// fade the music out even if 'on' is false
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
void MusicSDL::fadeIn(int msecs, bool loadFromMap) {
	if (!functional || !on)
		return;

	if (!isPlaying()) {
		/* make sure we've got something loaded to play */
		if (loadFromMap || !playing)
			load(c->location->map->music);

		if (!settings.volumeFades)
			play();
		else {
			if (Mix_FadeInMusic(playing, NLOOPS, msecs) == -1)
				errorWarning("Mix_FadeInMusic: %s\n", Mix_GetError());
		}
	}
}

/**
 * Returns true if the mixer is playing any audio
 */
bool MusicSDL::isActuallyPlaying() {
	return Mix_PlayingMusic();
}

/**
 * Play a midi file
 */
void MusicSDL::playMid(Type music) {
    if (!functional || !on)
        return;

    /* loaded a new piece of music */
    if (load(music)) {
        Mix_PlayMusic(playing, NLOOPS);
        //Mix_SetMusicPosition(0.0);  //Could be useful if music was stored on different 'it/mod' patterns
    }
}

/**
 * Set, increase, and decrease sound volume
 */
void MusicSDL::setSoundVolume(int volume) {
    /**
     * Use Channel 1 for sound effects
     */
    Mix_Volume(1, int((float)MIX_MAX_VOLUME / MAX_VOLUME * volume));
}


/**
 * Set, increase, and decrease music volume
 */
void MusicSDL::setMusicVolume(int volume) {
    Mix_VolumeMusic(int((float)MIX_MAX_VOLUME / MAX_VOLUME * volume));
}

/**
 * Returns an instance of the Music class
 */
Music *MusicSDL::getSDLInstance() {
    if (!instance)
        instance = new MusicSDL();
    return instance;
}

void MusicSDL::replaceMusicInstance()
{
	MusicSDL::GET_MUSIC_INSTANCE = &MusicSDL::getSDLInstance;
}


