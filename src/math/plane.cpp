#include "plane.h"
#include <math.h>

bool operator==(plane_t const &a, plane_t const &b)
{
    return (a.m_n == b.m_n) && (fabsf(a.m_d - b.m_d) < lynxmath::EPSILON);
}

bool operator!=(plane_t const &a, plane_t const &b)
{
    return (fabsf(a.m_d - b.m_d) > lynxmath::EPSILON) || !(a.m_n == b.m_n);
}

plane_t::plane_t(vec3_t p1, vec3_t p2, vec3_t p3)
{
    SetupPlane(p1, p2, p3);
}

plane_t::plane_t(vec3_t p, vec3_t n)
{
    SetupPlane(p, n);
}

plane_t::plane_t(float a, float b, float c, float d)
{
    SetupPlane(a, b, c, d);
}

void plane_t::SetupPlane(const vec3_t& p1,
                         const vec3_t& p2,
                         const vec3_t& p3)
{
    m_n = (p2-p1)^(p3-p1);
    m_n.Normalize();
    m_d = -m_n * p1;
}

void plane_t::SetupPlane(vec3_t p, vec3_t n)
{
    m_n = n;
    m_n.Normalize();
    m_d = -m_n * p;
}

void plane_t::SetupPlane(vec3_t n, float d)
{
    m_n = n;
    m_d = d;
}

void plane_t::SetupPlane(float a, float b, float c, float d)
{
    m_n = vec3_t(a, b, c);
    m_d = d / m_n.Abs();
    m_n.Normalize();
}

bool plane_t::GetIntersection(float *f, const vec3_t& p, const vec3_t& v) const
{
    const float q = m_n * v;
    if(fabsf(q) < lynxmath::EPSILON)
        return false;

    *f = -(m_n * p + m_d) / q;
    return true;
}

float plane_t::GetDistFromPlane(const vec3_t& p) const
{
    return p*m_n + m_d;
}

pointplane_t plane_t::Classify(const vec3_t& p, const float epsilon) const
{
    const float f = p*m_n + m_d; // GetDistFromPlane
    if(f > epsilon)
        return POINTPLANE_FRONT;
    else if(f < -epsilon)
        return POINTPLANE_BACK;
    else
        return POINT_ON_PLANE;
}

bool plane_t::IsPlaneBetween(const vec3_t& a, const vec3_t& b) const
{
    float q;
    float t;

    q = m_n * (b-a);
    if(fabs(q) < lynxmath::EPSILON)
        return true;

    t = -(m_n * a + m_d) / q;
    return (t > -lynxmath::EPSILON && t < 1.0f + lynxmath::EPSILON);
}
