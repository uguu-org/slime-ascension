// Common definitions.
#ifndef COMMON_H_
#define COMMON_H_

#include<stdint.h>
#include<stdlib.h>

#if TARGET_PLAYDATE
   #ifndef NDEBUG
      // Standard assert() doesn't work with console because it requires
      // functions not available in the runtime library that's linked for the
      // device.  When targeting Playdate and assert() is enabled, we will do
      // this hack to make it log to console instead.  This hack requires a
      // pointer to the PlaydateAPI via the g_pd global variable, preferably
      // assigned during setup.
      //
      // We could try to fix assert and get it to halt execution.  A logical
      // thing to try is pd->system->error(), but that appears to kill the
      // process without leaving error messages behind.  Another thing we
      // tried was implementing our own exception handler with setjmp/longjmp,
      // but that appears to crash in mysterious ways, and also doesn't leave
      // any error messages behind.  All things considered, logToConsole will
      // at least get us error messages when the device is connected.
      #include"pd_api.h"

      extern PlaydateAPI *g_pd;
      #define assert(expr)  \
         ((expr) ? (void)0  \
                 : g_pd->system->logToConsole("%s:%d: assertion failed: %s",  \
                                              __FILE__, __LINE__, #expr))
   #else
      #define assert(expr)    ((void)0)
   #endif

#else
   // Use standard assert.h when not targeting the device.
   #include<assert.h>
#endif

// Screen width and height in pixels.
//
// These are same as LCD_COLUMNS and LCD_ROWS from pd_api_gfx.h, but we
// define our own constants here to avoid dependency on Playdate SDK within
// our own library functions when building for simulator.  This makes
// the file easier to test.
#define SCREEN_WIDTH    400
#define SCREEN_HEIGHT   240

// Screen stride in bytes.
//
// This is same as LCD_ROWSIZE from pd_api_gfx.h.
#define SCREEN_STRIDE   52

// Branch prediction hints.
//
// https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html#index-_005f_005fbuiltin_005fexpect
// https://llvm.org/docs/BranchWeightMetadata.html#builtin-expect
#if __has_builtin(__builtin_expect)
   #define LIKELY(x)    __builtin_expect(!!(x), 1)
   #define UNLIKELY(x)  __builtin_expect(!!(x), 0)
#else
   #define LIKELY(x)    x
   #define UNLIKELY(x)  x
#endif

// Unreachable code.
//
// https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html#index-_005f_005fbuiltin_005funreachable
// https://clang.llvm.org/docs/LanguageExtensions.html#builtin-unreachable
#if __has_builtin(__builtin_unreachable)
   #define UNREACHABLE()   __builtin_unreachable()
#else
   #define UNREACHABLE()   assert(0)
#endif

// Generate a random integer in the range of 0..max.
//
// This resorts to doing 64bit math, as opposed to these other alternatives:
//
//    rand() % (max + 1)
//
//       This obviously generates results in the correct range, but the
//       results will be somewhat periodic and not so random.  This is
//       because with the linear congruential generator used by newlib's
//       rand(), the lower bits are less random than the higher bits.
//
//    (max + 1) * rand() / ((unsigned)RAND_MAX + 1U)
//
//       This doesn't work because "(max+1) * rand()" might overflow.
//
//    rand() / (((unsigned)RAND_MAX + 1U) / (max + 1))
//
//       This fixes the overflow problem by dividing-by-reciprocal.  This
//       works if RAND_MAX is large (e.g. 0x7fffffff) relative to max.  But
//       if RAND_MAX is a small, such as 0x7fff used by MingW, we will get
//       results that exceed the expected range.  This has to do with
//       integer divisions producing a smaller denominator than we needed.
//       We could fix this by adjusting the "max+1" part, but it's difficult
//       find divisors that would work at compile time.
//
//    (max + 1) * (float)rand() / ((float)RAND_MAX + 1)
//
//       This is back to moving the multiplier in the numerator, but we
//       convert to floating point first to avoid overflow.  This works, the
//       reason why we are not doing it is because ARM works faster with
//       integers.
//
// https://gcc.godbolt.org/z/q5q39b4rd
//
// Another alternative is to just get a better random number generator
// where we can expect high quality from the lower bits and just do
// "rand() % max".  Well, we are sticking to stdlib one because it's
// very fast, and the higher bits are still usable.
#define RAND(max)    ((int)( ((max)+1) * (uint64_t)rand() \
                             / ((unsigned)RAND_MAX+1U) ))

// Syntactic sugar, generate random number in the range of min..max.
#define RAND_RANGE(min, max)  (RAND((max) - (min)) + (min))

#endif  // COMMON_H_
