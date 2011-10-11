#include "GameObjPlayer.h"
#include "GameObjZombie.h"
#include "GameObjRocket.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

// <weapon registry>
static const CWeapon g_weapon_registry[] =
{
//    id              Readable name        dmg  MD5 model                         rate   range    ammo
    { WEAPON_NONE   , "NULL"            ,    0  , ""                              , 1000 , 0.0f   , -1 },
    { WEAPON_ROCKET , "Rocket Launcher" ,   80  , "rocket/rocketlauncher.md5mesh" ,  667 , 100.0f , 30 },
    { WEAPON_GUN    , "Machine Gun"     ,  100  , "gun/gun.md5mesh"               , 1200 , 120.0f , 15 }
};

const unsigned int g_weapon_count = sizeof(g_weapon_registry)/sizeof(g_weapon_registry[0]);
const int CGameObjPlayer::GetWeaponInfoByType(weapon_type_t weapon)
{
    for(unsigned int i=0;i<g_weapon_count;i++)
        if(g_weapon_registry[i].type == weapon)
            return i;

    assert(0); // something stupid happened
    return 0;
}
// </weapon registry>

CGameObjPlayer::CGameObjPlayer(CWorld* world) : CGameObj(world)
{
    m_prim_triggered = 0;
    m_prim_triggered_time = 0;

    m_clientid = -1;
    m_weapon_id = GetWeaponInfoByType(WEAPON_NONE);
}

CGameObjPlayer::~CGameObjPlayer(void)
{

}

const CWeapon* CGameObjPlayer::GetWeapon()
{
    assert(m_weapon_id < (int)g_weapon_count);

    // make sure we always return something to work with
    if(m_weapon_id < 0)
    {
        assert(0); // double check this
        return &g_weapon_registry[0]; // return NULL weapon
    }

    return &g_weapon_registry[m_weapon_id];
}

void CGameObjPlayer::SetClientID(CGameLogic* logic, int clientid)
{
    m_gamelogic = logic;
    m_clientid = clientid;
}

bool CGameObjPlayer::IsClient()
{
    return GetClientID() != -1;
}

CClientInfo* CGameObjPlayer::GetClient()
{
    assert(IsClient());
    if(!IsClient() || m_gamelogic == NULL)
        return NULL;

    assert(m_gamelogic->GetServer());
    CClientInfo* client = m_gamelogic->GetServer()->GetClient(GetClientID());
    assert(client);
    return client;
}

// Weapon stuff

void CGameObjPlayer::ActivateRocket()
{
    if(GetWeapon()->type == WEAPON_ROCKET)
        return;

    m_weapon_id = GetWeaponInfoByType(WEAPON_ROCKET);

    CClientInfo* client = GetClient();
    assert(client);

    if(client)
        client->hud.weapon = GetWeapon()->resource;
}

void CGameObjPlayer::ActivateGun()
{
    if(GetWeapon()->type == WEAPON_GUN)
        return;

    m_weapon_id = GetWeaponInfoByType(WEAPON_GUN);

    CClientInfo* client = GetClient();
    assert(client);
    if(client)
        client->hud.weapon = GetWeapon()->resource;
}

void CGameObjPlayer::CmdFire(bool active)
{
    const unsigned int timedelta = GetWorld()->GetLeveltime() - m_prim_triggered_time;
    CClientInfo* client = GetClient();
    // check if weapon animation should be idle
    if(client && !active && timedelta > GetWeapon()->firespeed-100)
        client->hud.weapon_animation = ANIMATION_IDLE;

    if(!m_prim_triggered && active)
    {
        if(timedelta < GetWeapon()->firespeed)
            return;
        m_prim_triggered_time = GetWorld()->GetLeveltime();
        if(client)
            client->hud.weapon_animation = ANIMATION_FIRE;

        switch(GetWeapon()->type)
        {
            case WEAPON_ROCKET:
                FireRocket();
                break;
            case WEAPON_GUN:
                FireGun();
                break;
            default:
                assert(0); // weapon type kaputt
                break;
        }
    }
    m_prim_triggered = active;
}

void CGameObjPlayer::FireGun()
{
    CGameObj::CreateSoundObj(GetOrigin(),
                        CLynx::GetBaseDirSound() + "rifle.ogg",
                        GetWeapon()->firespeed+10);

    world_obj_trace_t trace;
    vec3_t dir;
    GetLookDir().GetVec3(&dir, NULL, NULL);
    dir = -dir;
    trace.dir = dir;
    trace.start = GetOrigin();
    trace.excludeobj_id = GetID();
    if(GetWorld()->TraceObj(&trace, GetWeapon()->maxdist)) // hit something
    {
        if(trace.hitobj) // object hit
        {
            // we have to check if this object is actually
            // a CGameObj* and not only a CObj*.
            if(trace.hitobj->GetType() > GAME_OBJ_TYPE_NONE)
            {
                CGameObj* hitobj = (CGameObj*)trace.hitobj;
                bool killed;
                hitobj->DealDamage(GetWeapon()->damage,
                                   trace.hitpoint,
                                   trace.dir,
                                   this,
                                   killed);

                if(killed)
                {
                    assert(IsClient());
                    CClientInfo* client = GetClient();
                    if(client)
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

void CGameObjPlayer::FireRocket()
{
    CreateSoundObj(GetOrigin(),
              CLynx::GetBaseDirSound() + "rifle.ogg",
              GetWeapon()->firespeed+10);

    vec3_t dir, up, side;
    GetLookDir().GetVec3(&dir, &up, &side);
    dir = -dir;

    CGameObjRocket* rocket = new CGameObjRocket(GetWorld());

    vec3_t rocketstart = GetOrigin() +
                         up*GetRadius()*0.8f +
                         dir*1.1f*GetRadius() +
                         side*0.5f;

    rocket->SetOrigin(rocketstart);
    rocket->SetVel(dir * rocket->GetRocketSpeed());
    rocket->SetRot(GetLookDir());
    rocket->SetOwner(GetID()); // set rocket ownership to player entity
    GetWorld()->AddObj(rocket); // godspeed little projectile
}

void CGameObjPlayer::Respawn(const vec3_t& location,
                             const quaternion_t& rotation)
{
    CGameObj::Respawn(location, rotation);

    m_prim_triggered = 0;
    m_prim_triggered_time = 0;
}

void CGameObjPlayer::DealDamage(int damage, const vec3_t& hitpoint, const vec3_t& dir, CGameObj* dealer, bool& killed_me)
{
    CGameObj::DealDamage(damage, hitpoint, dir, dealer, killed_me);

    if(damage > 0)
    {
        //SpawnParticleBlood(hitpoint, dir, killed_me ? 6.0f: 3.0f); // more blood, if killed
        // Experimental: spawn blood in front of player's face
        // to get visual feedback on damage
        // EyePos
        SpawnParticleBlood(GetOrigin(), dir, killed_me ? 6.0f: 3.0f); // more blood, if killed
    }

    if(killed_me)
        Respawn();

    // Update the HUD
    CClientInfo* client = GetClient();
    if(client)
        client->hud.health = GetHealth();
}

void CGameObjPlayer::Respawn()
{
    vec3_t spawn = GetWorld()->GetBSP()->GetRandomSpawnPoint().point;
    SetOrigin(spawn);
    SetHealth(100);
}

void CGameObjPlayer::InitHUD()
{
    CClientInfo* client = GetClient();
    assert(client);
    if(client)
    {
        client->hud.health = GetHealth();
        client->hud.score = 0;
        client->hud.weapon = GetWeapon()->resource;
        client->hud.weapon_animation = ANIMATION_IDLE;
    }
}

