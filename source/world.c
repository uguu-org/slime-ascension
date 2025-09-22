#include"world.h"
#include<string.h>
#include"common.h"

// Offsets from collision rectangle corner to image location.
#define PLATFORM_OFFSET_X     (-32)
#define PLATFORM_OFFSET_Y     (-48)

// Margin from edges of platforms where jump can be initiated.
#define PLATFORM_MARGIN       16

// Spring sprite offsets.
#define SPRING_OFFSET_X       (-16)
#define SPRING_OFFSET_Y       (-31)

// Vertical velocity to be delivered by spring.
#define SPRING_VELOCITY       ((-20) << SLIME_FRACTION_BITS)

// Meteor sprite offsets.
#define METEOR_OFFSET_X       (-32)
#define METEOR_OFFSET_Y       (-32)

// Meteor velocity ranges.
#define METEOR_MIN_VELOCITY   5
#define METEOR_MAX_VELOCITY   15

// Number of pixels from slime coordinate (bottom edge) to its center.
#define SLIME_CENTER_OFFSET   9

// Image handles.
static LCDBitmapTable *g_platform;
static LCDBitmapTable *g_meteor;
static LCDBitmapTable *g_spring;

// Background patterns.
#include"build/gray_patterns.txt"

// Background pattern indices for each group of platform types,
// indexed by (platform->type / 6).
static const uint8_t kGrayLevel[4] = {0, 7, 49, 62};

// Maximum height from top edge of collision rectangle to bottom of visible
// graphic (minus a few stray pixels) for each group of platform types,
// indexed by world->platform_type.
static const int kPlatformHeight[4] =
{
   232 + PLATFORM_OFFSET_Y,   // kPlatformTrees
   229 + PLATFORM_OFFSET_Y,   // kPlatformRocks
   90 + PLATFORM_OFFSET_Y,    // kPlatformClouds
   79 + PLATFORM_OFFSET_Y,    // kPlatformSpace
};

// Syntactic sugar.
static int Min(int a, int b) { return a < b ? a : b; }

// Load world tiles.
void LoadWorld(PlaydateAPI *pd)
{
   const char *error;
   g_platform = pd->graphics->loadBitmapTable("platform", &error);
   assert(g_platform != NULL);
   g_meteor = pd->graphics->loadBitmapTable("meteor", &error);
   assert(g_meteor != NULL);
   g_spring = pd->graphics->loadBitmapTable("spring", &error);
   assert(g_spring != NULL);
}

// Reset world to initial state.
void ResetWorld(World *world)
{
   world->platform_limit = 1;
   world->platform_cursor = 0;
   world->platform_style = kPlatformTrees;

   world->platform[0].x = 0;
   world->platform[0].y = 0;
   world->platform[0].type = -1;

   world->beat = 0;
   world->meteor_start = 0;
   world->meteor_end = 0;

   world->spring_limit = 0;
   world->scroll_offset_y = 0;

   ResetSlime(&(world->slime));
}

// Draw background pattern.
static void DrawBackground(const World *world, PlaydateAPI *pd)
{
   // Initialize background pattern, taking scrolling into account.
   LCDPattern pattern;
   memcpy(pattern,
          kGrayPattern[world->background_color] +
             ((-world->scroll_offset_y) & 7),
          8);
   memset(pattern + 8, 0xff, 8);

   // Repaint background with a uniform pattern.
   pd->graphics->fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (LCDColor)pattern);
}

// Draw platform images starting from end_index backwards until next
// platform is outside of visible area.
static void DrawPlatforms(const World *world, PlaydateAPI *pd)
{
   assert(g_platform != NULL);

   // Draw platforms from back to front.  This is because new platforms that
   // are at higher elevations are appended to the end of the array, and should
   // be drawn behind the platforms that are at lower elevations.
   const Platform *platform = world->platform;
   const int end_index =
      Min(world->platform_cursor + 30, world->platform_limit);
   for(int i = end_index; i-- > 0;)
   {
      // Special case for drawing ground floor.
      if( world->platform[i].type < 0 )
      {
         assert(i == 0);
         assert(platform[0].x == 0);
         assert(platform[0].y == 0);
         pd->graphics->fillRect(0,
                                world->scroll_offset_y,
                                SCREEN_WIDTH,
                                SCREEN_HEIGHT,
                                kColorBlack);
         return;
      }

      assert(platform[i].type >= 0);
      assert(platform[i].type < 24);
      LCDBitmap *tile = pd->graphics->getTableBitmap(g_platform,
                                                     platform[i].type);
      assert(tile != NULL);
      const int x = platform[i].x + PLATFORM_OFFSET_X;
      const int y = platform[i].y + PLATFORM_OFFSET_Y + world->scroll_offset_y;
      pd->graphics->drawBitmap(tile, x, y, kBitmapUnflipped);

      // Wraparound.
      pd->graphics->drawBitmap(tile,
                               x < 0 ? x + SCREEN_WIDTH : x - SCREEN_WIDTH,
                               y,
                               kBitmapUnflipped);

      if( y >= SCREEN_HEIGHT )
         break;
   }
}

// Draw mechanical springs.
static void DrawSprings(const World *world, PlaydateAPI *pd)
{
   for(int i = world->spring_limit; i-- > 0;)
   {
      const int y =
         world->spring[i].y + SPRING_OFFSET_Y + world->scroll_offset_y;
      if( y < -32 )
         continue;
      if( y >= SCREEN_HEIGHT )
         break;
      LCDBitmap *s = pd->graphics->getTableBitmap(g_spring,
                                                  world->spring[i].frame);
      assert(s != NULL);
      const int x = world->spring[i].x + SPRING_OFFSET_X;
      pd->graphics->drawBitmap(s, x, y, kBitmapUnflipped);

      // Wraparound.
      if( x >= SCREEN_WIDTH - 32 )
      {
         pd->graphics->drawBitmap(s, x - SCREEN_WIDTH, y, kBitmapUnflipped);
      }
      else if( world->spring[i].x < 32 )
      {
         pd->graphics->drawBitmap(s, x + SCREEN_WIDTH, y, kBitmapUnflipped);
      }
   }
}

// Draw meteors.
static void DrawMeteor(const World *world, PlaydateAPI *pd)
{
   for(int i = world->meteor_start; i < world->meteor_end; i++)
   {
      const Meteor *meteor = &(world->meteor[i]);
      LCDBitmap *sprite = pd->graphics->getTableBitmap(g_meteor, meteor->frame);
      assert(sprite != NULL);
      const int x = meteor->x + METEOR_OFFSET_X;
      const int y = meteor->y + METEOR_OFFSET_Y + world->scroll_offset_y;
      pd->graphics->drawBitmap(sprite, x, y, kBitmapUnflipped);
   }
}

// Get Y value of the topmost platform.
static int GetWorldCeiling(const World *world)
{
   assert(world->platform_limit > 0);
   assert(world->platform[world->platform_limit - 1].y <= 0);
   return world->platform[world->platform_limit - 1].y;
}

// Get platform width from platform type.
static int GetPlatformWidth(int type)
{
   if( type < 0 )
      return SCREEN_WIDTH;
   assert(type >= 0);
   assert(type < 24);
   const int t = type % 6;
   return t < 2 ? 128 : t < 4 ? 96 : 64;
}

// Get horizontal range of a platform.  Returns [x0,x1) range via pointer.
static void GetPlatformXRange(const Platform *platform, int *x0, int *x1)
{
   const int width = GetPlatformWidth(platform->type) - 2 * PLATFORM_MARGIN;
   *x0 = platform->x + PLATFORM_MARGIN;
   *x1 = platform->x + PLATFORM_MARGIN + width;
}

// Generate platform velocity given a particular base platform type.
static uint16_t GetPlatformVelocity(int base_type)
{
   assert((base_type % 6) == 0);
   if( base_type >= 12 || RAND(2) > 0 )
      return 0;
   const int vx = RAND_RANGE(-3, 3);
   return vx < 0 ? SCREEN_WIDTH + vx : vx;
}

// Check that the entire platform list is sorted.
#ifndef NDEBUG
static int IsSorted(const World *world)
{
   for(int i = 1; i < world->platform_limit; i++)
   {
      if( world->platform[i - 1].y < world->platform[i].y )
         return 0;
   }
   return 1;
}
#endif

// Move the newly appended platform into the right place.  We don't need to
// do a full sort since we know only the last appended platform is out of
// order, so we just have to move that one into the right place.
static void SortPlatformSuffix(World *world)
{
   int i = world->platform_limit - 1;
   assert(i > 0);
   while( world->platform[i - 1].y < world->platform[i].y )
   {
      i--;
      assert(i > 0);
   }
   const int suffix_length = world->platform_limit - i;
   assert(suffix_length > 0);
   if( suffix_length == 1 )
      return;

   const Platform tmp = world->platform[world->platform_limit - 1];
   memmove(world->platform + i,
           world->platform + i - 1,
           sizeof(Platform) * suffix_length);
   memcpy(world->platform + i, &tmp, sizeof(Platform));
}

// Generate platforms that are simple chains.  In this method, there will be
// one obvious direction as to where to go next.
static void AppendSimpleChain(World *world, int base_type, int diversion_rate)
{
   assert(world->platform_limit > 0);

   // Select a starting point from highest platform.
   int x0, x1;
   GetPlatformXRange(&(world->platform[world->platform_limit - 1]), &x0, &x1);

   // Select new platform type to be placed.
   //
   // In the jam version, type was selected to be weighted toward thinner
   // platforms, which resulted in more narrow ladders going up and lots of
   // empty space between ladders.  We seem to get a more interesting
   // landscape if we just select the types to be uniformly random.
   const int type = base_type + RAND(5);
   assert(type >= 0);
   assert(type < 24);
   const int edge_offset = GetPlatformWidth(type) / 2;

   // Create a ghost slime that stands at a random point on the highest
   // platform, with a random jump angle.
   Slime ghost;
   memset(&ghost, 0, sizeof(Slime));
   ghost.y = GetWorldCeiling(world) << SLIME_FRACTION_BITS;
   ghost.x = RAND_RANGE(x0 << SLIME_FRACTION_BITS,
                        (x1 - 1) << SLIME_FRACTION_BITS);
   ghost.x %= SCREEN_WIDTH << SLIME_FRACTION_BITS;

   ghost.a = RAND_RANGE(360 - 60, 360 + 60) % 360;

   // Simulate this slime jumping until its vertical velocity is heading
   // downward.
   while( ghost.vy <= 0 )
   {
      // Jump is applied repeatedly to ensure full velocity.
      JumpSlime(&ghost);
      UpdateSlime(&ghost);
   }

   // Where this ghost lands will be the center of where we place the
   // new platform.  The +5 adjustment in vertical position is to make
   // the velocity needed to reach the platform less strict.
   Platform *new_platform = &(world->platform[world->platform_limit]);
   new_platform->x =
      ((ghost.x >> SLIME_FRACTION_BITS) - edge_offset + SCREEN_WIDTH) %
      SCREEN_WIDTH;
   new_platform->y = (ghost.y >> SLIME_FRACTION_BITS) + 5;
   new_platform->type = type;
   new_platform->vx = GetPlatformVelocity(base_type);
   new_platform->spring_index = -1;
   assert(new_platform->y < GetWorldCeiling(world));
   world->platform_limit++;
   assert(world->platform_limit <= MAX_PLATFORMS);
   assert(IsSorted(world));

   // Insert a random platform off to the side once in a while, so that
   // we don't have too much empty space in places that stray from the
   // main path.
   if( RAND(diversion_rate) > 0 )
   {
      Platform *diversion = &(world->platform[world->platform_limit++]);
      if( base_type == 12 && RAND(2) == 0 )
      {
         // If base type is rocks, generate a diversion in the form of clouds
         // instead of rocks once in a while, and make it a movable platform.
         diversion->type = RAND_RANGE(6, 11);
         diversion->vx = RAND_RANGE(1, 3);
         if( RAND(1) == 0 )
            diversion->vx = SCREEN_WIDTH - diversion->vx;
      }
      else
      {
         diversion->type = base_type + RAND(5);
         diversion->vx = GetPlatformVelocity(base_type);
      }
      diversion->spring_index = -1;

      // Place the diversion around half a screen away horizontally.  This
      // makes it fill the empty space better.
      diversion->x = (new_platform->x +
                      RAND_RANGE(SCREEN_WIDTH / 4, 3 * SCREEN_WIDTH / 4)) %
                     SCREEN_WIDTH;

      // Place the diversion below the newly added platform so that it won't
      // be considered the top platform after sorting.  This is needed since
      // new paths are continued from the top platform, and we don't want to
      // continue a path off of a diversion because it won't be contiguous.
      diversion->y = new_platform->y + RAND_RANGE(1, 5);

      // If we haven't generated enough springs yet, place one on this
      // diversion.  This gives player some incentive to visit these
      // diversions.
      //
      // The springs are generated at some probability so that they
      // don't appear on every diversion.  That said, we don't want the
      // probability to be too low since the diversions themselves are
      // already generated probabilistically.
      if( world->spring_limit < MAX_SPRINGS && RAND(2) > 0 )
      {
         GetPlatformXRange(diversion, &x0, &x1);
         Spring *new_spring = &(world->spring[world->spring_limit]);
         new_spring->x = RAND_RANGE(x0, x1) % SCREEN_WIDTH;
         new_spring->y = diversion->y;
         new_spring->frame = 0;
         diversion->spring_index = world->spring_limit;
         world->spring_limit++;
      }

      SortPlatformSuffix(world);
      assert(IsSorted(world));
   }
}

// Generate some predefined routes that require backtracking.
static void AppendPredefinedShape(World *world, int base_type)
{
   assert(world->platform_limit > 0);

   // Select a starting point from highest platform.
   int x0, x1;
   GetPlatformXRange(&(world->platform[world->platform_limit - 1]), &x0, &x1);

   // Create a ghost that jumps straight up.
   Slime ghost;
   memset(&ghost, 0, sizeof(Slime));
   const int world_ceiling = GetWorldCeiling(world);
   ghost.y = world_ceiling << SLIME_FRACTION_BITS;
   ghost.x = RAND_RANGE(x0 << SLIME_FRACTION_BITS,
                        (x1 - 1) << SLIME_FRACTION_BITS);
   ghost.x %= SCREEN_WIDTH << SLIME_FRACTION_BITS;
   ghost.a = 0;
   while( ghost.vy <= 0 )
   {
      JumpSlime(&ghost);
      UpdateSlime(&ghost);
   }

   // All platforms in the set get the same velocity, so that their relative
   // positions remain constant.
   const int vx = GetPlatformVelocity(base_type);

   // Append a new narrow platform a few pixels below the ghost's current
   // position, and also measure vertical distance to this platform.
   Platform *new_platform = &(world->platform[world->platform_limit]);
   new_platform->type = base_type + RAND_RANGE(4, 5);
   new_platform->x = ghost.x >> SLIME_FRACTION_BITS;
   new_platform->y = (ghost.y >> SLIME_FRACTION_BITS) + 5;
   new_platform->vx = vx;
   new_platform->spring_index = -1;
   const int vertical_distance =
      world->platform[world->platform_limit - 1].y - new_platform->y;
   world->platform_limit++;
   assert(IsSorted(world));

   // From this new platform, we will append an S-shaped route.
   const int p0y = new_platform->y;
   const int p1y = p0y - vertical_distance / 2;
   const int p2y = p0y - vertical_distance;
   const int p3y = p1y - vertical_distance;
   int p0x = new_platform->x;
   int p1x, p2x, p3x;
   if( RAND(1) == 0 )
   {
      // Left to right.
      //                  [#3#]
      //                    ^
      //  [#####2####]      |
      //   ^        |       |
      //   |        v       |
      //   |       [####1####]
      //   |
      // [#0#]
      p2x = p0x;
      p1x = p2x + GetPlatformWidth(0) - PLATFORM_MARGIN * 2;
      p3x = p1x + GetPlatformWidth(0) - PLATFORM_MARGIN * 2;
   }
   else
   {
      // Right to left.
      // [#3#]
      //   ^
      //   |      [#####2####]
      //   |       |        ^
      //   |       v        |
      //  [####1####]       |
      //                    |
      //                  [#0#]
      p2x = p0x + PLATFORM_MARGIN * 2 - GetPlatformWidth(0);
      p1x = p2x + PLATFORM_MARGIN * 2 - GetPlatformWidth(0);
      p3x = p1x + PLATFORM_MARGIN * 2 - GetPlatformWidth(4);
   }
   new_platform = &(world->platform[world->platform_limit++]);
   new_platform->type = base_type + RAND_RANGE(0, 1);
   new_platform->x = (p1x + SCREEN_WIDTH) % SCREEN_WIDTH;
   new_platform->y = p1y;
   new_platform->vx = vx;
   new_platform->spring_index = -1;

   new_platform = &(world->platform[world->platform_limit++]);
   new_platform->type = base_type + RAND_RANGE(0, 1);
   new_platform->x = (p2x + SCREEN_WIDTH) % SCREEN_WIDTH;
   new_platform->y = p2y;
   new_platform->vx = vx;
   new_platform->spring_index = -1;

   new_platform = &(world->platform[world->platform_limit++]);
   new_platform->type = base_type + RAND_RANGE(4, 5);
   new_platform->x = (p3x + SCREEN_WIDTH) % SCREEN_WIDTH;
   new_platform->y = p3y;
   new_platform->vx = vx;
   new_platform->spring_index = -1;

   assert(IsSorted(world));
}

// Adjust platform_cursor position to be at or below slime Y position.
static void AdjustPlatformCursor(World *world, int slime_y)
{
   assert(slime_y <= 0);
   assert(world->platform_limit > 0);
   assert(world->platform_cursor >= 0);
   assert(world->platform_cursor < world->platform_limit);

   // Move cursor up until we are at a platform that's above the slime.
   while( world->platform[world->platform_cursor].y >= slime_y )
   {
      world->platform_cursor++;

      // Slime can never reach the highest platform, because we always generate
      // new platforms at higher elevations just outside of the view.
      assert(world->platform_cursor < world->platform_limit);
   }

   // Move cursor down until we are at a platform that's at or below the slime.
   while( world->platform[world->platform_cursor].y < slime_y )
   {
      world->platform_cursor--;
      if( world->platform_cursor == 0 )
         return;
   }
   assert(world->platform_cursor + 1 < world->platform_limit);
   assert(world->platform[world->platform_cursor].y >= slime_y);
   assert(world->platform[world->platform_cursor + 1].y < slime_y);
}

// Spawn meteors toward player.
static void SpawnMeteors(World *world)
{
   const int target_x = world->slime.x >> SLIME_FRACTION_BITS;
   const int target_y = (world->slime.y >> SLIME_FRACTION_BITS) -
                        SLIME_CENTER_OFFSET;

   for(; world->meteor_end < world->beat; world->meteor_end++)
   {
      assert(world->meteor_end < MAX_METEORS);
      Meteor *new_meteor = &world->meteor[world->meteor_end];
      new_meteor->frame = RAND_RANGE(0, 17);
      new_meteor->hit = 0;

      // Set velocity.
      if( target_x < SCREEN_WIDTH / 4 )
      {
         new_meteor->vx = RAND_RANGE(-METEOR_MAX_VELOCITY,
                                     -METEOR_MIN_VELOCITY);
      }
      else if( target_x > 3 * SCREEN_WIDTH / 4 )
      {
         new_meteor->vx = RAND_RANGE(METEOR_MIN_VELOCITY, METEOR_MAX_VELOCITY);
      }
      else
      {
         new_meteor->vx = RAND_RANGE(METEOR_MIN_VELOCITY, METEOR_MAX_VELOCITY);
         if( RAND_RANGE(0, 1) == 0 )
            new_meteor->vx = -new_meteor->vx;
      }
      new_meteor->vy = RAND_RANGE(METEOR_MIN_VELOCITY, METEOR_MAX_VELOCITY);

      // Set initial position.
      int t;
      if( new_meteor->vx < 0 )
      {
         const int distance = -(SCREEN_WIDTH + 64 - target_x);
         t = distance / new_meteor->vx;
      }
      else
      {
         const int distance = 64 + target_x;
         t = distance / new_meteor->vx;
      }
      new_meteor->x = target_x - t * new_meteor->vx;
      new_meteor->y = target_y - t * new_meteor->vy;
   }
}

// Animate meteors and garbage collect dead meteors.
static void AnimateMeteors(World *world)
{
   // Center of slime.
   const int target_x = world->slime.x >> SLIME_FRACTION_BITS;
   const int target_y = (world->slime.y >> SLIME_FRACTION_BITS) -
                        SLIME_CENTER_OFFSET;

   for(int i = world->meteor_start; i < world->meteor_end; i++)
   {
      Meteor *meteor = &(world->meteor[i]);
      meteor->x += meteor->vx;
      meteor->y += meteor->vy;

      // If disable_meteors is true, all currently live meteors are
      // immediately moved outside of visible range.  New meteors will
      // still spawn according to current beat number, they will just
      // get removed immediately here.
      //
      // We do this instead of not spawning any meteors at all, so
      // that if the disable_meteors is changed later in the game, we
      // will still spawn meteors at the right beat.
      if( world->disable_meteors )
      {
         meteor->x = SCREEN_WIDTH + 65;
         meteor->vx = 1;
      }

      // Check for collision with slime.
      //
      // Note that each meteor is only eligible for at most one hit.  Once
      // it has collided with a slime, the hit is recorded and the meteor no
      // longer has any effect.  This is to avoid a single meteor pushing
      // the slime continuously as it falls through.
      //
      // In the jam version, we didn't have this check and the meteors were
      // far more brutal, since once you got hit, you pretty much get pushed
      // all the way to the edge with no escape.  You can relive the jam
      // experience by removing the "meteor->hit == 0" condition below and
      // see how that is a more difficult game.
      if( meteor->hit == 0 &&
          abs(meteor->x - target_x) < 16 && abs(meteor->y - target_y) < 16 )
      {
         HitSlime(&(world->slime), meteor->vx, meteor->vy);
         meteor->hit = 1;
      }

      // Expire old meteors that have moved outside of visible range,
      // but note that only the oldest meteor is expired.  This is so
      // that we update a contiguous range of meteors.
      if( meteor->vx > 0 )
      {
         if( meteor->x > SCREEN_WIDTH + 64 )
         {
            if( i == world->meteor_start )
               world->meteor_start++;
         }
         else
         {
            meteor->frame = (meteor->frame + 1) % 18;
         }
      }
      else
      {
         if( meteor->x < -64 )
         {
            if( i == world->meteor_start )
               world->meteor_start++;
         }
         else
         {
            meteor->frame = (meteor->frame + 17) % 18;
         }
      }
   }
}

// Set background color.
static void UpdateBackgroundColor(World *world)
{
   const Platform *platform = world->platform;

   // Find all color indices at each scanline.
   uint8_t background_color[SCREEN_HEIGHT];
   memset(background_color, kGrayLevel[3], SCREEN_HEIGHT);
   const int end_index =
      Min(world->platform_cursor + 30, world->platform_limit);
   for(int i = end_index; i-- > 1;)
   {
      assert(platform[i].type >= 0);
      assert(platform[i].type < 24);
      assert(platform[i].y <= platform[i - 1].y);
      int height = platform[i - 1].y - platform[i].y;
      assert(height >= 0);

      int start_y = platform[i].y + PLATFORM_OFFSET_Y + world->scroll_offset_y;
      if( start_y >= SCREEN_HEIGHT )
         break;
      if( start_y + height < 0 )
         continue;

      if( start_y < 0 )
      {
         height += start_y;
         start_y = 0;
      }
      if( start_y + height > SCREEN_HEIGHT )
         height = SCREEN_HEIGHT - start_y;
      assert(start_y >= 0);
      assert(start_y + height <= SCREEN_HEIGHT);
      memset(background_color + start_y,
             kGrayLevel[platform[i].type / 6],
             height);
   }

   unsigned int average_color = 0;
   for(int i = 0; i < SCREEN_HEIGHT; i++)
      average_color += background_color[i];
   average_color /= SCREEN_HEIGHT;
   assert(average_color >= 0);
   assert(average_color <= 64);
   world->background_color = average_color;
}

// Run a single time step of world+slime updates.
void UpdateWorld(World *world)
{
   // Add new platforms until all visible area is covered.
   //
   // We need to generate platforms ahead of the player so that they will have
   // somewhere to go, but we also want to generate them as late as possible
   // since the type of platform generated depends on current song position,
   // and we don't want the visuals to deviate from the song too much.
   while( GetWorldCeiling(world) +
          kPlatformHeight[world->platform_style] +
          world->scroll_offset_y >= 0 )
   {
      switch( world->platform_style )
      {
         case kPlatformTrees:
            AppendSimpleChain(world, 18, 6);
            break;
         case kPlatformRocks:
            if( RAND(9) == 0 )
               AppendPredefinedShape(world, 12);
            else
               AppendSimpleChain(world, 12, 5);
            break;
         case kPlatformClouds:
            if( RAND(6) == 0 )
               AppendPredefinedShape(world, 6);
            else
               AppendSimpleChain(world, 6, 4);
            break;
         case kPlatformSpace:
            AppendSimpleChain(world, 0, 0);
            break;
      }
   }

   // Update meteors.
   SpawnMeteors(world);
   AnimateMeteors(world);

   // Animate platforms.  All platforms are animated whether they are visible
   // or not.  This is so that the relative position of the platforms remain
   // constant for platforms with the same velocity.
   for(int i = 0; i < world->platform_limit; i++)
   {
      Platform *p = &(world->platform[i]);
      if( LIKELY(p->vx == 0) )
         continue;
      p->x = (p->x + p->vx) % SCREEN_WIDTH;
      if( p->spring_index >= 0 )
      {
         world->spring[p->spring_index].x =
            (world->spring[p->spring_index].x + p->vx) % SCREEN_WIDTH;
      }
   }

   // Apply slime movement.
   const int old_y = world->slime.y >> SLIME_FRACTION_BITS;
   AdjustPlatformCursor(world, old_y);
   const int old_platform_cursor = world->platform_cursor;
   assert(world->platform[old_platform_cursor].y >= old_y);

   UpdateSlime(&(world->slime));
   const int new_y = world->slime.y >> SLIME_FRACTION_BITS;
   AdjustPlatformCursor(world, new_y);

   // Apply collision checks only for downward movement.
   if( new_y > old_y )
   {
      // Check for collision with springs before checking for collision
      // with platforms.
      const int slime_x = world->slime.x >> SLIME_FRACTION_BITS;
      for(int i = world->spring_limit; i-- > 0;)
      {
         // Ignore springs that are out of range, and also reset their
         // compression state.
         const int spring_y = world->spring[i].y;
         if( spring_y < new_y || spring_y > new_y + 24 )
         {
            world->spring[i].frame = 0;
            continue;
         }
         const int spring_x = world->spring[i].x;
         assert(spring_x >= 0 && spring_x < SCREEN_WIDTH);
         assert(slime_x >= 0 && slime_x < SCREEN_WIDTH);
         const int d = abs(spring_x - slime_x);
         if( d > 16 && d < SCREEN_WIDTH - 16 )
         {
            world->spring[i].frame = 0;
            continue;
         }

         // Collided with spring.
         if( world->spring[i].frame < 2 )
         {
            // Spring is still being compressed.  We will reduce the
            // slime's vertical velocity so that it does not pass through
            // the spring while it's being compressed.
            if( spring_y - new_y > 12 )
               world->slime.vy = 1 << SLIME_FRACTION_BITS;
            else
               world->slime.vy = 1 << (SLIME_FRACTION_BITS - 3);
            world->spring[i].frame++;
         }
         else
         {
            // Spring has compressed enough, reset spring and boost slime's
            // vertical velocity.
            world->slime.vy = SPRING_VELOCITY;
            world->spring[i].frame = 0;
         }
      }

      int i = old_platform_cursor;

      // If slime was at the same Y coordinate as its starting platform,
      // then collision with the starting platform does not count.  This
      // allows the slime to drop to a platform below by jumping downward.
      if( world->platform[i].y == old_y )
         i--;
      assert(world->platform_cursor >= 0);
      for(; i >= world->platform_cursor; i--)
      {
         // Stop checking if platform is below slime position.
         if( world->platform[i].y > new_y )
            break;

         const int x0 = world->platform[i].x;
         const int x1 =
            (x0 + GetPlatformWidth(world->platform[i].type)) % SCREEN_WIDTH;
         if( CollideSlime(&(world->slime), x0, x1) )
         {
            LandSlime(&(world->slime), world->platform[i].y);
            break;
         }
      }
   }
   else
   {
      // If slime was stationary and it was sitting on a moving platform,
      // apply the platform's movement to the slime.
      if( UNLIKELY(world->platform[old_platform_cursor].vx != 0) &&
          world->slime.in_flight_time == 0 &&
          world->platform[old_platform_cursor].y == old_y )
      {
         world->slime.x = (world->slime.x +
                           (world->platform[old_platform_cursor].vx
                               << SLIME_FRACTION_BITS)) %
                          (SCREEN_WIDTH << SLIME_FRACTION_BITS);
      }
   }

   // Adjust camera to follow slime.
   // 1. target_offset is set to 3/4 of screen height.  The intent is to keep
   //    the slime near the bottom of the screen with more room to look up.
   //
   // 2. scroll_offset_y is adjusted to converge toward target_offset by
   //    taking the weighted average of the two values.   This scheme causes
   //    scroll_offset_y to smoothly approach target_offset, as opposed to
   //    instantly take on target_offset.
   //
   //    This weighted average scheme also means that if scroll_offset_y is
   //    sufficiently close to target_offset, scroll_offset_y will not change.
   //    So we get a sort of lazy-follow effect.
   //
   // 3. After the weighted average, scroll_offset_y is aligned to even values
   //    so that we always scroll in 2-pixel units.  This reduces flashing.
   //
   // Actually, slime movement is already fairly smooth because it accelerates
   // slowly, so camera movement isn't too jerky even if we don't do any
   // smoothing here.  But this smoothing does make things more stable for
   // small jumps.
   const int target_offset =
      (3 * SCREEN_HEIGHT / 4 - (world->slime.y >> SLIME_FRACTION_BITS));
   world->scroll_offset_y =
      ((7 * world->scroll_offset_y + target_offset) / 8) & ~1;

   // Set background color.
   UpdateBackgroundColor(world);
}

// Draw updated world.
void DrawWorld(const World *world, PlaydateAPI *pd)
{
   DrawBackground(world, pd);
   DrawPlatforms(world, pd);
   DrawSprings(world, pd);
   DrawSlime(&(world->slime), world->scroll_offset_y, pd);
   DrawMeteor(world, pd);

   if( world->slime.y < 0 )
   {
      char *text = NULL;
      const int length = pd->system->formatString(
         &text, "%d", (-world->slime.y) >> SLIME_FRACTION_BITS);
      if( world->background_color < 32 )
      {
         pd->graphics->setDrawMode(kDrawModeFillBlack);
         pd->graphics->drawText(text, length, kASCIIEncoding, 7, 222);
         pd->graphics->setDrawMode(kDrawModeFillWhite);
         pd->graphics->drawText(text, length, kASCIIEncoding, 5, 220);
      }
      else
      {
         pd->graphics->setDrawMode(kDrawModeFillWhite);
         pd->graphics->drawText(text, length, kASCIIEncoding, 7, 222);
         pd->graphics->setDrawMode(kDrawModeFillBlack);
         pd->graphics->drawText(text, length, kASCIIEncoding, 5, 220);
      }
      pd->graphics->setDrawMode(kDrawModeCopy);
      pd->system->realloc(text, 0);
   }
}
