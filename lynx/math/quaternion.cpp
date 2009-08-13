#include <math.h>
#include "quaternion.h"
#include "mathconst.h"
#include <assert.h>

const quaternion_t quaternion_t::identity = quaternion_t(0, 0, 0, 1);

void quaternion_t::FromMatrix(const matrix_t &mx)
{
    assert(0); // nicht implementiert
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

void quaternion_t::GetVec3(vec3_t* dir, vec3_t* up, vec3_t* side)
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

quaternion_t quaternion_t::Invert()
{
	x=-x;
	y=-y;
	z=-z;
	return *this;
}

quaternion_t quaternion_t::Inverse() const
{
	quaternion_t q;
	q = *this;
	q.Invert();
	return q;
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
	return (fabs(a.x-b.x) < lynxmath::EPSILON) &&
		   (fabs(a.y-b.y) < lynxmath::EPSILON) &&
		   (fabs(a.z-b.z) < lynxmath::EPSILON) &&
           (fabs(a.w-b.w) < lynxmath::EPSILON);
}

bool operator != (const quaternion_t& a, const quaternion_t& b)
{
	return (fabs(a.x-b.x) >= lynxmath::EPSILON) ||
           (fabs(a.y-b.y) >= lynxmath::EPSILON) ||
           (fabs(a.z-b.z) >= lynxmath::EPSILON) ||
           (fabs(a.w-b.w) >= lynxmath::EPSILON);
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
	
	if(fabsf(cosom) < 0.999 )
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