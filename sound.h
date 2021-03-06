#pragma once

/** Handles sound samples and music.
*/

#include <string>
using std::string;

#include "resources.h"

#if defined(__ANDROID__)
#include <SDL_mixer.h>
#elif defined(__linux)
#include <SDL/SDL_mixer.h>
#else
#include <SDL/SDL_mixer.h>
#endif

bool initSound();
void updateSound();
void freeSound();

namespace Gigalomania {
	class Sample : public TrackedObject {
		bool is_music;
		Mix_Music *music;
		Mix_Chunk *chunk;
		int channel;

		string text;

		Sample(bool is_music, Mix_Music *music, Mix_Chunk *chunk) : is_music(is_music), music(music), chunk(chunk) {
			channel = -1;
		}
	public:
		Sample() : is_music(false), music(NULL), chunk(NULL) {
			// create dummy Sample
		}
		virtual ~Sample();
		virtual const char *getClass() const { return "CLASS_SAMPLE"; }
		void play(int ch, int loops);
		//bool isPlaying() const;
		void fadeOut(int duration_ms);
		void setVolume(float volume);
		void setText(const char *text) {
			this->text = text;
		}

		static void pauseMusic();
		static void unpauseMusic();
		static void pauseChannel(int ch);
		static void unpauseChannel(int ch);

		static Sample *loadSample(const char *filename, bool iff = false);
		static Sample *loadSample(string filename, bool iff = false);
		static Sample *loadMusic(const char *filename);
		static Sample *loadMusic(string filename);
	};
}

const int SOUND_CHANNEL_SAMPLES   = 0;
const int SOUND_CHANNEL_MUSIC     = 1;
const int SOUND_CHANNEL_FX        = 2;
const int SOUND_CHANNEL_BIPLANE   = 3;
const int SOUND_CHANNEL_BOMBER    = 4;
const int SOUND_CHANNEL_SPACESHIP = 5;

inline void playSample(Gigalomania::Sample *sample, int channel = SOUND_CHANNEL_SAMPLES, int loops = 0) {
	sample->play(channel, loops);
}

bool isPlaying(int ch);

bool errorSound();
void resetErrorSound();
