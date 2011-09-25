#include "GameObjPlayer.h"
#include "GameObjZombie.h"
#include "GameObjRocket.h"

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

#define PLAYER_GUN_FIRESPEED            250 // fire a shot every x [ms]
#define PLAYER_GUN_DAMAGE               28  // damage points
#define PLAYER_GUN_MAX_DIST             (120.0f)   // max. weapon range

#define PLAYER_ROCKET_FIRESPEED         650 // fire a shot every x [ms]
#define PLAYER_ROCKET_DAMAGE            68  // splash damage

void CGameObjPlayer::CmdFire(bool active, CClientInfo* client)
{
    if(!m_prim_triggered && active)
    {
        if(GetWorld()->GetLeveltime() - m_prim_triggered_time < PLAYER_ROCKET_FIRESPEED)
            return;
        m_prim_triggered_time = GetWorld()->GetLeveltime();

        //FireGun(client);
        FireRocket(client);
    }
    m_prim_triggered = active;
}

void CGameObjPlayer::FireGun(CClientInfo* client)
{
    CGameObj::CreateSoundObj(GetOrigin(),
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
                bool killed;
                hitobj->DealDamage(PLAYER_GUN_DAMAGE, trace.hitpoint, trace.dir, this, killed);

                if(killed)
                {
                    client->hud.score++;
                }
            }
        }
        else // level geometry hit
        {
            SpawnParticleDust(trace.hitpoint, trace.hitnormal);
        }
    }
}

void CGameObjPlayer::FireRocket(CClientInfo* client)
{
    CreateSoundObj(GetOrigin(),
              CLynx::GetBaseDirSound() + "rifle.ogg",
              PLAYER_ROCKET_FIRESPEED+10);

    vec3_t dir, up, side;
    GetLookDir().GetVec3(&dir, &up, &side);
    dir = -dir;

    CGameObjRocket* rocket = new CGameObjRocket(GetWorld());

    rocket->SetOrigin(GetOrigin() + GetEyePos() + dir*GetRadius() + side*0.5f + up*0.1f);
    rocket->SetVel(dir * rocket->GetRocketSpeed());
    rocket->SetRot(GetLookDir());
    rocket->SetOwner(GetID()); // set rocket ownership to player entity
    GetWorld()->AddObj(rocket); // godspeed little projectile
}

void CGameObjPlayer::DealDamage(int damage, const vec3_t& hitpoint, const vec3_t& dir, CGameObj* dealer, bool& killed_me)
{
    CGameObj::DealDamage(damage, hitpoint, dir, dealer, killed_me);

    SpawnParticleBlood(hitpoint, dir, killed_me ? 6.0f: 3.0f); // more blood, if killed
    if(killed_me)
        Respawn();
}

void CGameObjPlayer::Respawn()
{
    vec3_t spawn = GetWorld()->GetBSP()->GetRandomSpawnPoint().point;
    SetOrigin(spawn);
    SetHealth(100);
}

