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
#define OBJ_STATE_SPEED          (1 <<  3)
#define OBJ_STATE_FOV            (1 <<  4)
#define OBJ_STATE_RADIUS         (1 <<  5)
#define OBJ_STATE_RESOURCE       (1 <<  6)
#define OBJ_STATE_ANIMATION      (1 <<  7)
#define OBJ_STATE_MIN            (1 <<  8)
#define OBJ_STATE_MAX            (1 <<  9)
#define OBJ_STATE_EYEPOS         (1 << 10)

#define OBJ_STATE_FULLUPDATE    ((1 << 11)-1)

#define DEFAULT_FOV		90.0f

int CObj::m_idpool = 0;

CObj::CObj(CWorld* world)
{
	assert(world);
	m_id = ++m_idpool;
	state.speed = 0.0f;
	state.radius = lynxmath::SQRT_2;
	state.min = vec3_t(-1,-1,-1);
	state.max = vec3_t(1,1,1);
	state.fov = DEFAULT_FOV;
	state.eyepos = vec3_t(0,1,0);
	m_mesh = NULL;
	m_world = world;
    UpdateMatrix();
}

CObj::~CObj(void)
{
}

void CObj::UpdateMatrix()
{
	m.SetTransform(NULL, &state.rot);
}

float CObj::GetSpeed()
{
	return state.speed;
}

void CObj::SetSpeed(float speed)
{
    state.speed = speed;
}

float CObj::GetRadius()
{
	return state.radius;
}

void CObj::SetAABB(const vec3_t& min, const vec3_t& max)
{
	float length1 = min.AbsSquared();
	float length2 = max.AbsSquared();
	state.radius = length1 > length2 ? sqrt(length1) : sqrt(length2);
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

		CModelMD2* model = m_world->m_resman.GetModel(resource);
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

bool CObj::Serialize(bool write, CStream* stream, const obj_state_t* oldstate)
{
    assert(!(!write && oldstate));
    assert(stream);
    DWORD updateflags = 0;

	if(write)
	{
		assert(GetID() < USHRT_MAX);
        CStream tempstream(8192); // FIXME
        
        stream->WriteWORD((WORD)GetID()); // FIXME muss die id in den obj serialize stream?
        DeltaDiffVec3(&state.origin,      oldstate ? &oldstate->origin : NULL,    OBJ_STATE_ORIGIN,     &updateflags, &tempstream);
        DeltaDiffVec3(&state.vel,         oldstate ? &oldstate->vel : NULL,       OBJ_STATE_VEL,        &updateflags, &tempstream);
        DeltaDiffVec3(&state.rot,         oldstate ? &oldstate->rot : NULL,       OBJ_STATE_ROT,        &updateflags, &tempstream);
        DeltaDiffFloat(&state.speed,      oldstate ? &oldstate->speed : NULL,     OBJ_STATE_SPEED,      &updateflags, &tempstream);
        DeltaDiffFloat(&state.fov,        oldstate ? &oldstate->fov : NULL,       OBJ_STATE_FOV,        &updateflags, &tempstream);
        DeltaDiffFloat(&state.radius,     oldstate ? &oldstate->radius : NULL,    OBJ_STATE_RADIUS,     &updateflags, &tempstream);
        DeltaDiffString(&state.resource,  oldstate ? &oldstate->resource : NULL,  OBJ_STATE_RESOURCE,   &updateflags, &tempstream);
        DeltaDiffString(&state.animation, oldstate ? &oldstate->animation : NULL, OBJ_STATE_ANIMATION,  &updateflags, &tempstream);
        DeltaDiffVec3(&state.min,         oldstate ? &oldstate->min : NULL,       OBJ_STATE_MIN,        &updateflags, &tempstream);
        DeltaDiffVec3(&state.max,         oldstate ? &oldstate->max : NULL,       OBJ_STATE_MAX,        &updateflags, &tempstream);
        DeltaDiffVec3(&state.eyepos,      oldstate ? &oldstate->eyepos : NULL,    OBJ_STATE_EYEPOS,     &updateflags, &tempstream);
        stream->WriteDWORD(updateflags);
        stream->WriteStream(tempstream);

        assert(oldstate ? 1 : (updateflags == OBJ_STATE_FULLUPDATE));
	}
	else
	{
		WORD id;

		stream->ReadWORD(&id);
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
        if(updateflags & OBJ_STATE_SPEED)
            stream->ReadFloat(&state.speed);
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
		CModelMD2* mesh = m_world->m_resman.GetModel(state.resource);
        if(!m_mesh && mesh && (updateflags & OBJ_STATE_RESOURCE))
		{
            m_mesh = mesh;
			vec3_t min, max;
			m_mesh->GetAABB(&min, &max);
			SetAABB(min, max);
			if(state.animation != "")
				m_mesh->SetAnimationByName(&m_mesh_state, (char*)state.animation.c_str());
			else
				m_mesh->SetAnimation(&m_mesh_state, 0);
		}
	}

	return updateflags != 0;
}