#pragma once

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

    float Abs(void) const; // absolute value of vector
    float AbsSquared(void) const; // absolute value squared
    float AbsFast(void) const; // absolute value of vector with fast sqrt (might be inaccurate)

    void Normalize(void);
    vec3_t Normalized(void) const;
    vec3_t NormalizedFast(void) const;
    bool IsNormalized() const;
    void SetLength(float length);

    bool IsNull() const; // is vector 0,0,0? (no epsilon test)
    bool Equals(const vec3_t& cmp, const float epsilon) const;

    bool IsInArea(const vec3_t& min, const vec3_t& max) const;

    static vec3_t Lerp(const vec3_t& p1, const vec3_t& p2, const float f); // Linear interpolation between p1 and p2. f=0 equals the point p1, f=1 equals p2

    static vec3_t Hermite(const vec3_t& p1, const vec3_t& p2, const vec3_t& t1, const vec3_t& t2, const float t); // Hermite curve (3D Game Programming p. 457)

    /*
        Quake style AngleVec3 (YXZ rotation)
        angles x = pitch, y = yaw, z = roll
        if angles is 0/0/0, forward points to (0,0,-1)
    */
    static void AngleVec3(const vec3_t& angles, vec3_t* forward, vec3_t* up, vec3_t* side);
    static float GetAngleDeg(const vec3_t& a, const vec3_t& b); // angle between a and b in degrees

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

    static const vec3_t cross(const vec3_t& a, const vec3_t& b);

    void Print() const; // print x,y,z to stderr

    vec3_t &operator += ( const vec3_t &v );
    vec3_t &operator -= ( const vec3_t &v );
    vec3_t &operator *= ( const float &f );
    vec3_t &operator /= ( const float &f );
    vec3_t  operator -  (void) const;
};

const vec3_t operator+(vec3_t const &a, vec3_t const &b);
const vec3_t operator-(vec3_t const &a, vec3_t const &b);

const vec3_t operator*(vec3_t const &v, float const &f);
const vec3_t operator*(float const &f, vec3_t const &v);

const vec3_t operator/(vec3_t const &v, float const &f);

// cross product
const vec3_t operator^(vec3_t const &a, vec3_t const &b);

// dot product
const float operator*(vec3_t const &a, vec3_t const &b);

bool   operator==(vec3_t const &a, vec3_t const &b);
bool   operator!=(vec3_t const &a, vec3_t const &b);

