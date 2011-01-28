#ifndef MUSIC_SDL_H_
#define MUSIC_SDL_H_

#include <SDL.h>
#include <SDL_mixer.h>
#include "music.h"

class MusicSDL: public Music {
public:
	MusicSDL();
	~MusicSDL();

	bool doLoad(Type music,string pathname,Type & current);

	void play();
	void stop();

	void fadeOut(int msecs);
	void fadeIn(int msecs, bool loadFromMap);

	bool isActuallyPlaying();
	void playMid(Type music);
	void setMusicVolume(int volume);
    void setSoundVolume(int volume);

    static void replaceMusicInstance();
    static Music * getSDLInstance();
};

#endif /* MUSIC_SDL_H_ */
