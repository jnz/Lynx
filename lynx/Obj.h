#pragma once

class CObj;
struct obj_state_t;
#include "math/vec3.h"
#include "math/matrix.h"
#include "Stream.h"
#include <string>
#include "World.h"
#include "ModelMD2.h"

// Serialize Helper Functions: Compare if newstate != oldstate, update updateflags with flagparam and write to stream (if not null)
int DeltaDiffVec3(const vec3_t* newstate,
                  const vec3_t* oldstate,
                  const DWORD flagparam, 
                  DWORD* updateflags,
                  CStream* stream);
int DeltaDiffFloat(const float* newstate,
                   const float* oldstate,
                   const DWORD flagparam, 
                   DWORD* updateflags,
                   CStream* stream);
int DeltaDiffString(const std::string* newstate,
                    const std::string* oldstate,
                    const DWORD flagparam, 
                    DWORD* updateflags,
                    CStream* stream);
int DeltaDiffDWORD(const DWORD* newstate,
                   const DWORD* oldstate,
                   const DWORD flagparam, 
                   DWORD* updateflags,
                   CStream* stream);

// If you change this, update the Serialize function
struct obj_state_t
{
	vec3_t	    origin;			// Position
	vec3_t	    vel;     		// Direction/Velocity
	vec3_t	    rot;			// Rotation (x = pitch, y = yaw, z = roll)

	float		fov;
	float		radius;
	std::string resource;
	std::string animation;
	vec3_t		min;
	vec3_t		max;
    vec3_t		eyepos;
};


class CObj
{
public:
	CObj(CWorld* world);
	virtual ~CObj(void);

	void	    UpdateMatrix();
	matrix_t    m;

	int			GetID() { return m_id; }

	bool		Serialize(bool write, CStream* stream, int id, const obj_state_t* oldstate=NULL); // Objekt in einen Byte-Stream schreiben. Wenn oldstate ungleich NULL, wird nur die Differenz geschrieben, gibt true zurück, wenn sich objekt durch geändert hat (beim lesen) oder wenn es sich von oldstate unterscheidet

    void        GetObjState(obj_state_t* objstate) { *objstate = state; }
	void		SetObjState(const obj_state_t* objstate, int id);

    const vec3_t GetOrigin() const { return state.origin; }
    void        SetOrigin(const vec3_t& origin) { state.origin = origin; }
    const vec3_t GetVel() const { return state.vel; }
    void        SetVel(const vec3_t& velocity) { state.vel = velocity; }
    const vec3_t GetRot() const { return state.rot; }
    void        SetRot(const vec3_t& rotation) { state.rot = rotation; }

	float		GetFOV(); // Field of View. X-Axis. Altgrad
	void		SetFOV(float fov); // Field of View. X-Axis. Altgrad
	float		GetRadius(); // Max. Object sphere size
    void        SetRadius(float radius);
	void		SetAABB(const vec3_t& min, const vec3_t& max);
	void		GetAABB(vec3_t* min, vec3_t* max);
	std::string GetResource();
	void		SetResource(std::string resource);
	std::string GetAnimation();
	void		SetAnimation(std::string animation);
	vec3_t		GetEyePos();
	void		SetEyePos(const vec3_t& eyepos);

    obj_state_t GetState() { return state; }

protected:
	// Direct Access for Renderer
	CModelMD2*	m_mesh;
    md2_state_t m_mesh_state;
	friend class CRenderer;

	void		UpdateProperties();
    obj_state_t state;
	CStream		m_stream;

private:
    // Don't touch these
	int			m_id;
	CWorld*		m_world;

	static int m_idpool;
};
