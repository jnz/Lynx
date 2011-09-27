#pragma once
#include <math.h>

#if defined(WIN32) || defined(_WIN32)
#define LYNX_INLINE    __forceinline
#else
#define LYNX_INLINE    inline
#endif

namespace lynxmath
{
    const float PI = 3.141592654f;
    const float PI_HALF = (PI*0.5f);
    const float SQRT_2_HALF = 0.707106781186547f;
    const float SQRT_2 = 1.4142135623730950488f;
    const float EPSILON = 0.00001f;
    const float DEGTORAD = PI / 180.0f;
    const float RADTODEG = 180.0f / PI;

    // once key open question is:
    // is this fast square root stuff really faster than
    // a normal sqrt(x) call? this stuff was used in the 90s quake engine
    // but we don't use 486 and Pentium I CPUs anymore.

    LYNX_INLINE float InvSqrtFast(float x) // reciprocal square root approximation: 1/sqrt(x)
    {
        // the magic inv square root
        const float xhalf = 0.5f * x;
        long i = *(long*)&x;       // store floating-point bits in integer
        i = 0x5f3759d5 - (i >> 1); // initial guess for Newton's method
        x = *(float*)&i;           // convert new bits into float
        x = x*(1.5f - xhalf*x*x);  // One round of Newton's method
        return x;
    }

    LYNX_INLINE float SqrtFast(float x) // square root approximation
    {
        // this time with better comments (from quake 3)
        const float original_x = x;
        const float xhalf = 0.5f * x;
        long i = *(long*)&x;           // evil floating point bit level hacking
        i = 0x5f3759d5 - (i >> 1);     // what the fuck?
        x = *(float*)&i;
        x = x*(1.5f - xhalf*x*x);
        return original_x * x;         // x * 1/sqrt(x) = x*sqrt(x)/(sqrt(x)*sqrt(x)) = x*sqrt(x)/x = sqrt(x)
    }

    LYNX_INLINE float InvSqrt(float x) // 1/sqrt(x)
    {
        return 1.0f/sqrt(x);
    }

    LYNX_INLINE float Sqrt(float x)
    {
        return sqrt(x);
    }
}

