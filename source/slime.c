#include"slime.h"
#include"common.h"

// Sprite offsets.
#define BODY_OFFSET_X         (-32)
#define BODY_OFFSET_Y         (-64)
#define LEFT_EYE_OFFSET_X     (-14)
#define RIGHT_EYE_OFFSET_X    2
#define EYE_OFFSET_Y          (-19)

// Acceleration due to gravity in sub-pixels per frame.
//
// Each pixel is worth (1 << SLIME_FRACTION_BITS) subpixels.
#define GRAVITY               200

// Maximum downward velocity in sub-pixels per frame.
#define TERMINAL_VELOCITY     (8 << SLIME_FRACTION_BITS)

// Syntactic sugar.
#define FIXED_SCREEN_WIDTH    (SCREEN_WIDTH << SLIME_FRACTION_BITS)
#define PEAK_SLIME_FRAME      7

// Image handles.
static LCDBitmapTable *g_body;
static LCDBitmapTable *g_eyes;

// Table of precomputed velocities for each angle.
typedef struct
{
   int16_t x, y;
} ShortXY;
static const ShortXY kVelocityTable[] =
{
   #include"build/velocity_table.txt"
};

// Load sprites.
void LoadSlime(PlaydateAPI *pd)
{
   const char *error;
   g_body = pd->graphics->loadBitmapTable("body", &error);
   assert(g_body != NULL);
   g_eyes = pd->graphics->loadBitmapTable("eyes", &error);
   assert(g_eyes != NULL);

   #ifndef NDEBUG
      int count, cellswide;
      pd->graphics->getBitmapTableInfo(g_body, &count, &cellswide);
      assert(count == 8);
      assert(cellswide == 1);
      pd->graphics->getBitmapTableInfo(g_eyes, &count, &cellswide);
      assert(count == 37);
      assert(cellswide == 1);
   #endif
}

// Reset slime to starting position.
void ResetSlime(Slime *slime)
{
   slime->x = (SCREEN_WIDTH / 2) << SLIME_FRACTION_BITS;
   slime->y = 0;
   slime->a = 0;
   slime->frame = 0;
   slime->vx = 0;
   slime->vy = 0;
   slime->peak = 0;
   slime->fall_start = 0;
   slime->max_fall = 0;
}

// Draw slime.
void DrawSlime(const Slime *slime, int scroll_offset_y, PlaydateAPI *pd)
{
   // Draw body.
   assert(g_body != NULL);
   LCDBitmap *body = pd->graphics->getTableBitmap(g_body, slime->frame);
   assert(body != NULL);
   const int x = slime->x >> SLIME_FRACTION_BITS;
   const int y = (slime->y >> SLIME_FRACTION_BITS) + scroll_offset_y;
   pd->graphics->drawBitmap(body,
                            x + BODY_OFFSET_X,
                            y + BODY_OFFSET_Y,
                            kBitmapUnflipped);

   assert(slime->a >= 0);
   assert(slime->a < 360);
   assert(g_eyes != NULL);
   LCDBitmap *eye = pd->graphics->getTableBitmap(
      g_eyes, slime->stun > 0 ? 36 : slime->a / 10);
   assert(eye != NULL);
   const int eye_y = y + EYE_OFFSET_Y - slime->frame;
   pd->graphics->drawBitmap(eye,
                            x + LEFT_EYE_OFFSET_X,
                            eye_y,
                            kBitmapUnflipped);
   pd->graphics->drawBitmap(eye,
                            x + RIGHT_EYE_OFFSET_X,
                            eye_y,
                            kBitmapUnflipped);

   // Wraparound.
   if( UNLIKELY(x <= 32) )
   {
      pd->graphics->drawBitmap(body,
                               x + BODY_OFFSET_X + SCREEN_WIDTH,
                               y + BODY_OFFSET_Y,
                               kBitmapUnflipped);
      pd->graphics->drawBitmap(eye,
                               x + LEFT_EYE_OFFSET_X + SCREEN_WIDTH,
                               eye_y,
                               kBitmapUnflipped);
      pd->graphics->drawBitmap(eye,
                               x + RIGHT_EYE_OFFSET_X + SCREEN_WIDTH,
                               eye_y,
                               kBitmapUnflipped);
   }
   else if( UNLIKELY(x > SCREEN_WIDTH - 32) )
   {
      pd->graphics->drawBitmap(body,
                               x + BODY_OFFSET_X - SCREEN_WIDTH,
                               y + BODY_OFFSET_Y,
                               kBitmapUnflipped);
      pd->graphics->drawBitmap(eye,
                               x + LEFT_EYE_OFFSET_X - SCREEN_WIDTH,
                               eye_y,
                               kBitmapUnflipped);
      pd->graphics->drawBitmap(eye,
                               x + RIGHT_EYE_OFFSET_X - SCREEN_WIDTH,
                               eye_y,
                               kBitmapUnflipped);
   }
}

// Set velocity to initiate a jump in the current direction.
void JumpSlime(Slime *slime)
{
   assert(slime->a >= 0);
   assert(slime->a < 360);

   // Input is always ignored when slime is stunned.
   if( slime->stun > 0 )
      return;

   // Accept acceleration to vertical velocity for a few frames after
   // a jump has been initiated.  This makes it possible for player to
   // control jump height by tapping versus holding buttons.
   if( slime->in_flight_time < 5 )
   {
      if( slime->in_flight_time == 0 )
      {
         // If slime recently became at rest, don't allow another jump
         // until its animation state has returned to normal.
         if( slime->frame > 0 )
            return;

         // Enter in-flight state.
         slime->in_flight_time = 1;
      }

      const int old_vy = slime->vy;
      slime->vy += kVelocityTable[slime->a].y;
      if( old_vy <= 0 && slime->vy > 0 )
         slime->fall_start = slime->y;
   }

   // Horizontal velocity is always tied to crank direction, but we
   // don't apply it unless slime is in-flight.
   slime->vx = kVelocityTable[slime->a].x;
}

// Update slime position.
void UpdateSlime(Slime *slime)
{
   assert(slime->a >= 0);
   assert(slime->a < 360);

   // Always wear off stun effect.
   if( slime->stun > 0 )
      slime->stun--;

   // If slime is at rest, return animation state to steady state and
   // we are done.
   if( slime->in_flight_time == 0 )
   {
      if( slime->frame > 0 )
         slime->frame--;
      return;
   }
   slime->in_flight_time++;

   // Apply motion.
   slime->x = (slime->x + slime->vx + FIXED_SCREEN_WIDTH) % FIXED_SCREEN_WIDTH;
   slime->y += slime->vy;

   // Check for collision with ground floor.
   if( slime->y > 0 )
   {
      LandSlime(slime, 0);
      return;
   }
   if( slime->peak > slime->y )
      slime->peak = slime->y;

   // Apply gravity.
   const int old_vy = slime->vy;
   slime->vy += GRAVITY;
   if( slime->vy > TERMINAL_VELOCITY )
      slime->vy = TERMINAL_VELOCITY;
   if( old_vy <= 0 && slime->vy > 0 )
      slime->fall_start = slime->y;

   // Apply animation.
   if( slime->frame < PEAK_SLIME_FRAME )
      slime->frame++;

   // If slime is not stunned, synchronize horizontal velocity with direction.
   if( slime->stun == 0 )
      slime->vx = kVelocityTable[slime->a].x;
}

// Mark the slime as being hit by meteor.
void HitSlime(Slime *slime, int vx, int vy)
{
   // Mark slime as stunned.  This doesn't accumulate, so getting hit
   // multiple times simultaneously will get the same amount of stun.
   slime->stun = 15;

   // If slime was at rest, it will start falling from the platform it's
   // standing on.
   if( slime->in_flight_time == 0 )
      slime->in_flight_time++;

   // Transfer part of the momentum from the hit.
   const int old_vy = slime->vy;
   slime->vx += vx << (SLIME_FRACTION_BITS - 2);
   slime->vy += vy << (SLIME_FRACTION_BITS - 2);
   if( slime->vy > TERMINAL_VELOCITY )
      slime->vy = TERMINAL_VELOCITY;
   if( old_vy <= 0 && slime->vy > 0 )
      slime->fall_start = slime->y;
}

// Check for horizontal collision.
int CollideSlime(const Slime *slime, int floor_x0, int floor_x1)
{
   const int x = slime->x >> SLIME_FRACTION_BITS;
   return floor_x0 < floor_x1 ? floor_x0 <= x && x <= floor_x1
                              : x <= floor_x1 || floor_x0 <= x;
}

// Make an in-flight slime come to rest at the specified height.
void LandSlime(Slime *slime, int y)
{
   slime->y = y << SLIME_FRACTION_BITS;
   slime->vx = 0;
   slime->vy = 0;
   slime->in_flight_time = 0;

   const int fall_height = slime->y - slime->fall_start;
   if( slime->max_fall < fall_height )
      slime->max_fall = fall_height;
}
