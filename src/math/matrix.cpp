#include <assert.h>
#include <math.h>
#include "mathconst.h"
#include "vec3.h"
#include "matrix.h"
#include "quaternion.h"
#include <memory.h> // for memset
#ifdef _DEBUG
#include <stdio.h> // for fprintf
#endif

const matrix_t m_identity(1,0,0,0,
                          0,1,0,0,
                          0,0,1,0,
                          0,0,0,1);


matrix_t::matrix_t()
{
}

matrix_t::matrix_t(float m11, float m21, float m31, float m41,
                   float m12, float m22, float m32, float m42,
                   float m13, float m23, float m33, float m43,
                   float m14, float m24, float m34, float m44)
{
    m[0][0] = m11;
    m[0][1] = m12;
    m[0][2] = m13;
    m[0][3] = m14;
    m[1][0] = m21;
    m[1][1] = m22;
    m[1][2] = m23;
    m[1][3] = m24;
    m[2][0] = m31;
    m[2][1] = m32;
    m[2][2] = m33;
    m[2][3] = m34;
    m[3][0] = m41;
    m[3][1] = m42;
    m[3][2] = m43;
    m[3][3] = m44;
}


void matrix_t::SetZeroMatrix()
{
    memset(&m[0][0], 0, sizeof(m));
}

void matrix_t::SetIdentity()
{
    //memcpy(&m[0][0], &m_identity.m[0][0], sizeof(m));
    *this = m_identity;
}

void matrix_t::SetTranslation(float dx, float dy, float dz)
{
    SetIdentity();
    m[3][0] = dx; m[3][1] = dy; m[3][2] = dz;
}
    
void matrix_t::SetRotationX(float wx)
{
    float sa, ca;
    wx *= lynxmath::DEGTORAD;
    sa = (float)sin(wx);
    ca = (float)cos(wx);
    SetIdentity();
    m[1][1] = ca;
    m[1][2] = sa;
    m[2][1] = -sa;
    m[2][2] = ca;
}

void matrix_t::SetRotationY(float wy)
{
    float sa, ca;
    wy *= lynxmath::DEGTORAD;
    sa = (float)sin(wy);
    ca = (float)cos(wy);
    SetIdentity();
    m[2][0] = sa;
    m[2][2] = ca;
    m[0][0] = ca;
    m[0][2] = -sa;
}

void matrix_t::SetRotationZ(float wz)
{
    float sa, ca;
    wz *= lynxmath::DEGTORAD;
    sa = (float)sin(wz);
    ca = (float)cos(wz);
    SetIdentity();
    m[0][0] = ca;
    m[0][1] = sa;
    m[1][0] = -sa;
    m[1][1] = ca;
}

void matrix_t::SetRotationZXY(const vec3_t* angles)
{
    float sx, sy, sz, cx, cy, cz;
    float angle;

    angle = angles->x * lynxmath::DEGTORAD;
    sx = sinf(angle);
    cx = cosf(angle);
    angle = angles->y * lynxmath::DEGTORAD;
    sy = sinf(angle);
    cy = cosf(angle);
    angle = angles->z * lynxmath::DEGTORAD;
    sz = sinf(angle);
    cz = cosf(angle);

    m[0][0] = cy*cz-sx*sy*sz;
    m[1][0] = -cx*sz;
    m[2][0] = sy*cz+sx*sz*cy;
    m[3][0] = 0.0f;

    m[0][1] = sz*cy+sx*sy*cz;
    m[1][1] = cx*cz;
    m[2][1] = sy*sz-sx*cy*cz;
    m[3][1] = 0.0f;

    m[0][2] = -cx*sy;
    m[1][2] = sx;
    m[2][2] = cx*cy;
    m[3][2] = 0.0f;

    m[0][3] = m[1][3] = m[2][3] = 0.0f;
    m[3][3] = 1.0f;
}

void matrix_t::SetRotationYXZ(const vec3_t* angles)
{
    float sx, sy, sz, cx, cy, cz;
    float angle;

    angle = angles->x * lynxmath::DEGTORAD;
    sx = sinf(angle);
    cx = cosf(angle);
    angle = angles->y * lynxmath::DEGTORAD;
    sy = sinf(angle);
    cy = cosf(angle);
    angle = angles->z * lynxmath::DEGTORAD;
    sz = sinf(angle);
    cz = cosf(angle);

    m[0][0] = cy*cz+sx*sy*sz;
    m[1][0] = sx*sy*cz-sz*cy;
    m[2][0] = cx*sy;
    m[3][0] = 0.0f;

    m[0][1] = cx*sz;
    m[1][1] = cx*cz;
    m[2][1] = -sx;
    m[3][1] = 0.0f;

    m[0][2] = sx*sz*cy-sy*cz;
    m[1][2] = sy*sz+sx*cy*cz;
    m[2][2] = cx*cy;
    m[3][2] = 0.0f;

    m[0][3] = m[1][3] = m[2][3] = 0.0f;
    m[3][3] = 1.0f;
}

void matrix_t::SetRotation(const vec3_t* axis, float angle)
{
    float sin, cos;
    float xc, yc, zc;
    float x, y, z;

    x = axis->x;
    y = axis->y;
    z = axis->z;

    angle *= lynxmath::DEGTORAD;
    sin = sinf(angle);
    cos = cosf(angle);

    xc = x*(1-cos);
    yc = y*(1-cos);
    zc = z*(1-cos);

    m[0][0] = x*xc+cos;
    m[0][1] = x*yc-z*sin;
    m[0][2] = x*zc+y*sin;
    m[0][3] = 0.0f;

    m[1][0] = y*xc+z*sin;
    m[1][1] = y*yc+cos;
    m[1][2] = y*zc-x*sin;
    m[1][3] = 0.0f;

    m[2][0] = z*xc-y*sin;
    m[2][1] = z*yc+x*sin;
    m[2][2] = z*zc+cos;
    m[2][3] = 0.0f;

    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = 0.0f;
    m[3][3] = 1.0f;
}

#pragma warning(disable : 4244)

bool matrix_t::Inverse(matrix_t* out)
{
    // 84+4+16 = 104 multiplications
    //             1 division
    double det, invDet;

    // 2x2 sub-determinants required to calculate 4x4 determinant
    float det2_01_01 = m[0][0] * m[1][1] - m[0][1] * m[1][0];
    float det2_01_02 = m[0][0] * m[1][2] - m[0][2] * m[1][0];
    float det2_01_03 = m[0][0] * m[1][3] - m[0][3] * m[1][0];
    float det2_01_12 = m[0][1] * m[1][2] - m[0][2] * m[1][1];
    float det2_01_13 = m[0][1] * m[1][3] - m[0][3] * m[1][1];
    float det2_01_23 = m[0][2] * m[1][3] - m[0][3] * m[1][2];

    // 3x3 sub-determinants required to calculate 4x4 determinant
    float det3_201_012 = m[2][0] * det2_01_12 - m[2][1] * det2_01_02 + m[2][2] * det2_01_01;
    float det3_201_013 = m[2][0] * det2_01_13 - m[2][1] * det2_01_03 + m[2][3] * det2_01_01;
    float det3_201_023 = m[2][0] * det2_01_23 - m[2][2] * det2_01_03 + m[2][3] * det2_01_02;
    float det3_201_123 = m[2][1] * det2_01_23 - m[2][2] * det2_01_13 + m[2][3] * det2_01_12;

    det = ( - det3_201_123 * m[3][0] + det3_201_023 * m[3][1] - det3_201_013 * m[3][2] + det3_201_012 * m[3][3] );

    if(fabsf(det) < lynxmath::EPSILON)
        return false;

    invDet = 1.0f / det;

    // remaining 2x2 sub-determinants
    float det2_03_01 = m[0][0] * m[3][1] - m[0][1] * m[3][0];
    float det2_03_02 = m[0][0] * m[3][2] - m[0][2] * m[3][0];
    float det2_03_03 = m[0][0] * m[3][3] - m[0][3] * m[3][0];
    float det2_03_12 = m[0][1] * m[3][2] - m[0][2] * m[3][1];
    float det2_03_13 = m[0][1] * m[3][3] - m[0][3] * m[3][1];
    float det2_03_23 = m[0][2] * m[3][3] - m[0][3] * m[3][2];

    float det2_13_01 = m[1][0] * m[3][1] - m[1][1] * m[3][0];
    float det2_13_02 = m[1][0] * m[3][2] - m[1][2] * m[3][0];
    float det2_13_03 = m[1][0] * m[3][3] - m[1][3] * m[3][0];
    float det2_13_12 = m[1][1] * m[3][2] - m[1][2] * m[3][1];
    float det2_13_13 = m[1][1] * m[3][3] - m[1][3] * m[3][1];
    float det2_13_23 = m[1][2] * m[3][3] - m[1][3] * m[3][2];

    // remaining 3x3 sub-determinants
    float det3_203_012 = m[2][0] * det2_03_12 - m[2][1] * det2_03_02 + m[2][2] * det2_03_01;
    float det3_203_013 = m[2][0] * det2_03_13 - m[2][1] * det2_03_03 + m[2][3] * det2_03_01;
    float det3_203_023 = m[2][0] * det2_03_23 - m[2][2] * det2_03_03 + m[2][3] * det2_03_02;
    float det3_203_123 = m[2][1] * det2_03_23 - m[2][2] * det2_03_13 + m[2][3] * det2_03_12;

    float det3_213_012 = m[2][0] * det2_13_12 - m[2][1] * det2_13_02 + m[2][2] * det2_13_01;
    float det3_213_013 = m[2][0] * det2_13_13 - m[2][1] * det2_13_03 + m[2][3] * det2_13_01;
    float det3_213_023 = m[2][0] * det2_13_23 - m[2][2] * det2_13_03 + m[2][3] * det2_13_02;
    float det3_213_123 = m[2][1] * det2_13_23 - m[2][2] * det2_13_13 + m[2][3] * det2_13_12;

    float det3_301_012 = m[3][0] * det2_01_12 - m[3][1] * det2_01_02 + m[3][2] * det2_01_01;
    float det3_301_013 = m[3][0] * det2_01_13 - m[3][1] * det2_01_03 + m[3][3] * det2_01_01;
    float det3_301_023 = m[3][0] * det2_01_23 - m[3][2] * det2_01_03 + m[3][3] * det2_01_02;
    float det3_301_123 = m[3][1] * det2_01_23 - m[3][2] * det2_01_13 + m[3][3] * det2_01_12;

    out->m[0][0] = - det3_213_123 * invDet;
    out->m[1][0] = + det3_213_023 * invDet;
    out->m[2][0] = - det3_213_013 * invDet;
    out->m[3][0] = + det3_213_012 * invDet;

    out->m[0][1] = + det3_203_123 * invDet;
    out->m[1][1] = - det3_203_023 * invDet;
    out->m[2][1] = + det3_203_013 * invDet;
    out->m[3][1] = - det3_203_012 * invDet;

    out->m[0][2] = + det3_301_123 * invDet;
    out->m[1][2] = - det3_301_023 * invDet;
    out->m[2][2] = + det3_301_013 * invDet;
    out->m[3][2] = - det3_301_012 * invDet;

    out->m[0][3] = - det3_201_123 * invDet;
    out->m[1][3] = + det3_201_023 * invDet;
    out->m[2][3] = - det3_201_013 * invDet;
    out->m[3][3] = + det3_201_012 * invDet;

    return true;
}

float matrix_t::Det() const
{
    // doom3's determinant
    float det2_01_12 = m[0][1] * m[1][2] - m[0][2] * m[1][1];
    float det2_01_13 = m[0][1] * m[1][3] - m[0][3] * m[1][1];
    float det2_01_23 = m[0][2] * m[1][3] - m[0][3] * m[1][2];
    float det2_01_01 = m[0][0] * m[1][1] - m[0][1] * m[1][0];
    float det2_01_02 = m[0][0] * m[1][2] - m[0][2] * m[1][0];
    float det2_01_03 = m[0][0] * m[1][3] - m[0][3] * m[1][0];

    float det3_201_012 = m[2][0] * det2_01_12 - m[2][1] * det2_01_02 + m[2][2] * det2_01_01;
    float det3_201_013 = m[2][0] * det2_01_13 - m[2][1] * det2_01_03 + m[2][3] * det2_01_01;
    float det3_201_023 = m[2][0] * det2_01_23 - m[2][2] * det2_01_03 + m[2][3] * det2_01_02;
    float det3_201_123 = m[2][1] * det2_01_23 - m[2][2] * det2_01_13 + m[2][3] * det2_01_12;

    return ( - det3_201_123 * m[3][0] + det3_201_023 * m[3][1] - det3_201_013 * m[3][2] + det3_201_012 * m[3][3] );
}

void matrix_t::MultiplyVec3(vec3_t *pDest, const vec3_t *pV) const
{
    vec3_t tmp = *pV;
    float w =   m[0][3]*pV->x + 
                m[1][3]*pV->y +  
                m[2][3]*pV->z +
                m[3][3];
    assert(fabsf(w)>lynxmath::EPSILON);
    w = 1/w;

    pDest->x =  (m[0][0]*tmp.x + 
                 m[1][0]*tmp.y +  
                 m[2][0]*tmp.z +
                 m[3][0])
                 *w;
    pDest->y =  (m[0][1]*tmp.x + 
                 m[1][1]*tmp.y +  
                 m[2][1]*tmp.z +
                 m[3][1])
                 *w;
    pDest->z =  (m[0][2]*tmp.x + 
                 m[1][2]*tmp.y +  
                 m[2][2]*tmp.z +
                 m[3][2])
                 *w;

}

void matrix_t::MultiplyVec3Fast(vec3_t *pDest, const vec3_t *pV) const
{
    vec3_t tmp = *pV;
#ifdef _DEBUG
    float w =   m[0][3]*pV->x + 
                m[1][3]*pV->y +  
                m[2][3]*pV->z +
                m[3][3];
    assert(fabsf(1.0f - w) < lynxmath::EPSILON); // if this happens, try MatrixVec3Mult
#endif

    pDest->x =  (m[0][0]*tmp.x + 
                 m[1][0]*tmp.y +  
                 m[2][0]*tmp.z +
                 m[3][0]);
    pDest->y =  (m[0][1]*tmp.x + 
                 m[1][1]*tmp.y +  
                 m[2][1]*tmp.z +
                 m[3][1]);
    pDest->z =  (m[0][2]*tmp.x + 
                 m[1][2]*tmp.y +  
                 m[2][2]*tmp.z +
                 m[3][2]);
}

void MatrixMultiply(matrix_t *pDest, const matrix_t *pMx1, const matrix_t *pMx2)
{
    pDest->m[0][0] = pMx2->m[0][0]*pMx1->m[0][0]
                   + pMx2->m[0][1]*pMx1->m[1][0]
                   + pMx2->m[0][2]*pMx1->m[2][0]
                   + pMx2->m[0][3]*pMx1->m[3][0];
    pDest->m[1][0] = pMx2->m[1][0]*pMx1->m[0][0]
                   + pMx2->m[1][1]*pMx1->m[1][0]
                   + pMx2->m[1][2]*pMx1->m[2][0]
                   + pMx2->m[1][3]*pMx1->m[3][0];
    pDest->m[2][0] = pMx2->m[2][0]*pMx1->m[0][0]
                   + pMx2->m[2][1]*pMx1->m[1][0]
                   + pMx2->m[2][2]*pMx1->m[2][0]
                   + pMx2->m[2][3]*pMx1->m[3][0];
    pDest->m[3][0] = pMx2->m[3][0]*pMx1->m[0][0]
                   + pMx2->m[3][1]*pMx1->m[1][0]
                   + pMx2->m[3][2]*pMx1->m[2][0]
                   + pMx2->m[3][3]*pMx1->m[3][0];

    pDest->m[0][1] = pMx2->m[0][0]*pMx1->m[0][1]
                   + pMx2->m[0][1]*pMx1->m[1][1]
                   + pMx2->m[0][2]*pMx1->m[2][1]
                   + pMx2->m[0][3]*pMx1->m[3][1];
    pDest->m[1][1] = pMx2->m[1][0]*pMx1->m[0][1]
                   + pMx2->m[1][1]*pMx1->m[1][1]
                   + pMx2->m[1][2]*pMx1->m[2][1]
                   + pMx2->m[1][3]*pMx1->m[3][1];
    pDest->m[2][1] = pMx2->m[2][0]*pMx1->m[0][1]
                   + pMx2->m[2][1]*pMx1->m[1][1]
                   + pMx2->m[2][2]*pMx1->m[2][1]
                   + pMx2->m[2][3]*pMx1->m[3][1];
    pDest->m[3][1] = pMx2->m[3][0]*pMx1->m[0][1]
                   + pMx2->m[3][1]*pMx1->m[1][1]
                   + pMx2->m[3][2]*pMx1->m[2][1]
                   + pMx2->m[3][3]*pMx1->m[3][1];

    pDest->m[0][2] = pMx2->m[0][0]*pMx1->m[0][2]
                   + pMx2->m[0][1]*pMx1->m[1][2]
                   + pMx2->m[0][2]*pMx1->m[2][2]
                   + pMx2->m[0][3]*pMx1->m[3][2];
    pDest->m[1][2] = pMx2->m[1][0]*pMx1->m[0][2]
                   + pMx2->m[1][1]*pMx1->m[1][2]
                   + pMx2->m[1][2]*pMx1->m[2][2]
                   + pMx2->m[1][3]*pMx1->m[3][2];
    pDest->m[2][2] = pMx2->m[2][0]*pMx1->m[0][2]
                   + pMx2->m[2][1]*pMx1->m[1][2]
                   + pMx2->m[2][2]*pMx1->m[2][2]
                   + pMx2->m[2][3]*pMx1->m[3][2];
    pDest->m[3][2] = pMx2->m[3][0]*pMx1->m[0][2]
                   + pMx2->m[3][1]*pMx1->m[1][2]
                   + pMx2->m[3][2]*pMx1->m[2][2]
                   + pMx2->m[3][3]*pMx1->m[3][2];

    pDest->m[0][3] = pMx2->m[0][0]*pMx1->m[0][3]
                   + pMx2->m[0][1]*pMx1->m[1][3]
                   + pMx2->m[0][2]*pMx1->m[2][3]
                   + pMx2->m[0][3]*pMx1->m[3][3];
    pDest->m[1][3] = pMx2->m[1][0]*pMx1->m[0][3]
                   + pMx2->m[1][1]*pMx1->m[1][3]
                   + pMx2->m[1][2]*pMx1->m[2][3]
                   + pMx2->m[1][3]*pMx1->m[3][3];
    pDest->m[2][3] = pMx2->m[2][0]*pMx1->m[0][3]
                   + pMx2->m[2][1]*pMx1->m[1][3]
                   + pMx2->m[2][2]*pMx1->m[2][3]
                   + pMx2->m[2][3]*pMx1->m[3][3];
    pDest->m[3][3] = pMx2->m[3][0]*pMx1->m[0][3]
                   + pMx2->m[3][1]*pMx1->m[1][3]
                   + pMx2->m[3][2]*pMx1->m[2][3]
                   + pMx2->m[3][3]*pMx1->m[3][3];
}

const matrix_t operator*(const matrix_t& m1, const matrix_t& m2)
{
    matrix_t m;
    MatrixMultiply(&m, &m1, &m2);
    return m;
}

void matrix_t::SetTransform(const vec3_t* position, const vec3_t* angles)
{
    float sx, sy, sz, cx, cy, cz;
    float angle;

    angle = angles->x * lynxmath::DEGTORAD;
    sx = sinf(angle);
    cx = cosf(angle);
    angle = angles->y * lynxmath::DEGTORAD;
    sy = sinf(angle);
    cy = cosf(angle);
    angle = angles->z * lynxmath::DEGTORAD;
    sz = sinf(angle);
    cz = cosf(angle);

    m[0][0] = cy*cz+sx*sy*sz;
    m[0][1] = cx*sz;
    m[0][2] = sx*sz*cy-sy*cz;

    m[1][0] = sx*sy*cz-sz*cy;
    m[1][1] = cx*cz;
    m[1][2] = sy*sz+sx*cy*cz;

    m[2][0] = cx*sy;
    m[2][1] = -sx;
    m[2][2] = cx*cy;

    if(position)
    {
        m[3][0] = position->x;
        m[3][1] = position->y;
        m[3][2] = position->z;
    }
    else
    {
        m[3][0] = m[3][1] = m[3][2] = 0.0f;
    }

    m[0][3] = m[1][3] = m[2][3] = 0.0f;
    m[3][3] = 1.0f;
}

void matrix_t::SetCamTransform(const vec3_t* pos, const vec3_t* angles)
{
    float sx, sy, sz, cx, cy, cz;
    float angle;

    angle = angles->x * lynxmath::DEGTORAD;
    sx = sinf(angle);
    cx = cosf(angle);
    angle = angles->y * lynxmath::DEGTORAD;
    sy = sinf(angle);
    cy = cosf(angle);
    angle = angles->z * lynxmath::DEGTORAD;
    sz = sinf(angle);
    cz = cosf(angle);

    m[0][0] = cy*cz+sx*sy*sz;
    m[1][0] = cx*sz;
    m[2][0] = sx*sz*cy-sy*cz;

    m[0][1] = sx*sy*cz-sz*cy;
    m[1][1] = cx*cz;
    m[2][1] = sy*sz+sx*cy*cz;

    m[0][2] = cx*sy;
    m[1][2] = -sx;
    m[2][2] = cx*cy;

    m[3][0] = - (pos->x * m[0][0] + pos->y * m[1][0] + pos->z * m[2][0]);
    m[3][1] = - (pos->x * m[0][1] + pos->y * m[1][1] + pos->z * m[2][1]);
    m[3][2] = - (pos->x * m[0][2] + pos->y * m[1][2] + pos->z * m[2][2]);

    m[0][3] = m[1][3] = m[2][3] = 0.0f;
    m[3][3] = 1.0f;
}

void matrix_t::SetCamTransform(const vec3_t& pos, const quaternion_t& q)
{
    m[0][0] = 1.0f - 2.0f*(q.y*q.y + q.z*q.z);
    m[1][0] = 2.0f * (q.x*q.y + q.w*q.z);
    m[2][0] = 2.0f * (q.x*q.z - q.w*q.y);

    m[0][1] = 2.0f * (q.x*q.y - q.w*q.z);
    m[1][1] = 1.0f - 2.0f * (q.x*q.x + q.z*q.z);
    m[2][1] = 2.0f * (q.y*q.z + q.w*q.x);

    m[0][2] = 2.0f * (q.x*q.z + q.w*q.y);
    m[1][2] = 2.0f * (q.y*q.z - q.w*q.x);
    m[2][2] = 1.0f - 2.0f * (q.x*q.x + q.y*q.y);

    m[3][0] = - (pos.x * m[0][0] + pos.y * m[1][0] + pos.z * m[2][0]);
    m[3][1] = - (pos.x * m[0][1] + pos.y * m[1][1] + pos.z * m[2][1]);
    m[3][2] = - (pos.x * m[0][2] + pos.y * m[1][2] + pos.z * m[2][2]);

    m[0][3] = m[1][3] = m[2][3] = 0.0f;
    m[3][3] = 1.0f;
}

void matrix_t::GetVec3Cam(vec3_t* dir, vec3_t* up, vec3_t* side)
{
    if(side)
    {
        side->x = m[0][0];
        side->y = m[1][0];
        side->z = m[2][0];
    }

    if(up)
    {
        up->x = m[0][1];
        up->y = m[1][1];
        up->z = m[2][1];
    }

    if(dir)
    {
        dir->x = m[0][2];
        dir->y = m[1][2];
        dir->z = m[2][2];
    }
}

void matrix_t::GetRow(int row, float* f4) const
{
    assert(row >= 0 && row <= 3);

    f4[0] = pm[ 0 + row];
    f4[1] = pm[ 4 + row];
    f4[2] = pm[ 8 + row];
    f4[3] = pm[12 + row];
    //f4[0] = pm[ 0 + 4*row];
    //f4[1] = pm[ 1 + 4*row];
    //f4[2] = pm[ 2 + 4*row];
    //f4[3] = pm[ 3 + 4*row];
}

// Graveyard

/*


static const unsigned char g_sarrus_index[4][3] = 
{ { 1,2,3 },
  { 0,2,3 },
  { 0,1,3 },
  { 0,1,2 } };

static inline float SubDet(int i, int j, float m[4][4])
{
    const int i1 = g_sarrus_index[i][0];
    const int i2 = g_sarrus_index[i][1];
    const int i3 = g_sarrus_index[i][2];
    const int j1 = g_sarrus_index[j][0];
    const int j2 = g_sarrus_index[j][1];
    const int j3 = g_sarrus_index[j][2];

    // Regel von Sarrus
    return  m[i1][j1] * m[i2][j2] * m[i3][j3] +
            m[i1][j2] * m[i2][j3] * m[i3][j1] +
            m[i1][j3] * m[i2][j1] * m[i3][j2] -
            m[i3][j1] * m[i2][j2] * m[i1][j3] -
            m[i3][j2] * m[i2][j3] * m[i1][j1] -
            m[i3][j3] * m[i2][j1] * m[i1][j2];
}

static inline float GetSign(int i, int j)
{
//  if((i+j)%2)
//      return -1.0f;
//  else
//      return 1.0f;

    union getsign_int_float_u
    {
        unsigned int i;
        float f;
    } u;
    assert(sizeof(float)==sizeof(unsigned int));
    u.f = 1.0f;
    u.i |= ((i+j) << (sizeof(u)*8-1));
    return u.f;
}

bool matrix_t::Inverse(matrix_t* out)
{
    int i, j;
    float det = Det();
    if(fabsf(det)<lynxmath::EPSILON)
        return false;
    det = 1/det;
    
    for(i=0;i<4;i++)
        for(j=0;j<4;j++)
            out->m[i][j] = det * SubDet(j, i, m) * GetSign(j, i);
    
    return true;
}



float matrix_t::Det()
{
    int i1, i2, i3;
    float d;
    d=0;
    // berechnen der Determinante nach entwicklungssatz
    i1=1;i2=2;i3=3;
    d+=m[0][0]*(
        +m[i1][1] * (m[i2][2]*m[i3][3] - m[i2][3]*m[i3][2])
        -m[i2][1] * (m[i1][2]*m[i3][3] - m[i1][3]*m[i3][2])
        +m[i3][1] * (m[i1][2]*m[i2][3] - m[i1][3]*m[i2][2]));
    i1=0;i2=2;i3=3;
    d-=m[1][0]*(
        +m[i1][1] * (m[i2][2]*m[i3][3] - m[i2][3]*m[i3][2])
        -m[i2][1] * (m[i1][2]*m[i3][3] - m[i1][3]*m[i3][2])
        +m[i3][1] * (m[i1][2]*m[i2][3] - m[i1][3]*m[i2][2]));
    i1=0;i2=1;i3=3;
    d+=m[2][0]*(
        +m[i1][1] * (m[i2][2]*m[i3][3] - m[i2][3]*m[i3][2])
        -m[i2][1] * (m[i1][2]*m[i3][3] - m[i1][3]*m[i3][2])
        +m[i3][1] * (m[i1][2]*m[i2][3] - m[i1][3]*m[i2][2]));
    i1=0;i2=1;i3=2;
    d-=m[3][0]*(
        +m[i1][1] * (m[i2][2]*m[i3][3] - m[i2][3]*m[i3][2])
        -m[i2][1] * (m[i1][2]*m[i3][3] - m[i1][3]*m[i3][2])
        +m[i3][1] * (m[i1][2]*m[i2][3] - m[i1][3]*m[i2][2]));
    return d;
}
*/

#ifdef _DEBUG
void matrix_t::Print()
{
    fprintf(stderr, "%.2f %.2f %.2f %.2f\n", m[0][0], m[1][0], m[2][0], m[3][0]);
    fprintf(stderr, "%.2f %.2f %.2f %.2f\n", m[0][1], m[1][1], m[2][1], m[3][1]);
    fprintf(stderr, "%.2f %.2f %.2f %.2f\n", m[0][2], m[1][2], m[2][2], m[3][2]);
    fprintf(stderr, "%.2f %.2f %.2f %.2f\n", m[0][3], m[1][3], m[2][3], m[3][3]);
    fprintf(stderr, "Det: %.2f\n", Det());

    float* p = &m[0][0];
    fprintf(stderr, "In mem: ");
    for(int i=0;i<16;i++) fprintf(stderr, "%.2f ", p[i]);
    fprintf(stderr, "\n\n");
}
#endif
