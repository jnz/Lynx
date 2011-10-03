#pragma once

#include "mathconst.h"
#include <assert.h>

struct vec3_t
{
    union
    {
        // access vector either by x, y, z or by v[0], v[1], v[2]:
        struct
        {
            float x, y, z;
        };
        float v[3];
    };

    vec3_t() : x(0.0f), y(0.0f), z(0.0f) {}
    vec3_t(float nx, float ny, float nz) : x(nx), y(ny), z(nz) {}
    vec3_t(float all) : x(all), y(all), z(all) {}

    static vec3_t rand(float mx, float my, float mz);

    float Abs(void) const; // absolute value of vector/length
    float AbsSquared(void) const; // absolute value squared
    float AbsFast(void) const; // absolute value of vector with fast sqrt (might be inaccurate)

    void Normalize(void);
    void NormalizeFast(void);
    vec3_t Normalized(void) const;
    vec3_t NormalizedFast(void) const;
    bool IsNormalized() const;
    void SetLength(float length);
    void MaxLength(float length); // if vector length (Abs) is larger than length, set to length

    bool IsNull() const; // is vector 0,0,0? (no epsilon test)
    bool IsNullEpsilon(const float epsilon=lynxmath::EPSILON) const; // is vector 0,0,0? (with epsilon test)
    bool Equals(const vec3_t& cmp, const float epsilon) const;

    bool IsInArea(const vec3_t& min, const vec3_t& max) const;

    static vec3_t Lerp(const vec3_t& p1, const vec3_t& p2, const float f); // Linear interpolation between p1 and p2. f=0 equals the point p1, f=1 equals p2

    static vec3_t Hermite(const vec3_t& p1, const vec3_t& p2, const vec3_t& t1, const vec3_t& t2, const float t); // Hermite curve (3D Game Programming p. 457)

    static float GetAngleDeg(const vec3_t& a, const vec3_t& b); // angle between a and b in degrees

    static bool IsPointInsideTriangle(const vec3_t& v0,
                                      const vec3_t& v1,
                                      const vec3_t& v2,
                                      const vec3_t& point);
    /*
        Helper function: Intersection of ray with cylinder.
        Returns: true, in f is the scale factor to the hit point.
        Calculate hit point: pStart + f*pDir
    */
    static bool RayCylinderIntersect(const vec3_t& pStart, const vec3_t& pDir,
                                     const vec3_t& edgeStart, const vec3_t& edgeEnd,
                                     const float radius,
                                     float* f);
    /*
        Helper function: Intersection ray with sphere
     */
    static bool RaySphereIntersect(const vec3_t& pStart, const vec3_t& pDir,
                                   const vec3_t& pSphere, const float radius,
                                   float* f);

    static const vec3_t origin; // 0,0,0
    static const vec3_t xAxis;  // 1,0,0
    static const vec3_t yAxis;  // 0,1,0
    static const vec3_t zAxis;  // 0,0,1

    static vec3_t cross(const vec3_t& a, const vec3_t& b);
    static float dot(const vec3_t& a, const vec3_t& b);

    void Print() const; // print x,y,z to stderr

    vec3_t &operator += ( const vec3_t &v );
    vec3_t &operator -= ( const vec3_t &v );
    vec3_t &operator *= ( const float &f );
    vec3_t &operator /= ( const float &f );
    vec3_t  operator -  (void) const;
};

LYNX_INLINE float vec3_t::Abs(void) const
{
    return lynxmath::Sqrt(x*x+y*y+z*z);
}

LYNX_INLINE float vec3_t::AbsFast(void) const
{
    return lynxmath::SqrtFast(x*x+y*y+z*z);
}

LYNX_INLINE float vec3_t::AbsSquared(void) const
{
    return x*x+y*y+z*z;
}

LYNX_INLINE void vec3_t::Normalize(void)
{
    const float abssqr = x*x+y*y+z*z;

    assert(!IsNullEpsilon());
    //if(abssqr > lynxmath::EPSILON)
    //{
        const float ilength = lynxmath::InvSqrt(abssqr);
        x *= ilength;
        y *= ilength;
        z *= ilength;
    //}
}

LYNX_INLINE void vec3_t::NormalizeFast(void)
{
    const float abssqr = x*x+y*y+z*z;

    assert(abssqr > lynxmath::EPSILON);
    //if(abssqr > lynxmath::EPSILON)
    //{
        const float ilength = lynxmath::InvSqrtFast(abssqr);
        x *= ilength;
        y *= ilength;
        z *= ilength;
    //}
}

LYNX_INLINE void vec3_t::SetLength(float scalelen)
{
    const float length = Abs();

    assert(length > lynxmath::EPSILON);
    //if(length > lynxmath::EPSILON)
    //{
        const float ilength = scalelen/length;
        x *= ilength;
        y *= ilength;
        z *= ilength;
    //}
}

// if vector length (Abs) is larger than length, set to length
LYNX_INLINE void vec3_t::MaxLength(float length)
{
    const float curlength = Abs();

    if(curlength > length)
    {
        const float ilength = length/curlength;
        x *= ilength;
        y *= ilength;
        z *= ilength;
    }
}

LYNX_INLINE vec3_t vec3_t::Normalized(void) const
{
    const float len = Abs();
    assert(len > lynxmath::EPSILON);
    //if(len < lynxmath::EPSILON)
        //return vec3_t(0.0f, 0.0f, 0.0f);
    const float ilen = 1.0f/len;
    return vec3_t(x*ilen, y*ilen, z*ilen);
}

LYNX_INLINE vec3_t vec3_t::NormalizedFast(void) const
{
    const float ilen = 1.0f/AbsFast();
    assert(!isnan(ilen));
    return vec3_t(x*ilen, y*ilen, z*ilen);
}

LYNX_INLINE bool vec3_t::IsNormalized() const
{
    return fabsf(Abs()-1.0f) < lynxmath::EPSILON;
}

LYNX_INLINE bool vec3_t::IsNull() const
{
    return x==0.0f && y==0.0f && z==0.0f;
}

LYNX_INLINE bool vec3_t::IsNullEpsilon(const float epsilon) const
{
    return Equals(vec3_t::origin, epsilon);
}

LYNX_INLINE bool vec3_t::Equals(const vec3_t& cmp, const float epsilon) const
{
    return (fabs(x-cmp.x) < epsilon) &&
           (fabs(y-cmp.y) < epsilon) &&
           (fabs(z-cmp.z) < epsilon);
}

LYNX_INLINE bool vec3_t::IsInArea(const vec3_t& min, const vec3_t& max) const
{
    return  x >= min.x && x <= max.x &&
            y >= min.y && y <= max.y &&
            z >= min.z && z <= max.z;
}

LYNX_INLINE vec3_t &vec3_t::operator +=(const vec3_t &v)
{
    x+= v.x;
    y+= v.y;
    z+= v.z;
    return *this;
}

LYNX_INLINE vec3_t &vec3_t::operator -=(const vec3_t &v)
{
    x-= v.x;
    y-= v.y;
    z-= v.z;
    return *this;
}

LYNX_INLINE vec3_t &vec3_t::operator *=(const float &f)
{
    x*=f;
    y*=f;
    z*=f;
    return *this;
}

LYNX_INLINE vec3_t &vec3_t::operator /=(const float &f)
{
    const float invf = 1/f;
    x*=invf;
    y*=invf;
    z*=invf;
    return *this;
}

LYNX_INLINE vec3_t vec3_t::operator -(void) const
{
    return vec3_t(-x, -y, -z);
}

LYNX_INLINE vec3_t operator+(vec3_t const &a, vec3_t const &b)
{
    return vec3_t( a.x+b.x, a.y+b.y, a.z+b.z );
}

LYNX_INLINE vec3_t operator-(vec3_t const &a, vec3_t const &b)
{
    return vec3_t( a.x-b.x, a.y-b.y, a.z-b.z );
}

LYNX_INLINE vec3_t operator*(vec3_t const &v, float const &f)
{
    return vec3_t( v.x*f, v.y*f, v.z*f );
}

LYNX_INLINE vec3_t operator*(float const &f, vec3_t const &v)
{
    return vec3_t( v.x*f, v.y*f, v.z*f );
}

LYNX_INLINE vec3_t operator/(vec3_t const &v, float const &f)
{
    const float fi = 1.0f / f;
    return vec3_t( v.x*fi, v.y*fi, v.z*fi );
}

// cross product
LYNX_INLINE vec3_t operator^(vec3_t const &a, vec3_t const &b)
{
    return vec3_t( (a.y*b.z-a.z*b.y),
                   (a.z*b.x-a.x*b.z),
                   (a.x*b.y-a.y*b.x) );
}

LYNX_INLINE vec3_t vec3_t::cross(const vec3_t& a, const vec3_t& b)
{
    return vec3_t( (a.y*b.z-a.z*b.y),
                   (a.z*b.x-a.x*b.z),
                   (a.x*b.y-a.y*b.x) );
}

// dot product
LYNX_INLINE float operator*(vec3_t const &a, vec3_t const &b)
{
    return a.x*b.x+a.y*b.y+a.z*b.z;
}

LYNX_INLINE float vec3_t::dot(vec3_t const &a, vec3_t const &b)
{
    return a.x*b.x+a.y*b.y+a.z*b.z;
}

LYNX_INLINE bool operator==(vec3_t const &a, vec3_t const &b)
{
    return ( (fabs(a.x-b.x) < lynxmath::EPSILON) &&
             (fabs(a.y-b.y) < lynxmath::EPSILON) &&
             (fabs(a.z-b.z) < lynxmath::EPSILON) );
}

LYNX_INLINE bool operator!=(vec3_t const &a, vec3_t const &b)
{
    return ( (fabs(a.x-b.x) > lynxmath::EPSILON) ||
             (fabs(a.y-b.y) > lynxmath::EPSILON) ||
             (fabs(a.z-b.z) > lynxmath::EPSILON) );
}

