#pragma once
#include <math.h>

namespace lynxmath
{
    const float PI = 3.141592654f;
    const float PI_HALF = (PI*0.5f);
    const float SQRT_2_HALF = 0.707106781186547f;
    const float SQRT_2 = 1.4142135623730950488f;
    const float EPSILON = 0.00001f;
    const float DEGTORAD = PI / 180.0f;
    const float RADTODEG = 180.0f / PI;

    inline float InvSqrtFast(float x) // reciprocal square root approximation: 1/sqrt(x)
    {
        // the magic inv square root
        const float xhalf = 0.5f * x;
        long i = *(long*)&x;       // store floating-point bits in integer
        i = 0x5f3759d5 - (i >> 1); // initial guess for Newton's method
        x = *(float*)&i;           // convert new bits into float
        x = x*(1.5f - xhalf*x*x);  // One round of Newton's method
        return x;
    }

    inline float SqrtFast(float x) // square root approximation
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

    inline float InvSqrt(float x) // 1/sqrt(x)
    {
        return 1/sqrt(x);
    }

    inline float Sqrt(float x)
    {
        return sqrt(x);
    }
}
