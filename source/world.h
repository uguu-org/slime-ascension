#ifndef WORLD_H_
#define WORLD_H_

#include"pd_api.h"
#include"slime.h"

// Maximum number of platforms that can be generated.
//
// Each platform costs 16 bytes of memory, and we got a fair bit of memory
// to spare, so here we give it 8192 entries.  That ought to be enough.
//
// If we want to be more rigorous about it: The game runs for the length
// of the song, which is 154.15 seconds.  At 30 frames per second and at 8
// frames per jump, player can complete at most 578 jumps.  Because some
// platforms generate a diversion on the side, we should need at most
// 578*2 = 1156 platforms.
//
// But this doesn't take the springs into account, which allows the player
// to skip over several platforms at once.  Getting this estimate right is
// a bit messy, so we just picked a reasonable multiplier and rounded the
// number to 8192.
#define MAX_PLATFORMS   8192

// Maximum number of meteors that can be spawned.  This matches maximum beat
// number in bgm.c.
#define MAX_METEORS     138

// Maximum number of springs that can be spawned.  It seems fair to have
// this match number of meteors, although in practice we will usually not
// hit this limit due to the low probability of generating a spring.
#define MAX_SPRINGS     MAX_METEORS

// A single platform for slimes to stand on.
typedef struct
{
   // Top left corner of the platform's collision rectangle.
   //
   // Y is always negative.  Platforms at higher elevations will have a
   // lower Y value.  Y is never zero since zero is the starting floor.
   int x, y;

   // Index of platform image [-1..23].
   // This also determines the width of the collision rectangle.
   //
   // -1 means the platform is the starting floor.  We use a special case
   // for the floor so that we don't have to handle that special case in
   // collision checks.
   int16_t type;

   // Platform horizontal velocity, in the range of [-2..2] modulus
   // SCREEN_WIDTH.
   uint16_t vx;

   // If there are any springs attached to this platform, this would be the
   // spring index, otherwise it's -1.
   int spring_index;
} Platform;

// A single meteor, contributing some downward velocity to slime when hit.
typedef struct
{
   // Center of the meteor.
   int x, y;

   // Movement direction.  vy is always positive, so meteor moves downward.
   int16_t vx, vy;

   // Rotation step [0..17].
   uint8_t frame;

   // If meteor has already collided with slime, this field will be set to
   // 1, and meteor is no longer eligible for collisions.  This is helpful
   // because the meteor transfers a bit of its velocity to the slime, which
   // means the slime was likely still on the meteor's trajectory and would
   // therefore sustained multiple continuous hits.  By making each meteor
   // contribute at most one hit, we no longer have the situation of a
   // meteor pushing the slime continuously, which makes the game easier.
   uint8_t hit;
} Meteor;

// A single spring, contributing some upward velocity to slime when landed on.
typedef struct
{
   // Center of bottom edge of spring.
   int x, y;

   // Spring compression state [0..2].
   //
   // Initial state is uncompressed (0).  When slime lands on top of
   // spring from above, slime's vertical downward velocity is reduced
   // to match the spring's compression rate.  After the final frame,
   // spring compression state is reset to zero, and slime is given a
   // boost in upward velocity.
   int frame;
} Spring;

// Style of newly generated platforms.  These correspond to the phase
// which the game is in.
typedef enum
{
   kPlatformTrees,
   kPlatformRocks,
   kPlatformClouds,
   kPlatformSpace
} PlatformStyle;

// World is a collection of platforms and slimes.
typedef struct
{
   // Player-controlled slime.
   Slime slime;

   // Index of the next empty platform spot.  Platforms are lazily
   // generated as they come into view, with new platforms appended to
   // end of platform array.
   int platform_limit;

   // Index of the last platform that was tested for collision.  We check
   // collision of slime against platforms by linearly searching through the
   // list of platforms, starting from platform_cursor.  This is faster than
   // doing binary search through platform[] array since the vertical
   // position do not change all that much from frame to frame.
   //
   // If this index is 0, it means the last platform tested is the floor.
   int platform_cursor;

   // Style of newly generated platforms.
   PlatformStyle platform_style;

   // Scroll offset.  This will be added to all sprites' Y coordinates
   // before drawing.  We use this instead of setDrawOffset so we have
   // better control of mixing scrolling and non-scrolling elements.
   int scroll_offset_y;

   // Song beat at last observation.  This determines number of
   // meteors to launch.
   int beat;

   // Background color [0..64], computed from average of visible platform types.
   int background_color;

   // Lowest index of a live meteor.  meteor_start <= meteors_end.
   int meteor_start;

   // Index of next meteor to spawn.
   int meteor_end;

   // If nonzero, all currently visible meteors will be removed, and new
   // meteors will not spawn until disable_meteors becomes zero.
   int disable_meteors;

   // Index of the next available spring slot.
   int spring_limit;

   // Array data are placed near the end of this struct, with the largest
   // platform[] array at the end.  This is so that we group the small
   // scalar members together, which should help with cache performance.

   // Flying meteors.
   Meteor meteor[MAX_METEORS];

   // Shortcut springs, sorted by elevation from lowest to highest.
   Spring spring[MAX_SPRINGS];

   // List of platforms, sorted by elevation from lowest to highest.
   Platform platform[MAX_PLATFORMS];
} World;

// Load world tiles.
void LoadWorld(PlaydateAPI *pd);

// Reset world to initial state.
void ResetWorld(World *world);

// Run a single time step of world+slime updates and render world.
void UpdateWorld(World *world);

// Draw updated world.
void DrawWorld(const World *world, PlaydateAPI *pd);

#endif  // WORLD_H_
