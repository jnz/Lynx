#include <math.h>
#include "quaternion.h"
#include "mathconst.h"
#include <assert.h>

const quaternion_t quaternion_t::identity = quaternion_t(0, 0, 0, 1.0f);

void quaternion_t::FromMatrix(const matrix_t &mx)
{
    const float tr = mx.m[0][0] + mx.m[1][1] + mx.m[2][2];

    if(tr > 0)
    {
        const float S = lynxmath::Sqrt(tr+1.0f) * 2.0f; // S=4*qw
        w = 0.25f * S;
        x = (mx.m[1][2] - mx.m[2][1]) / S;
        y = (mx.m[2][0] - mx.m[0][2]) / S;
        z = (mx.m[0][1] - mx.m[1][0]) / S;
    }
    else if((mx.m[0][0] > mx.m[1][1])&&(mx.m[0][0] > mx.m[2][2]))
    {
        const float S = lynxmath::Sqrt(1.0f + mx.m[0][0] - mx.m[1][1] - mx.m[2][2]) * 2.0f; // S=4*qx
        w = (mx.m[1][2] - mx.m[2][1]) / S;
        x = 0.25f * S;
        y = (mx.m[1][0] + mx.m[0][1]) / S;
        z = (mx.m[2][0] + mx.m[0][2]) / S;
    }
    else if (mx.m[1][1] > mx.m[2][2])
    {
        const float S = lynxmath::Sqrt(1.0f + mx.m[1][1] - mx.m[0][0] - mx.m[2][2]) * 2.0f; // S=4*qy
        w = (mx.m[2][0] - mx.m[0][2]) / S;
        x = (mx.m[1][0] + mx.m[0][1]) / S;
        y = 0.25f * S;
        z = (mx.m[2][1] + mx.m[1][2]) / S;
    }
    else
    {
        const float S = lynxmath::Sqrt(1.0f + mx.m[2][2] - mx.m[0][0] - mx.m[1][1]) * 2.0f; // S=4*qz
        w = (mx.m[0][1] - mx.m[1][0]) / S;
        x = (mx.m[2][0] + mx.m[0][2]) / S;
        y = (mx.m[2][1] + mx.m[1][2]) / S;
        z = 0.25f * S;
    }
}

void quaternion_t::ToMatrix(matrix_t& mx) const
{
    mx.m[0][3] = 0.0f;
    mx.m[1][3] = 0.0f;
    mx.m[2][3] = 0.0f;
    mx.m[3][0] = 0.0f;
    mx.m[3][1] = 0.0f;
    mx.m[3][2] = 0.0f;
    mx.m[3][3] = 1.0f;

    mx.m[0][0] = 1.0f - 2.0f*(y*y + z*z);
    mx.m[0][1] = 2.0f * (x*y + w*z);
    mx.m[0][2] = 2.0f * (x*z - w*y);

    mx.m[1][0] = 2.0f * (x*y - w*z);
    mx.m[1][1] = 1.0f - 2.0f * (x*x + z*z);
    mx.m[1][2] = 2.0f * (y*z + w*x);

    mx.m[2][0] = 2.0f * (x*z + w*y);
    mx.m[2][1] = 2.0f * (y*z - w*x);
    mx.m[2][2] = 1.0f - 2.0f * (x*x + y*y);
}

void quaternion_t::GetVec3(vec3_t* dir, vec3_t* up, vec3_t* side) const
{
    if(side)
    {
        side->x = 1.0f - 2.0f*(y*y + z*z);
        side->y = 2.0f * (x*y + w*z);
        side->z = 2.0f * (x*z - w*y);
    }

    if(up)
    {
        up->x = 2.0f * (x*y - w*z);
        up->y = 1.0f - 2.0f * (x*x + z*z);
        up->z = 2.0f * (y*z + w*x);
    }

    if(dir)
    {
        dir->x = 2.0f * (x*z + w*y);
        dir->y = 2.0f * (y*z - w*x);
        dir->z = 1.0f - 2.0f * (x*x + y*y);
    }
}

void quaternion_t::LookAt(const vec3_t& pFrom, const vec3_t& pAt, const vec3_t& pUp)
{
    assert(0); // Is this function OK? Currently untested and seems not so clever

    assert(pUp.IsNormalized());
    const vec3_t dir = (pAt - pFrom).Normalized();
    const vec3_t side = (dir ^ pUp).Normalized();
    const vec3_t up = (side ^ dir).Normalized();

    matrix_t m;
    m.SetIdentity();
    m.m[0][0] =  side.x;
    m.m[0][1] =  side.y;
    m.m[0][2] =  side.z;
    m.m[1][0] =  up.x;
    m.m[1][1] =  up.y;
    m.m[1][2] =  up.z;
    m.m[2][0] = -dir.x;
    m.m[2][1] = -dir.y;
    m.m[2][2] = -dir.z;

    FromMatrix(m);
}

void quaternion_t::RotationAxis(vec3_t v, float a)
{
    const float s = (float)sin(a/2);
    const float c = (float)cos(a/2);

    v.Normalize();
    x = s * v.x;
    y = s * v.y;
    z = s * v.z;
    w = c;
}

void quaternion_t::Invert()
{
    x=-x;
    y=-y;
    z=-z;
}

quaternion_t quaternion_t::Inverse() const
{
    quaternion_t q;
    q = *this;
    q.Invert();
    return q;
}

void quaternion_t::Normalize()
{
    const float abssqr = x*x + y*y* + z*z + w*w;
    if(fabsf(abssqr - 1.0f) > lynxmath::EPSILON ||
       fabsf(abssqr) < lynxmath::EPSILON)
    {
        assert(0);
        return;
    }
    const float invabs = lynxmath::InvSqrt(abssqr);
    x *= invabs;
    y *= invabs;
    z *= invabs;
    w *= invabs;
}

quaternion_t quaternion_t::Normalized() const
{
    quaternion_t q(*this);
    q.Normalize();
    return q;
}

bool quaternion_t::IsNormalized() const
{
    return (fabsf(x*x + y*y* + z*z + w*w - 1.0f) < lynxmath::EPSILON);
}

// compute w from x, y, z, assuming a normal quaternion
void quaternion_t::ComputeW()
{
    const float t = 1.0f - (x*x) - (y*y) - (z*z);

    if (t < 0.0f)
        w = 0.0f;
    else
        w = -lynxmath::Sqrt(t);
}

quaternion_t quaternion_t::operator *(const quaternion_t &q) const
{
    const float rw = (w * q.w) - (x * q.x) - (y * q.y) - (z * q.z);
    const float rx = (y * q.z) - (z * q.y) + (w * q.x) + (x * q.w);
    const float ry = (z * q.x) - (x * q.z) + (w * q.y) + (y * q.w);
    const float rz = (x * q.y) - (y * q.x) + (w * q.z) + (z * q.w);

    return quaternion_t(rx, ry, rz, rw);
}

quaternion_t quaternion_t::operator =(const quaternion_t &q)
{
    x=q.x;
    y=q.y;
    z=q.z;
    w=q.w;
    return *this;
}

bool operator == (const quaternion_t& a, const quaternion_t& b)
{
    return (fabsf(a.x-b.x) < lynxmath::EPSILON) &&
           (fabsf(a.y-b.y) < lynxmath::EPSILON) &&
           (fabsf(a.z-b.z) < lynxmath::EPSILON) &&
           (fabsf(a.w-b.w) < lynxmath::EPSILON);
}

bool operator != (const quaternion_t& a, const quaternion_t& b)
{
    return (fabsf(a.x-b.x) >= lynxmath::EPSILON) ||
           (fabsf(a.y-b.y) >= lynxmath::EPSILON) ||
           (fabsf(a.z-b.z) >= lynxmath::EPSILON) ||
           (fabsf(a.w-b.w) >= lynxmath::EPSILON);
}

float quaternion_t::ScalarMultiply(const quaternion_t &q1, const quaternion_t &q2)
{
    return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
}

void quaternion_t::Slerp(quaternion_t *pDest, const quaternion_t& q1, const quaternion_t& q2, const float t)
{
    float to1[4];
    float omega, cosom, sinom, scale0, scale1;

    // calc cosine
    cosom = q1.x * q2.x + q1.y * q2.y  + q1.z * q2.z + q1.w * q2.w;

      // adjust signs (if necessary)
    if(cosom < 0.0)
    {
        cosom = -cosom;
        to1[0] = - q2.x;
        to1[1] = - q2.y;
        to1[2] = - q2.z;
        to1[3] = - q2.w;
    }
    else
    {
        to1[0] = q2.x;
        to1[1] = q2.y;
        to1[2] = q2.z;
        to1[3] = q2.w;
    }

    if(cosom > 1)
        cosom = 1;

    if(fabsf(cosom) < 0.99f)
    {
        // standard case (slerp)
        omega = (float)acos(cosom);
        sinom = (float)sin(omega);
        scale0 = (float)sin((1.0 - t) * omega) / sinom;
        scale1 = (float)sin(t * omega) / sinom;
    }
    else
    {
        // "from" and "to" quaternions are very close
        //  ... so we can do a linear interpolation
        scale0 = 1.0f - t;
        scale1 = t;
    }

    pDest->x = scale0 * q1.x + scale1 * to1[0];
    pDest->y = scale0 * q1.y + scale1 * to1[1];
    pDest->z = scale0 * q1.z + scale1 * to1[2];
    pDest->w = scale0 * q1.w + scale1 * to1[3];
}
