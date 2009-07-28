#pragma once

class CObj;
#include "math/vec3.h"
#include "math/matrix.h"
#include "Stream.h"
#include <string>
#include "World.h"
#include "ModelMD2.h"

class CPosition
{
public:
	CPosition();
	vec3_t	origin;			// Position
	vec3_t	velocity;		// Direction/Velocity
	vec3_t	rot;			// Rotation (x = pitch, y = yaw, z = roll)

	int		Serialize(bool write, CStream* stream);
	void	UpdateMatrix();
	matrix_t m;
};

class CObj
{
public:
	CObj(CWorld* world);
	virtual ~CObj(void);

	CPosition	pos;

	int			GetID() { return m_id; }

	int			Serialize(bool write, CStream* stream); // Objekt in einen Byte-Stream schreiben.
	
	float		GetSpeed();
	void		SetSpeed(float speed);
	float		GetFOV(); // Field of View. X-Axis. Altgrad
	void		SetFOV(float fov); // Field of View. X-Axis. Altgrad
	float		GetRadius(); // Max. Object sphere size
	void		SetAABB(const vec3_t& min, const vec3_t& max);
	void		GetAABB(vec3_t* min, vec3_t* max);
	std::string GetResource();
	void		SetResource(std::string resource);
	std::string GetAnimation();
	void		SetAnimation(std::string animation);
	vec3_t		GetEyePos();
	void		SetEyePos(const vec3_t& eyepos);

	void		ClearUpdateFlags();
	bool		HasChanged();

	// Direct Access for Renderer
	CModelMD2*	m_mesh;
	md2_state_t m_mesh_state;

private:
	DWORD		m_updateflags;

	float		m_speed;
	float		m_fov;
	float		m_radius;
	std::string m_resource;
	std::string m_animation;
	vec3_t		m_min;
	vec3_t		m_max;
	vec3_t		m_eyepos;

	// Don't touch these
	int			m_id;
	CWorld*		m_world;

	static int m_idpool;
};
