#pragma once

struct vec3_t
{
	union
	{
		struct
		{
			float x, y, z;
	 	};
		float v[3];
	};

	vec3_t();
	vec3_t(float nx, float ny, float nz);
	vec3_t(float all);

	vec3_t rand(float mx, float my, float mz);

	float Abs(void) const;
	float AbsSquared(void) const;

	float Normalize(void);
	vec3_t Normalized(void) const;
	bool IsNormalized() const;
	void SetLength(float length);

	bool IsNull() const; // Exakt auf 0,0,0 testen (ohne Epsilon)

	bool IsInArea(const vec3_t& min, const vec3_t& max) const;

    static vec3_t Lerp(const vec3_t& p1, const vec3_t& p2, const float f); // Lineare Interpolation zw. p1 und p2. Bei f=0 ist das Ergebnis p1, bei f=1 wird p2 zurückgegeben

	/*
        AngleVec3
		angles x = pitch, y = yaw, z = roll
		bei 0/0/0 zeigt forward nach (0,0,-1)
		AngleVec3 entspricht den Drehungen einer YXZ Rotationsmatrix
	*/
	static void AngleVec3(const vec3_t& angles, vec3_t* forward, vec3_t* up, vec3_t* side);
	static float GetAngleDeg(vec3_t& a, vec3_t& b); // angle between a and b in degrees

	/*
		Helferfunktion: Schnittpunkt Ray mit Zylinder. 
		Rückgabe: true, in f steht der Skalierungsfaktor zum Schnittpunkt pStart + f*(pEnd-pStart)
	*/
	static bool RayCylinderIntersect(const vec3_t& pStart, const vec3_t& pDir, 
									 const vec3_t& edgeStart, const vec3_t& edgeEnd,
									 const float radius,
									 float* f);
    /*
        Helferfunktion: Schnittpunkt Ray mit Sphere
     */
    static bool RaySphereIntersect(const vec3_t& pStart, const vec3_t& pDir,
                                   const vec3_t& pSphere, const float radius,
                                   float* f);

	static const vec3_t origin;
    static const vec3_t xAxis; // 1,0,0
    static const vec3_t yAxis; // 0,1,0
    static const vec3_t zAxis; // 0,0,1

#ifdef _DEBUG
	void Print();
#endif

	vec3_t &operator += ( const vec3_t &v );
	vec3_t &operator -= ( const vec3_t &v );
	vec3_t &operator *= ( const float &f );
	vec3_t &operator /= ( const float &f );
	vec3_t  operator -  (void) const;
};

float vabs(vec3_t v);

const vec3_t operator+(vec3_t const &a, vec3_t const &b);
const vec3_t operator-(vec3_t const &a, vec3_t const &b);

const vec3_t operator*(vec3_t const &v, float const &f);
const vec3_t operator*(float const &f, vec3_t const &v);

const vec3_t operator/(vec3_t const &v, float const &f);

// cross product
const vec3_t operator^(vec3_t const &a, vec3_t const &b);

// dot product
const float operator*(vec3_t const &a, vec3_t const &b);
bool   operator==(vec3_t const &a, vec3_t const &b);
bool   operator!=(vec3_t const &a, vec3_t const &b);
