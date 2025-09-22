#include"bgm.h"
#include"common.h"

// File player handle.
static FilePlayer *g_fileplayer = NULL;

// Song time states.  We maintain our own time because fileplayer->getOffset
// is unreliable.
//
// The lesson learned here is that you can't really synchronize
// something to music unless you are doing your own synth.
static uint64_t g_last_update_time_ms;
static uint32_t g_song_time_ms;

typedef struct
{
   float timestamp;
   int beat;
} BeatData;
static const BeatData kSongBeats[] =
{
   // Tree phase.
   {12.336, 0},
   {21.200, 1},
   {30.009, 2},
   {39.006, 3},
   {48.004, 4},
   {56.911, 5},

   // Rock phase.
   {58.008, 5 | (1 << 16)},  // No meteor at start of phase.
   {58.588, 6 | (1 << 16)},
   {59.642, 7 | (1 << 16)},
   {61.743, 8 | (1 << 16)},
   {62.829, 9 | (1 << 16)},
   {63.842, 10 | (1 << 16)},
   {64.844, 11 | (1 << 16)},
   {65.392, 12 | (1 << 16)},

   {66.942, 13 | (1 << 16)},
   {67.934, 14 | (1 << 16)},
   {68.925, 15 | (1 << 16)},
   {69.948, 16 | (1 << 16)},
   {70.929, 17 | (1 << 16)},
   {71.952, 18 | (1 << 16)},
   {72.923, 19 | (1 << 16)},
   {73.914, 20 | (1 << 16)},

   {74.916, 21 | (1 << 16)},
   {75.907, 22 | (1 << 16)},
   {76.888, 23 | (1 << 16)},
   {77.795, 24 | (1 << 16)},
   {78.808, 25 | (1 << 16)},
   {79.736, 26 | (1 << 16)},
   {80.633, 27 | (1 << 16)},
   {81.624, 28 | (1 << 16)},

   {82.520, 29 | (1 << 16)},
   {83.470, 30 | (1 << 16)},
   {84.356, 31 | (1 << 16)},
   {85.294, 32 | (1 << 16)},
   {86.149, 33 | (1 << 16)},
   {87.024, 34 | (1 << 16)},
   {87.931, 35 | (1 << 16)},
   {88.796, 36 | (1 << 16)},

   {89.661, 37 | (1 << 16)},
   {90.536, 38 | (1 << 16)},
   {91.369, 39 | (1 << 16)},
   {92.192, 40 | (1 << 16)},
   {93.036, 41 | (1 << 16)},
   {93.901, 42 | (1 << 16)},
   {94.692, 43 | (1 << 16)},
   {95.493, 44 | (1 << 16)},

   {96.295, 45 | (1 << 16)},
   {97.086, 46 | (1 << 16)},
   {97.835, 47 | (1 << 16)},
   {98.573, 48 | (1 << 16)},
   {99.311, 49 | (1 << 16)},
   {99.670, 50 | (1 << 16)},
   {100.092, 51 | (1 << 16)},
   {100.440, 52 | (1 << 16)},
   {100.809, 53 | (1 << 16)},
   {101.178, 54 | (1 << 16)},
   {101.537, 55 | (1 << 16)},

   // Cloud phase.
   {101.968, 55 | (2 << 16)},  // No meteor at start of phase.
   {102.349, 56 | (2 << 16)},
   {103.035, 57 | (2 << 16)},
   {103.815, 58 | (2 << 16)},
   {104.543, 59 | (2 << 16)},
   {105.271, 60 | (2 << 16)},
   {105.977, 61 | (2 << 16)},
   {106.663, 62 | (2 << 16)},
   {107.401, 63 | (2 << 16)},

   {108.108, 64 | (2 << 16)},
   {108.814, 65 | (2 << 16)},
   {109.532, 66 | (2 << 16)},
   {110.249, 67 | (2 << 16)},
   {110.913, 68 | (2 << 16)},
   {111.620, 69 | (2 << 16)},
   {112.305, 70 | (2 << 16)},
   {113.012, 71 | (2 << 16)},

   {113.708, 72 | (2 << 16)},
   {114.394, 73 | (2 << 16)},
   {115.079, 74 | (2 << 16)},
   {115.733, 75 | (2 << 16)},
   {116.419, 76 | (2 << 16)},
   {117.052, 77 | (2 << 16)},
   {117.706, 78 | (2 << 16)},
   {118.444, 79 | (2 << 16)},

   {119.119, 80 | (2 << 16)},
   {119.710, 81 | (2 << 16)},
   {120.353, 82 | (2 << 16)},
   {121.038, 83 | (2 << 16)},
   {121.703, 84 | (2 << 16)},
   {122.420, 85 | (2 << 16)},
   {123.021, 86 | (2 << 16)},
   {123.675, 87 | (2 << 16)},

   {124.361, 88 | (2 << 16)},
   {124.962, 89 | (2 << 16)},
   {125.626, 90 | (2 << 16)},
   {126.216, 91 | (2 << 16)},
   {126.913, 92 | (2 << 16)},
   {127.546, 93 | (2 << 16)},
   {128.189, 94 | (2 << 16)},
   {128.822, 95 | (2 << 16)},

   {129.413, 96 | (2 << 16)},
   {130.046, 97 | (2 << 16)},
   {130.689, 98 | (2 << 16)},
   {131.290, 99 | (2 << 16)},
   {131.860, 100 | (2 << 16)},
   {132.493, 101 | (2 << 16)},
   {133.073, 102 | (2 << 16)},
   {133.695, 103 | (2 << 16)},

   // Space phase.
   {133.933, 103 | (3 << 16)},  // No meteor at start of phase.
   {134.264, 104 | (3 << 16)},
   {135.413, 106 | (3 << 16)},
   {136.400, 108 | (3 << 16)},

   {139.318, 110 | (3 << 16)},
   {140.251, 112 | (3 << 16)},

   {143.180, 114 | (3 << 16)},
   {144.134, 116 | (3 << 16)},

   {145.082, 118 | (3 << 16)},
   {145.344, 120 | (3 << 16)},
   {145.598, 122 | (3 << 16)},
   {145.850, 124 | (3 << 16)},
   {146.100, 126 | (3 << 16)},
   {146.347, 128 | (3 << 16)},
   {146.601, 130 | (3 << 16)},

   {149.816, 138 | (3 << 16)},

   // End of song.  Timestamp here is bogus, we rely on isPlaying to
   // determine the true end of the song.  This is so that we will
   // hear the full song even if our time tracking is off.
   {999.999, 138 | (4 << 16)}
};
static const int kSongBeatCount = sizeof(kSongBeats) / sizeof(BeatData);

// Index into kSongBeats.
static int g_song_cursor = 0;

// Start playing background music.
void PlayBackgroundMusic(PlaydateAPI *pd)
{
   if( g_fileplayer != NULL )
      return;
   g_fileplayer = pd->sound->fileplayer->newPlayer();
   pd->sound->fileplayer->loadIntoPlayer(
      g_fileplayer, "in_the_hall_of_the_mountain_king");
   pd->sound->fileplayer->play(g_fileplayer, 1);

   g_song_cursor = 0;
   g_song_time_ms = 0;
   g_last_update_time_ms = pd->system->getCurrentTimeMilliseconds();
}

// Stop background music.
//
// g_fileplayer is released as part of this step.  We could have kept it
// around to avoid recreating the fileplayer when user decided to enable
// background music later, but doing that will cause the background music to
// resume playback from a random time offset, as opposed to restart from the
// beginning.  Calling setOffset appears to have no effect.
//
// Given that there is no reason to hold on to the resources when we aren't
// playing anyways, we just recreate the fileplayer whenever we need to
// start the background music.
void StopBackgroundMusic(PlaydateAPI *pd)
{
   if( g_fileplayer == NULL )
      return;
   pd->sound->fileplayer->stop(g_fileplayer);
   pd->sound->fileplayer->freePlayer(g_fileplayer);
   g_fileplayer = NULL;
}

// Get current song phase.
int GetSongBeat(PlaydateAPI *pd)
{
   if( pd->sound->fileplayer->isPlaying(g_fileplayer) == 0 )
      return kSongBeats[kSongBeatCount - 1].beat;

   // Update clocks.
   const uint32_t current_time_ms = pd->system->getCurrentTimeMilliseconds();
   const uint32_t delta_time_ms = current_time_ms - g_last_update_time_ms;
   if( UNLIKELY(delta_time_ms < 0 || delta_time_ms > 1000) )
      g_song_time_ms++;
   else
      g_song_time_ms += delta_time_ms;
   g_last_update_time_ms = current_time_ms;

   #ifndef NDEBUG
      const int previous_cursor = g_song_cursor;
   #endif

   const float t = g_song_time_ms / 1000.0f;
   while( g_song_cursor < kSongBeatCount &&
          kSongBeats[g_song_cursor].timestamp < t )
   {
      g_song_cursor++;
   }

   #ifndef NDEBUG
      // Log beat change to console.
      if( previous_cursor != g_song_cursor )
      {
         pd->system->logToConsole(
            "song_cursor: %d -> %d, time = %u, beat = %x",
            previous_cursor,
            g_song_cursor,
            g_song_time_ms,
            g_song_cursor >= kSongBeatCount
               ? kSongBeats[kSongBeatCount - 1].beat
               : kSongBeats[g_song_cursor].beat);
      }
   #endif

   if( g_song_cursor >= kSongBeatCount )
      return kSongBeats[kSongBeatCount - 1].beat;
   return kSongBeats[g_song_cursor].beat;
}
