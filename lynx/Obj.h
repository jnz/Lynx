#pragma once

class CObj;
struct obj_state_t;
#include "math/vec3.h"
#include "math/matrix.h"
#include "math/quaternion.h"
#include "Stream.h"
#include <string>
#include "World.h"
#include "ModelMD2.h"

#define OBJ_FLAGS_ELASTIC       (1 << 0)

#define OBJFLAGTYPE             BYTE


// Serialize Helper Functions: Compare if newstate != oldstate, update updateflags with flagparam and write to stream (if not null)
int DeltaDiffVec3(const vec3_t* newstate,
                  const vec3_t* oldstate,
                  const DWORD flagparam, 
                  DWORD* updateflags,
                  CStream* stream);
int DeltaDiffQuat(const quaternion_t* newstate,
                  const quaternion_t* oldstate,
                  const DWORD flagparam, 
                  DWORD* updateflags,
                  CStream* stream);
int DeltaDiffFloat(const float* newstate,
                   const float* oldstate,
                   const DWORD flagparam, 
                   DWORD* updateflags,
                   CStream* stream);
int DeltaDiffInt16(const INT16* newstate,
                   const INT16* oldstate,
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
int DeltaDiffBytes(const BYTE* newstate,
                   const BYTE* oldstate,
                   const DWORD flagparam, 
                   DWORD* updateflags,
                   CStream* stream,
                   const int size);


// If you change this, update the Serialize function
struct obj_state_t
{
	vec3_t	        origin;			// Position
	vec3_t	        vel;     		// Direction/Velocity
	quaternion_t    rot;			// Rotation (x = pitch, y = yaw, z = roll)

	float		    radius;
	std::string     resource;
    INT16           animation;
    INT16           nextanimation;
    vec3_t		    eyepos;
    OBJFLAGTYPE     flags;
};


class CObj
{
public:
    CObj(CWorld* world);
    virtual ~CObj(void);

    int			GetID() const { return m_id; }

    bool		Serialize(bool write, CStream* stream, int id, const obj_state_t* oldstate=NULL); // Objekt in einen Byte-Stream schreiben. Wenn oldstate ungleich NULL, wird nur die Differenz geschrieben, gibt true zurück, wenn sich objekt durch geändert hat (beim lesen) oder wenn es sich von oldstate unterscheidet

    obj_state_t GetObjState() const { return state; }
    void		SetObjState(const obj_state_t* objstate, int id);
    void        CopyObjStateFrom(const CObj* source); // Eigenschaften von anderem Objekt kopieren

    const vec3_t GetOrigin() const { return state.origin; }
    void         SetOrigin(const vec3_t& origin) { state.origin = origin; }
    const vec3_t GetVel() const { return state.vel; }
    void         SetVel(const vec3_t& velocity) { state.vel = velocity; }
    const quaternion_t GetRot() const { return state.rot; }
    void        SetRot(const quaternion_t& rotation);
    void        GetDir(vec3_t* dir, vec3_t* up, vec3_t* side) const;

    float		GetRadius() const; // Max. Object sphere size
    void        SetRadius(float radius);
    std::string GetResource() const;
    void		SetResource(std::string resource);
    INT16       GetAnimation() const;
    void		SetAnimation(INT16 animation);
    INT16       GetNextAnimation() const;
    void		SetNextAnimation(INT16 animation);
    vec3_t		GetEyePos() const;
    void		SetEyePos(const vec3_t& eyepos);
    OBJFLAGTYPE GetFlags() const;
    void        SetFlags(OBJFLAGTYPE flags);
    void        AddFlags(OBJFLAGTYPE flags);
    void        RemoveFlags(OBJFLAGTYPE flags);

    // Local Attributes
    bool        locGetIsOnGround() const { return m_locIsOnGround; }

    int         GetAnimationFromName(const char* name) const;

    // Model Data
    const CModelMD2* GetMesh() const { return m_mesh; }
    md2_state_t*     GetMeshState() { return &m_mesh_state; }

    const matrix_t* GetRotMatrix() const { return &m; }


protected:
    CModelMD2*	m_mesh;
    md2_state_t m_mesh_state;
    void	    UpdateMatrix();
    matrix_t    m;

    void		UpdateProperties();
    obj_state_t state;

    // Local Attributes
    bool        m_locIsOnGround;
    friend class CWorld;

    CWorld*     GetWorld() { return m_world; }

private:
    // Don't touch these
    int			m_id;
    CWorld*		m_world;

    static int m_idpool;
};
