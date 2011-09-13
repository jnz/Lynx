#pragma once

#include "vec3.h"
#include "matrix.h"

struct quaternion_t
{
    float x, y, z, w;
    static const quaternion_t identity;

    quaternion_t()
    {
        *this = identity;
    }

    quaternion_t(float _x, float _y, float _z, float _w)
    {
        x=_x; y=_y; z=_z; w=_w;
    }

    quaternion_t(const quaternion_t& src)
    {
        x=src.x; y=src.y; z=src.z; w=src.w;
    }

    quaternion_t(const vec3_t& v, float a) // v = axis, a = angle (rad)
    {
        RotationAxis(v, a);
    }

    quaternion_t(const quaternion_t& q1, const quaternion_t& q2, const float t) // slerp constructor
    {
        Slerp(this, q1, q2, t);
    }

    void FromMatrix(const matrix_t &mx);
    void ToMatrix(matrix_t& m) const;
    void GetVec3(vec3_t* dir, vec3_t* up, vec3_t* side) const;

    void LookAt(const vec3_t& pFrom, const vec3_t& pAt, const vec3_t& pUp);

    void RotationAxis(vec3_t v, float a); // a in [rad]

    void Invert();
    quaternion_t Inverse() const;

    void Normalize();
    bool IsNormalized() const;

    quaternion_t operator *(const quaternion_t& q) const;
    quaternion_t operator =(const quaternion_t& q);

    void Vec3Multiply(const vec3_t& vin, vec3_t* vout) const;

    void ComputeW(); // compute w from x, y, z, assuming a normal quaternion

    static float ScalarMultiply(const quaternion_t &q1, const quaternion_t &q2);
    static void Slerp(quaternion_t *pDest, const quaternion_t& q1, const quaternion_t& q2, const float t);
};

bool operator == (const quaternion_t& a, const quaternion_t& b);
bool operator != (const quaternion_t& a, const quaternion_t& b);
