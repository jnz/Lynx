#pragma once

#include "math/plane.h"

enum frustum_plane_t 
{
    plane_near = 0, plane_left, plane_right, plane_up, plane_down, plane_far
};

struct CFrustum
{
    void Setup(const vec3_t& origin, 
               const vec3_t& dir, 
               const vec3_t& up, 
               const vec3_t& side,
               float fovY, float ratio,
               float near, float far);

    bool TestSphere(const vec3_t& point, const float radius) const; // Is the sphere inside the frustum
    bool TestPlane(const plane_t& plane) const; // Is the plane visible to the frustum?

    plane_t planes[6]; // see frustum_plane_t
    vec3_t ftl, ftr, fbl, fbr;
    vec3_t ntl, ntr, nbl, nbr;
    vec3_t pos;
};
