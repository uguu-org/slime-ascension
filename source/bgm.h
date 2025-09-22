// Library for managing background music.

#ifndef BGM_H_
#define BGM_H_

#include"pd_api.h"

// Start background music.
void PlayBackgroundMusic(PlaydateAPI *pd);

// Stop background music.
void StopBackgroundMusic(PlaydateAPI *pd);

// Get current song beat.  Returns a number where lower 16 bits is the song
// beat, and upper 16 bits is the song phase.
//
// PlayBackgroundMusic must have been called first.
int GetSongBeat(PlaydateAPI *pd);

#endif  // BGM_H_
