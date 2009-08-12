#pragma once

#include "vec3.h"
#include "mathconst.h"

enum pointplane_t { POINTPLANE_BACK=0, POINT_ON_PLANE, POINTPLANE_FRONT }; // do not change order

struct plane_t
{
	plane_t(void);
	plane_t(vec3_t p1, vec3_t p2, vec3_t p3);
	plane_t(vec3_t p, vec3_t n);
	plane_t(float a, float b, float c, float d);

	void SetupPlane(const vec3_t& p1, 
					const vec3_t& p2, 
					const vec3_t& p3);
	void SetupPlane(vec3_t p, vec3_t n);
	void SetupPlane(vec3_t n, float d);
	void SetupPlane(float a, float b, float c, float d);

	bool GetIntersection(float *f, const vec3_t& p, const vec3_t& v) const;
	float GetDistFromPlane(const vec3_t& p) const;
	pointplane_t Classify(const vec3_t& p, float epsilon = lynxmath::EPSILON) const;
	bool IsPlaneBetween(const vec3_t&a, const vec3_t& b) const;

	// Plane Data
	vec3_t m_n;
	float m_d;
};

bool operator==(plane_t const &a, plane_t const &b);
bool operator!=(plane_t const &a, plane_t const &b);
