#pragma once

namespace lynxmath
{
    const float PI = 3.141592654f;
    const float PI_HALF = (PI*0.5f);
    const float SQRT_2_HALF = 0.707106781186547f;
    const float SQRT_2 = 1.4142135623730950488f;
    const float EPSILON = 0.00001f;
    const float DEGTORAD = PI / 180.0f;
    const float RADTODEG = 180.0f / PI;

    inline float InvSqrt(float x) // Quake's fast invsqrt
    {
        float xhalf = 0.5f * x;
        int i = *(int*)&x; // store floating-point bits in integer
        i = 0x5f3759d5 - (i >> 1); // initial guess for Newton's method
        x = *(float*)&i; // convert new bits into float
        x = x*(1.5f - xhalf*x*x); // One round of Newton's method
        return x;
    }
    inline float Sqrt(float x)
    {
        return x * InvSqrt(x);
    }
}
