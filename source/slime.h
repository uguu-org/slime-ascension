// Player sprite library.

#ifndef SLIME_H_
#define SLIME_H_

#include"pd_api.h"

// Number of bits used in the fractional part of slime's position and velocity.
#define SLIME_FRACTION_BITS   8

// Slime state.
typedef struct
{
   // Slime position.  See comments near SLIME_FRACTION_BITS.
   //
   // This is the coordinate of the bottom center of the sprite.  Bottom
   // center is used to simplify collision detection with the floor.
   int x, y;

   // Slime velocity.  See comments near SLIME_FRACTION_BITS.
   //
   // This is set to a nonzero magnitude at the start of a jump, and
   // returns to zero when the slime lands on some platform.  It's
   // possible to adjust this velocity while in-flight using the
   // crank.  This makes the jumps a bit more forgiving.
   int vx, vy;

   // Movement direction in degrees [0..359].  0 is up, 90 is right.
   //
   // This is always in-sync with absolute crank position.
   unsigned int a;

   // Animation frame [0..7].
   unsigned int frame;

   // Number of frames spent in flight.  This is zero when slime is at rest,
   // and is incremented continuously while slime is in-flight.
   unsigned int in_flight_time;

   // Set to positive value if slime is currently stunned, otherwise zero.
   // If this value is positive, all inputs are ignored, and this value is
   // decremented by one at each frame.
   unsigned int stun;

   // Lowest Y value, used to track maximum height that was reached.
   int peak;

   // Maximum falling distance.
   int max_fall;

   // Height of when vy changed to positive.  This is the starting
   // height of a fall.  LandSlime will use this to update max_fall.
   int fall_start;
} Slime;

// Load sprites.
void LoadSlime(PlaydateAPI *pd);

// Reset slime to starting position.
void ResetSlime(Slime *slime);

// Draw slime.
void DrawSlime(const Slime *slime, int scroll_offset_y, PlaydateAPI *pd);

// Set velocity to initiate a jump in the current direction.
void JumpSlime(Slime *slime);

// Update slime position.
void UpdateSlime(Slime *slime);

// Mark the slime as being hit by meteor.
void HitSlime(Slime *slime, int vx, int vy);

// Check if slime has collided with a platform, returns 1 if so.
//
// This only check for horizontal intersection.  Caller is responsible for
// checking that slime was above the platform before the collision, and is
// current at or below the platform after the collision.
int CollideSlime(const Slime *slime, int floor_x0, int floor_x1);

// Make an in-flight slime come to rest at the specified height.
void LandSlime(Slime *slime, int y);

#endif  // SLIME_H_
