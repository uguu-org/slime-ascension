#include<stdint.h>
#include<stdlib.h>

#include"pd_api.h"

#include"common.h"
#include"bgm.h"
#include"slime.h"
#include"world.h"

// Syntactic sugar.
//
// All button presses are given the same interpretation in this game.
// Assuming that the crank is in good condition, this game should be
// playable as long as there is at least one working button remaining.
#define ANY_BUTTON   (kButtonA | kButtonB | \
                      kButtonUp | kButtonDown | kButtonLeft | kButtonRight)

// Saved pointer to PlaydateAPI, used for our assert hack.  See common.h.
#if TARGET_PLAYDATE
#ifndef NDEBUG
PlaydateAPI *g_pd;
#endif
#endif

// Version info.
#include"build/version.txt"

// Current game state.
typedef enum
{
   kTitleScreen,
   kGameInProgress,
   kGameOver
} GameState;
static GameState g_game_state;

// Accelerometer state.
typedef enum
{
   kAccelerometerDisabled,
   kAccelerometerStarting,
   kAccelerometerEnabled,
   kAccelerometerStopping,
} AccelerometerState;
static AccelerometerState g_accelerometer_state;

// Control mode names.
static const char *kControlModes[2] = {"crank", "tilt"};

// World state.
static World g_world;

// Loaded font.
static LCDFont *g_bold_font = NULL;

// Image handles.
static LCDBitmap *g_title = NULL;
static LCDBitmap *g_info = NULL;

// Menu options.
static PDMenuItem *g_control_mode = NULL;
static PDMenuItem *g_meteor_enabled = NULL;

// Initialize font.
static void LoadFont(PlaydateAPI *pd)
{
   static const char kFontPath[] = "/System/Fonts/Asheville-Sans-14-Bold.pft";

   const char *error;
   g_bold_font = pd->graphics->loadFont(kFontPath, &error);
   if( g_bold_font == NULL )
      pd->system->error("Error loading %s: %s", kFontPath, error);
   else
      pd->graphics->setFont(g_bold_font);
}

// Load title image.
static void LoadTitle(PlaydateAPI *pd)
{
   const char *error;
   g_title = pd->graphics->loadBitmap("title", &error);
   assert(g_title != NULL);
}

// Draw black text on white rectangle.
static void DrawBoxedText(PlaydateAPI *pd, const char *text, int x, int y)
{
   const int length = strlen(text);
   const int text_width = pd->graphics->getTextWidth(
      g_bold_font,
      text,
      length,
      kASCIIEncoding,
      pd->graphics->getTextTracking());

   pd->graphics->fillRect(x, y, text_width + 20, 25, kColorWhite);
   pd->graphics->drawText(text, length, kASCIIEncoding, x + 10, y + 5);
}

// Initialize pause menu image.
static void SetMenuImage(PlaydateAPI *pd)
{
   if( g_info != NULL )
      return;
   g_info = pd->graphics->newBitmap(LCD_COLUMNS, LCD_ROWS, kColorClear);
   pd->graphics->pushContext(g_info);

   // Shaded background.
   static const LCDPattern kShade =
   {
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xaa, 0xff, 0x55, 0xff, 0xaa, 0xff, 0x55
   };
   pd->graphics->fillRect(0, 0, LCD_COLUMNS, 194, (LCDColor)kShade);
   pd->graphics->fillRect(0, 194, LCD_COLUMNS, 46, kColorWhite);

   // Control instructions.
   DrawBoxedText(pd, "Crank mode:", 1, 1);
   DrawBoxedText(pd, "crank to set direction", 7, 31);
   DrawBoxedText(pd, "press button to jump", 7, 53);
   DrawBoxedText(pd, "Tilt mode:", 1, 91);
   DrawBoxedText(pd, "tilt to set direction", 7, 121);
   DrawBoxedText(pd, "jumps continuously", 7, 143);

   // Version information.
   static const char kContact[] = "omoikane@uguu.org";
   pd->graphics->drawText(kVersion, strlen(kVersion), kASCIIEncoding, 4, 198);
   pd->graphics->drawText(kContact, strlen(kContact), kASCIIEncoding, 4, 220);

   pd->graphics->popContext();
   pd->system->setMenuImage(g_info, 0);
}

// Reset game to title screen.
static void Reset(void *userdata)
{
   PlaydateAPI *pd = userdata;

   StopBackgroundMusic(pd);
   g_game_state = kTitleScreen;
   ResetWorld(&g_world);
}

// Change control mode.
static void SetControlMode(void *userdata)
{
   PlaydateAPI *pd = userdata;

   if( pd->system->getMenuItemValue(g_control_mode) == 1 )
      g_accelerometer_state = kAccelerometerStarting;
   else
      g_accelerometer_state = kAccelerometerStopping;
}

// Toggle falling meteors.
static void ToggleMeteors(void *userdata)
{
   PlaydateAPI *pd = userdata;
   g_world.disable_meteors = !(pd->system->getMenuItemValue(g_meteor_enabled));
}

// Read direction based on mode config.
static int GetDirection(PlaydateAPI *pd)
{
   // Use crank angle by default.
   int angle = pd->system->getCrankAngle();
   switch( g_accelerometer_state )
   {
      case kAccelerometerDisabled:
         // Control is in crank mode, so we don't need to do anything more.
         break;

      case kAccelerometerEnabled:
         // Control is in tilt mode.  Here we will emulate crank angle
         // using accelerometer X reading.
         {
            float x, ignored_y, ignored_z;
            pd->system->getAccelerometer(&x, &ignored_y, &ignored_z);
            if( x < 0 )
               angle = x <= -1 ? 180 : 360 + x * 180;
            else
               angle = x >= 1 ? 180 : x * 180;
         }
         break;

      case kAccelerometerStarting:
         // Transitioning from crank mode to tilt mode.
         pd->system->setPeripheralsEnabled(kAccelerometer);
         g_accelerometer_state = kAccelerometerEnabled;
         break;

      case kAccelerometerStopping:
         // Transitioning from tilt mode to crank mode.
         pd->system->setPeripheralsEnabled(kNone);
         g_accelerometer_state = kAccelerometerDisabled;
         break;
   }
   assert(angle >= 0);
   angle %= 360;
   return angle;
}

// Update and draw the world in title screen state.
static void UpdateTitleScreen(PlaydateAPI *pd)
{
   // Update and draw world.  Update needs to run for at least one
   // frame to get the world populated.  All updates after that will
   // be mostly no-op since we are not accepting input yet.  (Mostly,
   // because we still update things such as scroll offsets).
   UpdateWorld(&g_world);
   DrawWorld(&g_world, pd);

   // Show title logo and other info text.
   pd->graphics->drawBitmap(g_title, 32, 20, kBitmapUnflipped);

   DrawBoxedText(pd, "press A to start", 130, 180);

   pd->graphics->setDrawMode(kDrawModeFillWhite);
   static const char kInfo1[] = "PlayJam 8 \"Ascension\"";
   static const char kInfo2[] = "(c)2025 uguu.org";
   pd->graphics->drawText(kInfo1, strlen(kInfo1), kASCIIEncoding, 5, 220);
   pd->graphics->drawText(kInfo2, strlen(kInfo2), kASCIIEncoding, 267, 220);
   pd->graphics->setDrawMode(kDrawModeCopy);

   // Start/stop accelerometer in response to menu changes.
   (void)GetDirection(pd);

   // Handle input.
   PDButtons current, pushed, released;
   pd->system->getButtonState(&current, &pushed, &released);
   if( (pushed & ANY_BUTTON) != 0 )
   {
      g_game_state = kGameInProgress;
      PlayBackgroundMusic(pd);
   }
}

// Update the world while game is in progress.
static void UpdateGameInProgress(PlaydateAPI *pd)
{
   // Synchronize beats and also determine game over condition.
   const int beat = GetSongBeat(pd);
   g_world.beat = beat & 0xffff;
   assert((beat >> 16) >= g_world.platform_style);
   switch( beat >> 16 )
   {
      case 0: g_world.platform_style = kPlatformTrees; break;
      case 1: g_world.platform_style = kPlatformRocks; break;
      case 2: g_world.platform_style = kPlatformClouds; break;
      case 3: g_world.platform_style = kPlatformSpace; break;
      default:
         #ifndef NDEBUG
            // Log extra stats to console when transitioning to game over state.
            {
               int movable_platforms = 0;
               for(int i = 0; i < g_world.platform_limit; i++)
               {
                  if( g_world.platform[i].vx != 0 )
                     movable_platforms++;
               }
               pd->system->logToConsole(
                  "world: platform_cursor=%d, platform_limit=%d, ceiling=%d, "
                  "scroll_offset_y=%d, meteor_start=%d, meteor_end=%d, "
                  "spring_limit=%d, movable_platforms=%d",
                  g_world.platform_cursor,
                  g_world.platform_limit,
                  g_world.platform[g_world.platform_limit - 1].y,
                  g_world.scroll_offset_y,
                  g_world.meteor_start,
                  g_world.meteor_end,
                  g_world.spring_limit,
                  movable_platforms);
               pd->system->logToConsole(
                  "slime: xy=(%d,%d), vxy=(%d,%d), peak=%d, max_fall=%d",
                  g_world.slime.x,
                  g_world.slime.y,
                  g_world.slime.vx,
                  g_world.slime.vy,
                  g_world.slime.peak,
                  g_world.slime.max_fall);
            }
         #endif
         g_game_state = kGameOver;
         break;
   }

   // Update and draw world.
   UpdateWorld(&g_world);
   DrawWorld(&g_world, pd);

   // Handle input.
   g_world.slime.a = GetDirection(pd);
   if( g_accelerometer_state != kAccelerometerEnabled )
   {
      // When control is in crank mode, slime jumps on button press, and
      // will jump continuously if button is held.
      PDButtons current, pushed, released;
      pd->system->getButtonState(&current, &pushed, &released);
      if( (current & ANY_BUTTON) != 0 )
         JumpSlime(&(g_world.slime));
   }
   else
   {
      // When control is in tilt mode, slime behaves as if the buttons are
      // permanently held, and will jump continuously.
      JumpSlime(&(g_world.slime));
   }
}

// Show a single line of stats for game over screen.
static void ShowSlimeStat(PlaydateAPI *pd,
                          const char *template,
                          int fixe_point_value,
                          int y)
{
   char *text = NULL;
   pd->system->formatString(
      &text, template, fixe_point_value >> SLIME_FRACTION_BITS);
   DrawBoxedText(pd, text, 10, y);
   pd->system->realloc(text, 0);
}

// Draw the world without updates when game is over.
static void UpdateGameOver(PlaydateAPI *pd)
{
   // Draw world without updates.
   DrawWorld(&g_world, pd);

   // Show stats and "return to title" text.
   ShowSlimeStat(pd, "Final height %d", -g_world.slime.y, 15);
   ShowSlimeStat(pd, "Peak height %d", -g_world.slime.peak, 47);
   ShowSlimeStat(pd, "Longest free fall %d", g_world.slime.max_fall, 79);

   static const char kReturnToTitle[] = "press A to return to title";
   pd->graphics->fillRect(198, 215, 202, 25, kColorBlack);
   pd->graphics->setDrawMode(kDrawModeFillWhite);
   pd->graphics->drawText(kReturnToTitle, strlen(kReturnToTitle),
                          kASCIIEncoding, 208, 220);
   pd->graphics->setDrawMode(kDrawModeCopy);

   // Start/stop accelerometer in response to menu changes.
   (void)GetDirection(pd);

   // Handle input.
   PDButtons current, pushed, released;
   pd->system->getButtonState(&current, &pushed, &released);
   if( (pushed & ANY_BUTTON) != 0 )
      Reset(pd);
}

// Draw a single frame.
static int Update(void *userdata)
{
   PlaydateAPI *pd = userdata;

   switch( g_game_state )
   {
      case kTitleScreen:    UpdateTitleScreen(pd);    break;
      case kGameInProgress: UpdateGameInProgress(pd); break;
      case kGameOver:       UpdateGameOver(pd);       break;
   }

   #ifndef NDEBUG
      pd->system->drawFPS(0, 0);
   #endif
   pd->graphics->markUpdatedRows(0, LCD_ROWS - 1);
   return 1;
}

#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI *pd, PDSystemEvent event, uint32_t unused_arg)
{
   // Check for consistency of constants between our header files and
   // Playdate SDK.
   assert(SCREEN_WIDTH == LCD_COLUMNS);
   assert(SCREEN_HEIGHT == LCD_ROWS);
   assert(SCREEN_STRIDE == LCD_ROWSIZE);

   switch( event )
   {
      case kEventInit:
         #if TARGET_PLAYDATE
         #ifndef NDEBUG
            g_pd = pd;
         #endif
         #endif
         srand(pd->system->getSecondsSinceEpoch(NULL));

         pd->system->setUpdateCallback(Update, pd);
         pd->display->setRefreshRate(30);

         pd->system->addMenuItem("reset", Reset, pd);
         g_control_mode = pd->system->addOptionsMenuItem(
            "control", kControlModes, 2, SetControlMode, pd);

         // I called those falling things "meteors" internally to avoid
         // conflict with the rocky platforms.  But most players prefer
         // to call those falling meteors "rocks" since they don't need a
         // name for those platforms, so here the menu says "rocks".
         g_meteor_enabled = pd->system->addCheckmarkMenuItem(
            "rocks", 1, ToggleMeteors, pd);

         LoadFont(pd);
         LoadSlime(pd);
         LoadWorld(pd);
         LoadTitle(pd);
         Reset(pd);
         break;

      case kEventPause:
         SetMenuImage(pd);
         break;

      default:
         break;
   }
   return 0;
}
