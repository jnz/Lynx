#include "lynx.h"
#include <math.h>
#include "math/mathconst.h"
#include "Frustum.h"
#include "math/matrix.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

void CFrustum::Setup(const vec3_t& origin, 
                     const vec3_t& dir, 
                     const vec3_t& up, 
                     const vec3_t& side,
                     float fovY, float ratio,
                     float near, float far)
{
    vec3_t farup, farside;
    vec3_t nearup, nearside;
    vec3_t farplane, nearplane;
    float angle;
    float fw, fh, nw, nh;

    angle = tanf(fovY * lynxmath::DEGTORAD * 0.5f);
    nh = angle * near;
    nw = nh * ratio;
    fh = angle * far;
    fw = fh * ratio;
    
    farup = up * fh;
    farside = side * fw;
    nearup = up * nh;
    nearside = side * nw;

    farplane = dir * far;
    nearplane = dir * near;

    ftl = origin + farplane + farup - farside;
    ftr = origin + farplane + farup + farside;
    fbl = origin + farplane - farup - farside;
    fbr = origin + farplane - farup + farside;
    ntl = origin + nearplane + nearup - nearside;
    ntr = origin + nearplane + nearup + nearside;
    nbl = origin + nearplane - nearup - nearside;
    nbr = origin + nearplane - nearup + nearside;

    planes[plane_left].SetupPlane(fbl, ftl, ntl);
    planes[plane_right].SetupPlane(ftr, fbr, nbr);
    planes[plane_up].SetupPlane(ntl, ftl, ftr);
    planes[plane_down].SetupPlane(fbr, fbl, nbl);
    planes[plane_near].SetupPlane(origin + nearplane, dir);
    planes[plane_far].SetupPlane(fbr, ftr, ftl);

    pos = origin;
}

bool CFrustum::TestSphere(const vec3_t& point, float radius) const
{
    float dist;
    radius = -radius;

    for(int i=0;i<6;i++)
    {
        dist = planes[i].GetDistFromPlane(point);
        if(dist < radius)
            return false;
    }

    return true;
}

bool CFrustum::TestPlane(const plane_t &plane) const
{
    if(plane.IsPlaneBetween(pos, ftl))
        return true;
    if(plane.IsPlaneBetween(pos, ftr))
        return true;
    if(plane.IsPlaneBetween(pos, fbl))
        return true;
    if(plane.IsPlaneBetween(pos, fbr))
        return true;
    return false;
}
