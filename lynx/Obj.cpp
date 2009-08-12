#include <string.h>
#include "lynx.h"
#include "Obj.h"
#include <math.h>

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#define OBJ_STATE_ORIGIN         (1 <<  0)
#define OBJ_STATE_VEL            (1 <<  1)
#define OBJ_STATE_ROT            (1 <<  2)
#define OBJ_STATE_FOV            (1 <<  3)
#define OBJ_STATE_RADIUS         (1 <<  4)
#define OBJ_STATE_RESOURCE       (1 <<  5)
#define OBJ_STATE_ANIMATION      (1 <<  6)
#define OBJ_STATE_MIN            (1 <<  7)
#define OBJ_STATE_MAX            (1 <<  8)
#define OBJ_STATE_EYEPOS         (1 <<  9)

#define OBJ_STATE_FULLUPDATE    ((1 << 10)-1)

#define DEFAULT_FOV		90.0f

int CObj::m_idpool = 0;

CObj::CObj(CWorld* world)
{
	assert(world);
	m_id = ++m_idpool;
    assert(m_id < USHRT_MAX); // Die id wird im Netzwerk als WORD übertragen
	//state.radius = lynxmath::SQRT_2;
    state.radius = 2*lynxmath::SQRT_2;
	state.min = vec3_t(-1,-1,-1);
	state.max = vec3_t(1,1,1);
	state.fov = DEFAULT_FOV;
	state.eyepos = vec3_t(0,1,0);
	m_mesh = NULL;
	m_world = world;
    UpdateMatrix();
	m_stream.Resize(512);
}

CObj::~CObj(void)
{
}

void CObj::UpdateMatrix()
{
	m.SetTransform(NULL, &state.rot);
}

float CObj::GetRadius()
{
	return state.radius;
}

void CObj::SetRadius(float radius)
{
    state.radius = radius;
}

void CObj::SetAABB(const vec3_t& min, const vec3_t& max)
{
	state.min = min;
	state.max = max;
}

void CObj::GetAABB(vec3_t* min, vec3_t* max)
{
	*min = state.min;
	*max = state.max;
}

float CObj::GetFOV()
{
	return state.fov;
}

void CObj::SetFOV(float fov)
{
    state.fov = fov;
}

std::string CObj::GetResource()
{
	return state.resource;
}

void CObj::SetResource(std::string resource)
{
	assert(resource.size() < USHRT_MAX);
	if(resource.size() >= USHRT_MAX)
		return;

	if(state.resource != resource)
	{
		state.resource = resource;
		UpdateProperties();
		if(m_mesh)
		{
			state.radius = m_mesh->GetSphere();
			vec3_t min, max;
			m_mesh->GetAABB(&min, &max);
			SetAABB(min, max);
		}
	}
}

std::string CObj::GetAnimation()
{
	return state.animation;
}

void CObj::SetAnimation(std::string animation)
{
	assert(animation.size() < USHRT_MAX);
	if(animation.size() >= USHRT_MAX)
		return;

	if(animation != state.animation)
	{
		state.animation = animation;
		UpdateProperties();
	}
}

vec3_t CObj::GetEyePos()
{
	return state.eyepos;
}

void CObj::SetEyePos(const vec3_t& eyepos)
{
    state.eyepos = eyepos;
}

// DELTA COMPRESSION CODE (ugly) ------------------------------------

int DeltaDiffVec3(const vec3_t* newstate,
                  const vec3_t* oldstate,
                  const DWORD flagparam, 
                  DWORD* updateflags,
                  CStream* stream)
{
    if(!oldstate || !(*newstate == *oldstate)) {
        *updateflags |= flagparam;
        if(stream) stream->WriteVec3(*newstate);
        return STREAM_SIZE_VEC3;
    }
    return 0;
}

int DeltaDiffFloat(const float* newstate,
                   const float* oldstate,
                   const DWORD flagparam, 
                   DWORD* updateflags,
                   CStream* stream)
{
    if(!oldstate || !(*newstate == *oldstate)) {
        *updateflags |= flagparam;
        if(stream) stream->WriteFloat(*newstate);
        return sizeof(float);
    }
    return 0;
}

int DeltaDiffString(const std::string* newstate,
                    const std::string* oldstate,
                    const DWORD flagparam, 
                    DWORD* updateflags,
                    CStream* stream)
{
    if(!oldstate || *newstate != *oldstate) {
        *updateflags |= flagparam;
        if(stream) stream->WriteString(*newstate);
        return (int)CStream::StringSize(*newstate);
    }
    return 0;
}

int DeltaDiffDWORD(const DWORD* newstate,
                   const DWORD* oldstate,
                   const DWORD flagparam, 
                   DWORD* updateflags,
                   CStream* stream)
{
    if(!oldstate || *newstate != *oldstate) {
        *updateflags |= flagparam;
        if(stream) stream->WriteDWORD(*newstate);
        return sizeof(DWORD);
    }
    return 0;
}

bool CObj::Serialize(bool write, CStream* stream, int id, const obj_state_t* oldstate)
{
    assert(!(!write && oldstate));
    assert(stream);
    DWORD updateflags = 0;

	if(write)
	{
        assert(GetID() == id);
		assert(id < USHRT_MAX);
		m_stream.ResetWritePosition();

		DeltaDiffVec3(&state.origin,      oldstate ? &oldstate->origin : NULL,    OBJ_STATE_ORIGIN,     &updateflags, &m_stream);
        DeltaDiffVec3(&state.vel,         oldstate ? &oldstate->vel : NULL,       OBJ_STATE_VEL,        &updateflags, &m_stream);
        DeltaDiffVec3(&state.rot,         oldstate ? &oldstate->rot : NULL,       OBJ_STATE_ROT,        &updateflags, &m_stream);
        DeltaDiffFloat(&state.fov,        oldstate ? &oldstate->fov : NULL,       OBJ_STATE_FOV,        &updateflags, &m_stream);
        DeltaDiffFloat(&state.radius,     oldstate ? &oldstate->radius : NULL,    OBJ_STATE_RADIUS,     &updateflags, &m_stream);
        DeltaDiffString(&state.resource,  oldstate ? &oldstate->resource : NULL,  OBJ_STATE_RESOURCE,   &updateflags, &m_stream);
        DeltaDiffString(&state.animation, oldstate ? &oldstate->animation : NULL, OBJ_STATE_ANIMATION,  &updateflags, &m_stream);
        DeltaDiffVec3(&state.min,         oldstate ? &oldstate->min : NULL,       OBJ_STATE_MIN,        &updateflags, &m_stream);
        DeltaDiffVec3(&state.max,         oldstate ? &oldstate->max : NULL,       OBJ_STATE_MAX,        &updateflags, &m_stream);
        DeltaDiffVec3(&state.eyepos,      oldstate ? &oldstate->eyepos : NULL,    OBJ_STATE_EYEPOS,     &updateflags, &m_stream);
        stream->WriteDWORD(updateflags);
        stream->WriteStream(m_stream);

        assert(oldstate ? 1 : (updateflags == OBJ_STATE_FULLUPDATE));
	}
	else
	{
        stream->ReadDWORD(&updateflags);

        if(updateflags & OBJ_STATE_ORIGIN)
            stream->ReadVec3(&state.origin);
        if(updateflags & OBJ_STATE_VEL)
            stream->ReadVec3(&state.vel);
        if(updateflags & OBJ_STATE_ROT)
        {
            stream->ReadVec3(&state.rot);
            UpdateMatrix();
        }
        if(updateflags & OBJ_STATE_FOV)
            stream->ReadFloat(&state.fov);
        if(updateflags & OBJ_STATE_RADIUS)
            stream->ReadFloat(&state.radius);
        if(updateflags & OBJ_STATE_RESOURCE)
            stream->ReadString(&state.resource);
        if(updateflags & OBJ_STATE_ANIMATION)
            stream->ReadString(&state.animation);
        if(updateflags & OBJ_STATE_MIN)
            stream->ReadVec3(&state.min);
        if(updateflags & OBJ_STATE_MAX)
            stream->ReadVec3(&state.max);
        if(updateflags & OBJ_STATE_EYEPOS)
            stream->ReadVec3(&state.eyepos);

        m_id = id;
        if(updateflags & OBJ_STATE_RESOURCE || updateflags & OBJ_STATE_ANIMATION)
		{
			UpdateProperties();
		}
	}

	return updateflags != 0;
}

void CObj::SetObjState(const obj_state_t* objstate, int id)
{
	m_id = id;
    bool resourcechange = objstate->resource != state.resource || 
                          objstate->animation != state.animation;
	state = *objstate;
    if(resourcechange)
	    UpdateProperties();
}

void CObj::UpdateProperties()
{
	m_mesh = m_world->GetResourceManager()->GetModel(state.resource);
	if(state.animation != "")
		m_mesh->SetAnimationByName(&m_mesh_state, (char*)state.animation.c_str());
	else
		m_mesh->SetAnimation(&m_mesh_state, 0);
	UpdateMatrix();
}