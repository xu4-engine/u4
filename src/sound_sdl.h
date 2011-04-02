/*
 * SoundMgr_SDL.h
 *
 *  Created on: 2011-01-27
 *      Author: Darren Janeczek
 */

#ifndef SOUNDMGR_SDL_H_
#define SOUNDMGR_SDL_H_

#include "sound.h"
#include <vector>
#include <SDL.h>
#include <SDL_mixer.h>


class SoundMgr_SDL : public SoundMgr {
public:
	SoundMgr_SDL(){};
	virtual ~SoundMgr_SDL(){};

	int init(void);
	bool load(Sound sound);
	void play(Sound sound, bool onlyOnce = true, int specificDurationInTicks = -1);
	void stop(int channel = 1);
    void del() {}

	static SoundMgr *getSDLInstance();
	static void replaceSoundMgrInstance();
private:
	std::vector<Mix_Chunk *> soundChunk;
};

#endif /* SOUNDMGR_SDL_H_ */
