#include <string.h>
#include "lynx.h"
#include "Obj.h"
#include <math.h>

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#define DEFAULT_FOV		90.0f
//#define SAVE_BANDWIDTH		// Benötigte Bandbreite wird um ca. 50% reduziert

#define OBJ_UPDATE_FOV			(1<<0)
#define	OBJ_UPDATE_AABB			(1<<1)
#define OBJ_UPDATE_RESOURCE		(1<<2)
#define OBJ_UPDATE_ANIMATION	(1<<3)
#define OBJ_UPDATE_SPEED		(1<<4)
#define OBJ_UPDATE_EYEPOS		(1<<5)

CPosition::CPosition()
{

}

int CPosition::Serialize(bool write, CStream* stream)
{
#ifdef SAVE_BANDWIDTH
	const int reqsize = STREAM_SIZE_POS6 +				// velocity
						STREAM_SIZE_POS6 +				// origin
						STREAM_SIZE_ANGLE3;				// angle
#else
	const int reqsize = STREAM_SIZE_VEC3 +				// velocity
						STREAM_SIZE_VEC3 +				// origin
						STREAM_SIZE_VEC3;				// angle
#endif

	if(!stream || 
		(write && stream->GetSpaceLeft() < reqsize) ||
		(!write && stream->GetBytesToRead() < reqsize))
		return reqsize;

	if(write)
	{
#ifdef SAVE_BANDWIDTH
		stream->WritePos6(&origin);
		stream->WritePos6(&velocity);
		stream->WriteAngle3(&rot);
#else
		stream->WriteVec3(&origin);
		stream->WriteVec3(&velocity);
		stream->WriteVec3(&rot);
#endif
	}
	else
	{
#ifdef SAVE_BANDWIDTH
		stream->ReadPos6(&origin);
		stream->ReadPos6(&velocity);
		stream->ReadAngle3(&rot);
#else
		stream->ReadVec3(&origin);
		stream->ReadVec3(&velocity);
		stream->ReadVec3(&rot);
#endif
		UpdateMatrix();
	}

	return reqsize;
}

void CPosition::UpdateMatrix()
{
	m.SetTransform(NULL, &rot);
}

int CObj::m_idpool = 0;

CObj::CObj(CWorld* world)
{
	assert(world);
	m_id = ++m_idpool;
	m_speed = 0.0f;
	m_radius = lynxmath::SQRT_2;
	m_min = vec3_t(-1,-1,-1);
	m_max = vec3_t(1,1,1);
	m_fov = DEFAULT_FOV;
	m_mesh = NULL;
	m_world = world;
	m_updateflags = 0;
	m_eyepos = vec3_t(0,1,0);
}

CObj::~CObj(void)
{
}

void CObj::ClearUpdateFlags()
{
	m_updateflags = 0;
}

bool CObj::HasChanged()
{
	return (m_updateflags > 0);
}

int CObj::Serialize(bool write, CStream* stream)
{
	int reqsize;

	if(write)
	{
		reqsize = 0;
		reqsize += sizeof(WORD);
		reqsize += sizeof(DWORD);
		if(m_updateflags & OBJ_UPDATE_FOV)
			reqsize += sizeof(float);
		if(m_updateflags & OBJ_UPDATE_RESOURCE)
			reqsize += sizeof(WORD) + (int)m_resource.size();
		if(m_updateflags & OBJ_UPDATE_ANIMATION)
			reqsize += sizeof(WORD) + (int)m_animation.size();
		if(m_updateflags & OBJ_UPDATE_SPEED)
			reqsize += sizeof(float);
		if(m_updateflags & OBJ_UPDATE_EYEPOS)
			reqsize += STREAM_SIZE_VEC3;
		reqsize += pos.Serialize(write, NULL);

		if(!stream || 
			(write && stream->GetSpaceLeft() < reqsize))
			return reqsize;

		assert(GetID() < USHRT_MAX);
		stream->WriteWORD((WORD)GetID());
		stream->WriteDWORD(m_updateflags);
		if(m_updateflags & OBJ_UPDATE_FOV)
			stream->WriteFloat(m_fov);
		if(m_updateflags & OBJ_UPDATE_RESOURCE)
			stream->WriteString(m_resource);	
		if(m_updateflags & OBJ_UPDATE_ANIMATION)
			stream->WriteString(m_animation);
		if(m_updateflags & OBJ_UPDATE_SPEED)
			stream->WriteFloat(m_speed);
		if(m_updateflags & OBJ_UPDATE_EYEPOS)
			stream->WriteVec3(&m_eyepos);

		pos.Serialize(true, stream);
	}
	else
	{
		WORD id;
		DWORD updateflags;

		assert(stream);

		reqsize = stream->GetBytesRead();
		stream->ReadWORD(&id);
		m_id = id;
		stream->ReadDWORD(&updateflags);
		if(updateflags & OBJ_UPDATE_FOV)
			stream->ReadFloat(&m_fov);
		if(updateflags & OBJ_UPDATE_RESOURCE)
		{
			stream->ReadString(&m_resource);
			m_mesh = m_world->m_resman.GetModel(m_resource);
			if(m_mesh)
			{
				vec3_t min, max;
				m_mesh->GetAABB(&min, &max);
				SetAABB(min, max);
				if(m_animation != "")
					m_mesh->SetAnimationByName(&m_mesh_state, (char*)m_animation.c_str());
				else
					m_mesh->SetAnimation(&m_mesh_state, 0);
			}
			else
				SetAABB(vec3_t(-1,-1,-1), vec3_t(1,1,1));
		}
		if(updateflags & OBJ_UPDATE_ANIMATION)
		{
			stream->ReadString(&m_animation);
			if(m_mesh)
				m_mesh->SetAnimationByName(&m_mesh_state, (char*)m_animation.c_str());
		}
		if(updateflags & OBJ_UPDATE_SPEED)
			stream->ReadFloat(&m_speed);
		if(updateflags & OBJ_UPDATE_EYEPOS)
			stream->ReadVec3(&m_eyepos);

		pos.Serialize(false, stream);

		reqsize = stream->GetBytesRead()-reqsize;
	}

	return reqsize;
}

float CObj::GetSpeed()
{
	return m_speed;
}

void CObj::SetSpeed(float speed)
{
	if(speed != m_speed)
	{
		m_updateflags |= OBJ_UPDATE_SPEED;
		m_speed = speed;
	}
}

float CObj::GetRadius()
{
	return m_radius;
}

void CObj::SetAABB(const vec3_t& min, const vec3_t& max)
{
	if(min != m_min || max != m_max)
		m_updateflags |= OBJ_UPDATE_AABB;

	float length1 = min.AbsSquared();
	float length2 = max.AbsSquared();
	m_radius = length1 > length2 ? sqrt(length1) : sqrt(length2);
	m_min = min;
	m_max = max;
}

void CObj::GetAABB(vec3_t* min, vec3_t* max)
{
	*min = m_min;
	*max = m_max;
}

float CObj::GetFOV()
{
	return m_fov;
}

void CObj::SetFOV(float fov)
{
	if(fov != m_fov)
	{
		m_fov = fov;
		m_updateflags |= OBJ_UPDATE_FOV;
	}
}

std::string CObj::GetResource()
{
	return m_resource;
}

void CObj::SetResource(std::string resource)
{
	assert(resource.size() < USHRT_MAX);
	if(resource.size() >= USHRT_MAX)
		return;

	if(m_resource != resource)
	{
		m_resource = resource;	
		m_updateflags |= OBJ_UPDATE_RESOURCE;

		CModelMD2* model = m_world->m_resman.GetModel(m_resource);
		assert(model);
		if(model)
		{
			vec3_t min, max;
			model->GetAABB(&min, &max);
			SetAABB(min, max);
		}
	}
}

std::string CObj::GetAnimation()
{
	return m_animation;
}

void CObj::SetAnimation(std::string animation)
{
	assert(animation.size() < USHRT_MAX);
	if(animation.size() >= USHRT_MAX)
		return;

	if(animation != m_animation)
	{
		m_animation = animation;	
		m_updateflags |= OBJ_UPDATE_ANIMATION;
	}
}

vec3_t CObj::GetEyePos()
{
	return m_eyepos;
}

void CObj::SetEyePos(const vec3_t& eyepos)
{
	if(eyepos != m_eyepos)
	{
		m_eyepos = eyepos;
		m_updateflags |= OBJ_UPDATE_EYEPOS;
	}
}
