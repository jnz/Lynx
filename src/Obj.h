#pragma once

class CObj;
struct obj_state_t;
#include "math/vec3.h"
#include "math/matrix.h"
#include "math/quaternion.h"
#include "Stream.h"
#include <string>
#include "World.h"
#include "Model.h"
#include "Sound.h"
#include <memory>
#include "ParticleSystem.h"

#define OBJ_FLAGS_ELASTIC       (1 << 0)
#define OBJ_FLAGS_NOGRAVITY     (1 << 1)
#define OBJ_FLAGS_GHOST         (1 << 2)

#define OBJFLAGTYPE             uint8_t

// Ghost objects:
// --------------
// For light weight objects with no graphical representation.
// Consequences of the GHOST flag:
//  - World::GetNearObj skips these objects
//  - World::ObjMove skips these objects
//  - World::TraceObj skips these objects
//  - The object is not rendered

// Serialize Helper Functions: Compare if newstate != oldstate, update updateflags with flagparam and write to stream (if not null)
int DeltaDiffVec3(const vec3_t* newstate,
                  const vec3_t* oldstate,
                  const uint32_t flagparam,
                  uint32_t* updateflags,
                  CStream* stream);
int DeltaDiffQuat(const quaternion_t* newstate,
                  const quaternion_t* oldstate,
                  const uint32_t flagparam,
                  uint32_t* updateflags,
                  CStream* stream);
int DeltaDiffQuatUnit(const quaternion_t* newstate,
                      const quaternion_t* oldstate,
                      const uint32_t flagparam,
                      uint32_t* updateflags,
                      CStream* stream); //Unit quat
int DeltaDiffFloat(const float* newstate,
                   const float* oldstate,
                   const uint32_t flagparam,
                   uint32_t* updateflags,
                   CStream* stream);
int DeltaDiffInt16(const int16_t* newstate,
                   const int16_t* oldstate,
                   const uint32_t flagparam,
                   uint32_t* updateflags,
                   CStream* stream);
int DeltaDiffString(const std::string* newstate,
                    const std::string* oldstate,
                    const uint32_t flagparam,
                    uint32_t* updateflags,
                    CStream* stream);
int DeltaDiffDWORD(const uint32_t* newstate,
                   const uint32_t* oldstate,
                   const uint32_t flagparam,
                   uint32_t* updateflags,
                   CStream* stream);
int DeltaDiffBytes(const uint8_t* newstate,
                   const uint8_t* oldstate,
                   const uint32_t flagparam,
                   uint32_t* updateflags,
                   CStream* stream,
                   const int size);


// If you change this in any way, update the Serialize function!
// -------------------------------------------------------------
// The basic object state has no acceleration (f=ma)
// property. The Lynx physical model is in general pretty
// simple. But forces should be handled on the server side
// anyway.
struct obj_state_t
{
    vec3_t          origin;         // Position
    vec3_t          vel;            // Direction/Velocity
    quaternion_t    rot;            // Rotation

    float           radius;
    std::string     resource;
    animation_t     animation;
    OBJFLAGTYPE     flags;
    std::string     particles;      // Particlesystem bound to this object. The
                                    // string looks something like this:
                                    // "blood|dx=0.1,dy=0.6,dz=23".
};

class CObj
{
public:
    CObj(CWorld* world);
    virtual ~CObj(void);

    int         GetID() const { return m_id; }
    virtual int GetType() const { return 0; } // poor man's rtti

    // Serialize:
    // Write object to a byte stream. If there is an oldstate available,
    // the delta compression/decompression is used.
    // For writing, id should match GetID(), for reading, id is the new object id.
    bool        Serialize(bool write, CStream* stream,
                          int id, const obj_state_t* oldstate=NULL);

    obj_state_t GetObjState() const { return state; }
    void        SetObjState(const obj_state_t* objstate, int id);
    // CopyObjStateFrom: Copy from other object.
    // This will also update the resources.
    void        CopyObjStateFrom(const CObj* source);

    const vec3_t&       GetOrigin() const { return state.origin; }
    void                SetOrigin(const vec3_t& origin) { state.origin = origin; }
    const vec3_t&       GetVel() const { return state.vel; }
    void                SetVel(const vec3_t& velocity) { state.vel = velocity; }
    const quaternion_t& GetRot() const { return state.rot; }
    void                SetRot(const quaternion_t& rotation);
    void                GetDir(vec3_t* dir, vec3_t* up, vec3_t* side) const;

    float               GetRadius() const; // Max. object sphere size
    void                SetRadius(float radius);
    const std::string&  GetResource() const;
    void                SetResource(std::string resource);
    animation_t         GetAnimation() const;
    void                SetAnimation(animation_t animation);
    OBJFLAGTYPE         GetFlags() const;
    void                SetFlags(OBJFLAGTYPE flags);
    void                AddFlags(OBJFLAGTYPE flags);
    void                RemoveFlags(OBJFLAGTYPE flags);
    void                SetParticleSystem(const std::string psystem);
    const std::string&  GetParticleSystemName() const;

    // Local Attributes
    // Has this object touched the ground? Set by World::ObjMove
    bool                locGetIsOnGround() const { return m_locIsOnGround; }

    // Rotation
    // Direct access to the rotation matrix, used by the renderer
    const matrix_t*     GetRotMatrix() const { return &m; }

    // Model Data
    CModel*             GetMesh() const { return m_mesh; }
    model_state_t*      GetMeshState() { return m_mesh_state; }

    // Sound Data
    const CSound*       GetSound() const { return m_sound; }
    sound_state_t*      GetSoundState() { return &m_sound_state; }

    // Particle Systems
    CParticleSystem*    GetParticleSystem() { return m_particlesys.get(); }

    // Wallhit notification, called by World::ObjMove(...)
    virtual void        OnHitWall(const vec3_t& location, const vec3_t& normal) {}

protected:
    obj_state_t         state; // Core data

    // UpdateResources: make sure that m_mesh points to the right model
    // (i.e. the one set in state.resource)
    // This will also load sounds, if the object has a soundfile
    // in the state.resource string.
    void                UpdateResources();

    // Animation extension
    CModel*             m_mesh;
    model_state_t*      m_mesh_state;

    // Sound
    CSound*             m_sound;
    sound_state_t       m_sound_state;

    // Rotation extension
    void                UpdateMatrix();
    matrix_t            m;

    // Particle extension
    void                UpdateParticles();
    std::auto_ptr<CParticleSystem> m_particlesys;

    // Local Attributes
    bool                m_locIsOnGround;

    friend class CWorld;
    CWorld*             GetWorld() { return m_world; }

private:
    // Don't touch these
    int                 m_id;
    CWorld*             m_world;

    static int          m_idpool; // unique id number for new objects
};

