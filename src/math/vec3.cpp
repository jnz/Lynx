#include <assert.h>
#include <math.h>
#include <stdio.h> // for fprintf
#include <stdlib.h>
#include "mathconst.h"
#include "vec3.h"
#include "matrix.h"

const vec3_t vec3_t::origin(0,0,0);
const vec3_t vec3_t::xAxis(1.0f, 0.0f, 0.0f);
const vec3_t vec3_t::yAxis(0.0f, 1.0f, 0.0f);
const vec3_t vec3_t::zAxis(0.0f, 0.0f, 1.0f);

vec3_t vec3_t::rand(float mx, float my, float mz)
{
    return vec3_t( mx * ( (float)(::rand()%20000)*0.0001f - 1.0f ),
                   my * ( (float)(::rand()%20000)*0.0001f - 1.0f ),
                   mz * ( (float)(::rand()%20000)*0.0001f - 1.0f ) );
}

float vec3_t::GetAngleDeg(const vec3_t& a, const vec3_t& b)
{
    float d = a.Abs() * b.Abs();
#ifdef _DEBUG
    if(fabsf(d) < lynxmath::EPSILON)
    {
        assert(0);
        return 0.0f;
    }
#endif
    return acosf((a * b) / d) * lynxmath::RADTODEG;
}

void vec3_t::Print() const
{
    fprintf(stderr, "%.3f %.3f %.3f", x, y, z);
}

vec3_t vec3_t::Lerp(const vec3_t& p1, const vec3_t& p2, const float f)
{
    return (1-f)*p1 + f*p2;
}

vec3_t vec3_t::Hermite(const vec3_t& p1, const vec3_t& p2, const vec3_t& T1, const vec3_t& T2, const float t)
{
    const float t2 = t*t;
    const float t3 = t2*t;
    return (1 - 3*t2 + 2*t3)*p1 + t2*(3-2*t)*p2 + t*(t-1)*(t-1)*T1 + t2*(t-1)*T2;
}

bool vec3_t::RayCylinderIntersect(const vec3_t& pStart, const vec3_t& pDir,
                                  const vec3_t& edgeStart, const vec3_t& edgeEnd,
                                  const float radius,
                                  float* f)
{
    // math. for 3d game programming 2nd ed. page 270
    // assert(edgeStart != edgeEnd); // sloppy level design
    const vec3_t pa = edgeEnd - edgeStart;
    const vec3_t s0 = pStart - edgeStart;
    const float pa_squared = pa.AbsSquared();
    const float pa_isquared = 1.0f/pa_squared;

    // a
    const float pva = pDir * pa;
    const float a = pDir.AbsSquared() - pva*pva*pa_isquared;

    // b
    const float b = s0*pDir - (s0*pa)*(pva)*pa_isquared;

    // c
    const float ps0a = s0*pa;
    const float c = s0.AbsSquared() - radius*radius - ps0a*ps0a*pa_isquared;

    const float dis = b*b - a*c;
    if(dis < 0)
        return false;

    *f = (-b - lynxmath::Sqrt(dis))/a;
    const float collision = (pStart + *f*pDir - edgeStart)*pa;
    return collision >= 0 && collision <= pa_squared;
}

bool vec3_t::RaySphereIntersect(const vec3_t& pStart, const vec3_t& pDir,
                                const vec3_t& pSphere, const float radius,
                                float* f)
{
    // calculate in world space
    const vec3_t pEnd = pStart + pDir;
    const float a = (pEnd.x - pStart.x)*(pEnd.x - pStart.x) + (pEnd.y - pStart.y)*(pEnd.y - pStart.y) + (pEnd.z - pStart.z)*(pEnd.z - pStart.z);
    const float b = 2.0f*( (pEnd.x - pStart.x)*(pStart.x - pSphere.x) + (pEnd.y - pStart.y)*(pStart.y - pSphere.y) + (pEnd.z - pStart.z)*(pStart.z - pSphere.z) );
    const float c = pSphere.x*pSphere.x + pSphere.y*pSphere.y + pSphere.z*pSphere.z + pStart.x*pStart.x + pStart.y*pStart.y + pStart.z*pStart.z - 2*(pSphere.x*pStart.x + pSphere.y*pStart.y + pSphere.z*pStart.z) - radius*radius;
    const float discrsquare = b*b - 4.0f*a*c;
    if(discrsquare <= 0.0f)
        return false;

    const float discr = lynxmath::Sqrt(discrsquare);
    const float u1 = (-b + discr)/(2*a);
    const float u2 = -(b + discr)/(2*a);
    *f = u1 < u2 ? u1 : u2;
    return true;
}

