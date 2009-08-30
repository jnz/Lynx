#include "GameObjPlayer.h"
#include "GameObjZombie.h"

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

void CGameObjPlayer::OnCmdFire()
{
    if(GetWorld()->GetLeveltime() - m_prim_triggered_time < 50)
        return;
    m_prim_triggered_time = GetWorld()->GetLeveltime();

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
        //if(!hitobj->IsClient())
        hitobj->DealDamage(20, trace.hitpoint, trace.dir, this);
    }
    else
    {
        // Kein Objekt getroffen, mit Levelgeometrie testen
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