#include "GameObjPlayer.h"
#include "GameObjZombie.h"
#include "GameObjRocket.h"
#include "ServerClient.h" // for SERVER_UPDATETIME in gun sound spam prevention

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CGameObjPlayer::CGameObjPlayer(CWorld* world) : CGameObj(world)
{
    m_prim_triggered = 0;
    m_prim_triggered_time = 0;
}

CGameObjPlayer::~CGameObjPlayer(void)
{
}

void CGameObjPlayer::CmdFire(bool active)
{
    if(!m_prim_triggered && active)
        OnCmdFire();
    m_prim_triggered = active;
}

#define PLAYER_GUN_FIRESPEED            350 // fire a shot every x [ms]
#define PLAYER_GUN_DAMAGE               28  // damage points
#define PLAYER_GUN_MAX_DIST             (120.0f)   // max. weapon range

void CGameObjPlayer::OnCmdFire()
{
    if(GetWorld()->GetLeveltime() - m_prim_triggered_time < PLAYER_GUN_FIRESPEED)
        return;
    m_prim_triggered_time = GetWorld()->GetLeveltime();

    FireGun();
}

void CGameObjPlayer::FireGun()
{
    PlaySound(GetOrigin(),
              CLynx::GetBaseDirSound() + "rifle.ogg",
              PLAYER_GUN_FIRESPEED+10);

    world_obj_trace_t trace;
    vec3_t dir;
    GetLookDir().GetVec3(&dir, NULL, NULL);
    dir = -dir;
    trace.dir = dir;
    trace.start = GetOrigin();
    trace.excludeobj_id = GetID();
    if(GetWorld()->TraceObj(&trace, PLAYER_GUN_MAX_DIST))
    {
        if(trace.hitobj) // object hit
        {
            // we have to check if this object is actually a CGameObj* and not only a CObj*.
            if(trace.hitobj->GetType() > GAME_OBJ_TYPE_NONE)
            {
                CGameObj* hitobj = (CGameObj*)trace.hitobj;
                hitobj->DealDamage(PLAYER_GUN_DAMAGE, trace.hitpoint, trace.dir, this);
            }
        }
        else // level geometry hit
        {
            SpawnParticleDust(trace.hitpoint, trace.hitnormal);
        }
    }
}

void CGameObjPlayer::DealDamage(int damage, const vec3_t& hitpoint, const vec3_t& dir, CGameObj* dealer)
{
    CGameObj::DealDamage(0, hitpoint, dir, dealer);

    SpawnParticleBlood(hitpoint, dir, 3.0f);
}

