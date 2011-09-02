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

#define PLAYER_GUN_FIRESPEED            350
#define PLAYER_GUN_DAMAGE               28

void CGameObjPlayer::OnCmdFire()
{
    if(GetWorld()->GetLeveltime() - m_prim_triggered_time < PLAYER_GUN_FIRESPEED)
        return;
    m_prim_triggered_time = GetWorld()->GetLeveltime();

    FireGun();
}

void CGameObjPlayer::FireGun()
{
    // we have to prevent sound spamming from the machine gun
    // remember the sound obj and only play a new sound if the old one
    // is deleted
    // THE SOUND SYSTEM SUCKS ATM FIXME
    PlaySound(GetOrigin(),
              CLynx::GetBaseDirSound() + "rifle.ogg",
              PLAYER_GUN_FIRESPEED+10);


    world_obj_trace_t trace;
    vec3_t dir;
    GetLookDir().GetVec3(&dir, NULL, NULL);
    dir = -dir;
    trace.dir = dir;
    trace.start = GetOrigin();
    trace.excludeobj = GetID();
    if(GetWorld()->TraceObj(&trace))
    {
        CGameObj* hitobj = (CGameObj*)GetWorld()->GetObj(trace.objid);
        assert(hitobj);
        if(hitobj)
            hitobj->DealDamage(PLAYER_GUN_DAMAGE, trace.hitpoint, trace.dir, this);
    }
    else
    {
        // Test with level geometry
        bsp_sphere_trace_t spheretrace;
        spheretrace.start = trace.start;
        spheretrace.dir = dir * 800.0f;
        spheretrace.radius = 0.01f;
        GetWorld()->GetBSP()->TraceSphere(&spheretrace);
        if(spheretrace.f < 1.0f)
        {
            vec3_t hitpoint = spheretrace.start + spheretrace.f * spheretrace.dir;
            SpawnParticleDust(hitpoint, spheretrace.p.m_n);
        }
    }
}

void CGameObjPlayer::DealDamage(int damage, const vec3_t& hitpoint, const vec3_t& dir, CGameObj* dealer)
{
    CGameObj::DealDamage(0, hitpoint, dir, dealer);

    SpawnParticleBlood(hitpoint, dir);
}

