#pragma once

struct vec3_t;
struct quaternion_t;

/*
    column-major order. 16 consecutive floats. as in OpenGL.

    1 0 0 1
    0 1 0 2
    0 0 1 3
    0 0 0 1

    In memory:
    float m[] = { 1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  1, 2, 3, 1 };
    0  4  8 12 
    1  5  9 13
    2  6 10 14
    3  7 11 15

    Access:
    m[3][0] = 1
    m[3][1] = 2
    m[3][2] = 3

    m[x][y] | m[spalte][zeile]
*/

class matrix_t
{
public:
    union
    {
        float m[4][4];
        float pm[16];
    };

    matrix_t();
    matrix_t(float m11, float m21, float m31, float m41,
             float m12, float m22, float m32, float m42,
             float m13, float m23, float m33, float m43,
             float m14, float m24, float m34, float m44);

    bool Inverse(matrix_t* out); // *out can be the matrix itself. this method is btw. not that expensive.
    float Det() const;

    void GetRow(int row, float* f4) const;

    void SetZeroMatrix();
    void SetIdentity();
    void SetTranslation(float dx, float dy, float dz);

    void SetRotationX(float wx);
    void SetRotationY(float wy);
    void SetRotationZ(float wz);
    void SetRotationZXY(const vec3_t* angles); // (x = pitch, y = yaw, z = roll)
    void SetRotationYXZ(const vec3_t* angles); // (x = pitch, y = yaw, z = roll)
    void SetRotation(const vec3_t* axis, float angle);

    void SetTransform(const vec3_t* position, const vec3_t* angles);
    void SetCamTransform(const vec3_t* position, const vec3_t* angles); // camera ZXY angle order (Inverse of SetTransform)
    void SetCamTransform(const vec3_t& position, const quaternion_t& quat);
    void GetVec3Cam(vec3_t* dir, vec3_t* up, vec3_t* side); // only useful after SetCamTransform

    void MultiplyVec3(vec3_t *pDest, const vec3_t* pV) const; // pDest = M * pV. pV kann pDest sein
    void MultiplyVec3Fast(vec3_t *pDest, const vec3_t* pV) const; // w Komponente in Homogenen Koordinaten wird als 1 angenommen

#ifdef _DEBUG
    void Print();
#endif
};

void MatrixMultiply(matrix_t *pDest, const matrix_t *pMx1, const matrix_t *pMx2);

const matrix_t operator*(const matrix_t& m1, const matrix_t& m2);
