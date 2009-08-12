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

    quaternion_t(vec3_t v, float a) // v = axis, a = angle (rad)
    {
        RotationAxis(v, a);
    }

	void FromMatrix(const matrix_t &mx);
	void ToMatrix(matrix_t& m) const;
    void GetVec3(vec3_t* dir, vec3_t* up, vec3_t* side);

    void RotationAxis(vec3_t v, float a);

    quaternion_t Invert();
	quaternion_t Inverse() const;

	quaternion_t operator *(const quaternion_t& q) const;
	quaternion_t operator =(const quaternion_t& q);

    static float ScalarMultiply(const quaternion_t &q1, const quaternion_t &q2);
	static void Slerp(quaternion_t *pDest, const quaternion_t& q1, const quaternion_t& q2, const float t);
};

bool operator == (const quaternion_t& a, const quaternion_t& b);
bool operator != (const quaternion_t& a, const quaternion_t& b);
