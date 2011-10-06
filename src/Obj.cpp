#include <string>
#include "lynx.h"
#include "Obj.h"
#include <math.h>
#include <sstream> // Particle Tokenizer
#include <string.h>
#include "ModelMD5.h"
#include "ModelMD2.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#define OBJ_STATE_ORIGIN         (1 <<  0)
#define OBJ_STATE_VEL            (1 <<  1)
#define OBJ_STATE_ROT            (1 <<  2)
#define OBJ_STATE_RADIUS         (1 <<  3)
#define OBJ_STATE_RESOURCE       (1 <<  4)
#define OBJ_STATE_ANIMATION      (1 <<  5)
#define OBJ_STATE_EYEPOS         (1 <<  6)
#define OBJ_STATE_FLAGS          (1 <<  7)
#define OBJ_STATE_PARTICLES      (1 <<  8)

#define OBJ_STATE_FULLUPDATE     ((1 << 9)-1)

int CObj::m_idpool = 0;

CObj::CObj(CWorld* world)
{
    assert(world);
    m_id = ++m_idpool;
    state.radius = 2*lynxmath::SQRT_2;
    state.eyepos = vec3_t(0,0.5f,0);
    state.animation = ANIMATION_NONE;
    state.flags = 0;
    m_mesh = NULL;
    m_mesh_state = NULL;
    m_sound = NULL;
    m_world = world;
    UpdateMatrix();

    // Local attributes
    m_locIsOnGround = false;
}

CObj::~CObj(void)
{
    SAFE_RELEASE(m_mesh_state);
}

void CObj::SetRot(const quaternion_t& rotation)
{
    bool bupdate = state.rot != rotation;
    if(bupdate)
    {
        state.rot = rotation;
        UpdateMatrix();
    }
}

void CObj::GetDir(vec3_t* dir, vec3_t* up, vec3_t* side) const
{
    state.rot.GetVec3(dir, up, side);
}

void CObj::UpdateMatrix()
{
    state.rot.ToMatrix(m);
}

float CObj::GetRadius() const
{
    return state.radius;
}

void CObj::SetRadius(float radius)
{
    state.radius = radius;
}

const std::string& CObj::GetResource() const
{
    return state.resource;
}

void CObj::SetResource(std::string resource)
{
    // longer than 256 characters? technically no problem,
    // but there is probably something wrong.
    assert(resource.size() < 256);
    if(resource.size() >= USHRT_MAX)
        return;

    if(state.resource != resource)
    {
        state.resource = resource;
        UpdateAnimation();
        if(m_mesh)
        {
            state.radius = m_mesh->GetSphere();
        }
    }
}

animation_t CObj::GetAnimation() const
{
    return state.animation;
}

void CObj::SetAnimation(animation_t animation)
{
    if(animation != state.animation)
    {
        state.animation = animation;
        UpdateAnimation();
    }
}

const vec3_t& CObj::GetEyePos() const
{
    return state.eyepos;
}

void CObj::SetEyePos(const vec3_t& eyepos)
{
    state.eyepos = eyepos;
}

OBJFLAGTYPE CObj::GetFlags() const
{
    return state.flags;
}

void CObj::SetFlags(OBJFLAGTYPE flags)
{
    state.flags = flags;
}

void CObj::AddFlags(OBJFLAGTYPE flags)
{
    state.flags |= flags;
}

void CObj::RemoveFlags(OBJFLAGTYPE flags)
{
    state.flags = state.flags & ~flags;
}

void CObj::UpdateParticles()
{
    if(!GetWorld()->IsClient())
        return;

    if(state.particles.size() < 1)
    {
        m_particlesys = std::auto_ptr<CParticleSystem>(NULL);
        return;
    }

    std::istringstream iss(state.particles);
    std::string psystem, pconfig;
    if(getline(iss, psystem, '|') && getline(iss, pconfig, '|'))
    {
        PROPERTYMAP properties;
        CParticleSystem::SetPropertyMap(pconfig, properties);
        m_particlesys = std::auto_ptr<CParticleSystem>(
                                        CParticleSystem::CreateSystem(
                                        psystem,
                                        properties,
                                        GetWorld()->GetResourceManager(),
                                        GetOrigin()
                                        ));
    }
    else
    {
        assert(0);
    }
}

void CObj::SetParticleSystem(const std::string psystem)
{
    state.particles = psystem;
    UpdateParticles();
}

const std::string& CObj::GetParticleSystemName() const
{
    return state.particles;
}

// DELTA COMPRESSION CODE ----------------------------------------

int DeltaDiffVec3(const vec3_t* newstate,
                  const vec3_t* oldstate,
                  const uint32_t flagparam,
                  uint32_t* updateflags,
                  CStream* stream)
{
    if(!oldstate || (*newstate != *oldstate)) {
        *updateflags |= flagparam;
        if(stream) stream->WriteVec3(*newstate);
        return sizeof(vec3_t);
    }
    return 0;
}

int DeltaDiffQuat(const quaternion_t* newstate,
                  const quaternion_t* oldstate,
                  const uint32_t flagparam,
                  uint32_t* updateflags,
                  CStream* stream)
{
    if(!oldstate || (*newstate != *oldstate)) {
        *updateflags |= flagparam;
        if(stream) stream->WriteQuat(*newstate);
        return sizeof(quaternion_t);
    }
    return 0;
}

int DeltaDiffQuatUnit(const quaternion_t* newstate,
                      const quaternion_t* oldstate,
                      const uint32_t flagparam,
                      uint32_t* updateflags,
                      CStream* stream)
{
    if(!oldstate || (*newstate != *oldstate)) {
        *updateflags |= flagparam;
        if(stream) stream->WriteQuatUnit(*newstate);
        return sizeof(quaternion_t);
    }
    return 0;
}

int DeltaDiffFloat(const float* newstate,
                   const float* oldstate,
                   const uint32_t flagparam,
                   uint32_t* updateflags,
                   CStream* stream)
{
    if(!oldstate || !(*newstate == *oldstate)) {
        *updateflags |= flagparam;
        if(stream) stream->WriteFloat(*newstate);
        return sizeof(float);
    }
    return 0;
}

int DeltaDiffInt16(const int16_t* newstate,
                   const int16_t* oldstate,
                   const uint32_t flagparam,
                   uint32_t* updateflags,
                   CStream* stream)
{
    if(!oldstate || *newstate != *oldstate) {
        *updateflags |= flagparam;
        if(stream) stream->WriteInt16(*newstate);
        return sizeof(int16_t);
    }
    return 0;
}

int DeltaDiffString(const std::string* newstate,
                    const std::string* oldstate,
                    const uint32_t flagparam,
                    uint32_t* updateflags,
                    CStream* stream)
{
    if(!oldstate || *newstate != *oldstate) {
        *updateflags |= flagparam;
        if(stream) stream->WriteString(*newstate);
        return (int)CStream::StringSize(*newstate);
    }
    return 0;
}

int DeltaDiffDWORD(const uint32_t* newstate,
                   const uint32_t* oldstate,
                   const uint32_t flagparam,
                   uint32_t* updateflags,
                   CStream* stream)
{
    if(!oldstate || *newstate != *oldstate) {
        *updateflags |= flagparam;
        if(stream) stream->WriteDWORD(*newstate);
        return sizeof(uint32_t);
    }
    return 0;
}

int DeltaDiffBytes(const uint8_t* newstate,
                   const uint8_t* oldstate,
                   const uint32_t flagparam,
                   uint32_t* updateflags,
                   CStream* stream,
                   const int size)
{
    int cmp;
    if(oldstate)
        cmp = memcmp(newstate, oldstate, size);
    else
        cmp = 0;
    if(!oldstate || cmp != 0) {
        *updateflags |= flagparam;
        if(stream) stream->WriteBytes(newstate, size);
        return size;
    }
    return 0;
}

bool CObj::Serialize(bool write, CStream* stream, int id, const obj_state_t* oldstate)
{
    assert(!(!write && oldstate));
    assert(stream);
    uint32_t updateflags = 0;
    if(!stream)
        return false;

    if(write)
    {
        assert(GetID() == id);
        assert(id < INT_MAX);
        CStream tempstream = stream->GetStream(); // we remember the position of the updateflags
        stream->WriteAdvance(sizeof(uint32_t)); // we advance by sizeof(DWORD) bytes

        DeltaDiffVec3(&state.origin,       oldstate ? &oldstate->origin : NULL,     OBJ_STATE_ORIGIN,     &updateflags, stream);
        DeltaDiffVec3(&state.vel,          oldstate ? &oldstate->vel : NULL,        OBJ_STATE_VEL,        &updateflags, stream);
        DeltaDiffQuat(&state.rot,          oldstate ? &oldstate->rot : NULL,        OBJ_STATE_ROT,        &updateflags, stream);
        DeltaDiffFloat(&state.radius,      oldstate ? &oldstate->radius : NULL,     OBJ_STATE_RADIUS,     &updateflags, stream);
        DeltaDiffString(&state.resource,   oldstate ? &oldstate->resource : NULL,   OBJ_STATE_RESOURCE,   &updateflags, stream);
        DeltaDiffInt16(&state.animation,   oldstate ? &oldstate->animation : NULL,  OBJ_STATE_ANIMATION,  &updateflags, stream);
        DeltaDiffVec3(&state.eyepos,       oldstate ? &oldstate->eyepos : NULL,     OBJ_STATE_EYEPOS,     &updateflags, stream);
        DeltaDiffBytes(&state.flags,       oldstate ? &oldstate->flags : NULL,      OBJ_STATE_FLAGS,      &updateflags, stream, sizeof(state.flags));
        DeltaDiffString(&state.particles,  oldstate ? &oldstate->particles : NULL,  OBJ_STATE_PARTICLES,  &updateflags, stream);

        tempstream.WriteDWORD(updateflags); // now we can write the updateflags

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
            stream->ReadQuat(&state.rot);
            UpdateMatrix();
        }
        if(updateflags & OBJ_STATE_RADIUS)
            stream->ReadFloat(&state.radius);
        if(updateflags & OBJ_STATE_RESOURCE)
            stream->ReadString(&state.resource);
        if(updateflags & OBJ_STATE_ANIMATION)
            stream->ReadInt16(&state.animation);
        if(updateflags & OBJ_STATE_EYEPOS)
            stream->ReadVec3(&state.eyepos);
        if(updateflags & OBJ_STATE_FLAGS)
            stream->ReadBytes(&state.flags, sizeof(state.flags));
        if(updateflags & OBJ_STATE_PARTICLES)
            stream->ReadString(&state.particles);

        m_id = id;
        if(updateflags & OBJ_STATE_RESOURCE || updateflags & OBJ_STATE_ANIMATION)
            UpdateAnimation();
        if(updateflags & OBJ_STATE_PARTICLES)
            UpdateParticles();
    }

    assert(!stream->GetReadOverflow() && !stream->GetWriteOverflow());
    return (updateflags != 0) && !stream->GetReadOverflow() && !stream->GetWriteOverflow();
}

void CObj::SetObjState(const obj_state_t* objstate, int id)
{
    m_id = id;
    bool resourcechange = objstate->resource != state.resource ||
                          objstate->animation != state.animation;
    bool rotationchange = objstate->rot != state.rot;
    bool particlechange = objstate->particles != state.particles;

    state = *objstate;
    if(resourcechange)
        UpdateAnimation();
    if(rotationchange)
        UpdateMatrix();
    if(particlechange)
        UpdateParticles();
}

void CObj::CopyObjStateFrom(const CObj* source)
{
    SetObjState(&source->state, m_id);
}

void CObj::UpdateAnimation() // FIXME the name is misleading, as this method also loads sounds
{
    m_mesh = NULL;
    m_sound = NULL;

    if(state.resource == "")
        return;

    if(state.resource.find(".md5") != std::string::npos)
    {
        CModelMD5* mesh = (CModelMD5*)m_world->GetResourceManager()->GetModel(state.resource);
        if(mesh != m_mesh)
        {
            if(m_mesh_state)
                delete m_mesh_state;
            m_mesh_state = new md5_state_t();
            m_mesh = mesh;
        }
        if(m_mesh)
            m_mesh->SetAnimation(m_mesh_state, state.animation);
    }
    else if(state.resource.find(".md2") != std::string::npos)
    {
        CModelMD2* mesh = (CModelMD2*)m_world->GetResourceManager()->GetModel(state.resource);
        if(mesh != m_mesh)
        {
            if(m_mesh_state)
                delete m_mesh_state;
            m_mesh_state = new md2_state_t();
            m_mesh = mesh;
        }
        if(m_mesh)
            m_mesh->SetAnimation(m_mesh_state, state.animation);
    }
    else if(state.resource.find(".ogg") != std::string::npos)
    {
        if(m_sound_state.soundpath != state.resource)
        {
            m_sound = m_world->GetResourceManager()->GetSound(state.resource);
            m_sound_state.init();
            m_sound_state.soundpath = state.resource;
        }
    }
    else
    {
        fprintf(stderr, "Unknown resource: %s\n", state.resource.c_str());
    }
}

