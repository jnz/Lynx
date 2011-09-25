#include "GameObjRocket.h"
#include "ParticleSystemRocket.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CGameObjRocket::CGameObjRocket(CWorld* world) : CGameObj(world)
{
    SetOwner(0); // someone should take later the ownership
    SetResource(CLynx::GetBaseDirModel() + "rocket/projectile.md5mesh");
    SetAnimation(ANIMATION_NONE);
    SetRadius(0.2f);
    // rocket think function:
    // m_think.AddFunc(new CThinkFuncRocket(GetWorld()->GetLeveltime() + 50, GetWorld(), this));

    // remove the rocket after 14 seconds no matter what:
    m_think.AddFunc(new CThinkFuncRemoveMe(GetWorld()->GetLeveltime() + 14000, GetWorld(), this));
    AddFlags(OBJ_FLAGS_NOGRAVITY); // feynman would be angry, but this is what we like
    // attach a nice smoke trail particle system to the rocket
    SetParticleSystem("rock|" + CParticleSystemRocket::GetConfigString(vec3_t(0.0f, 0.0f, 0.0f)));
}

CGameObjRocket::~CGameObjRocket(void)
{

}

void CGameObjRocket::OnHitWall(const vec3_t& location, const vec3_t& normal)
{
    DestroyRocket(location);
}

void CGameObjRocket::DestroyRocket(const vec3_t& location)
{
    SpawnParticleExplosion(location, 8.0f); // 8.0f = explosion size
    // the rocket particle system would disappear, if we delete
    // the rocket straight away (it's linked to the rocket).
    // remove us in 800 ms, give the particle system time to fade out
    // meanwhile the rocket is a ghost
    SetVel(vec3_t::origin); // no need to move anymore
    AddFlags(OBJ_FLAGS_GHOST); // this makes the rocket invisible, but not the particle system
    m_think.RemoveAll();
    m_think.AddFunc(new CThinkFuncRemoveMe(GetWorld()->GetLeveltime() + 800, GetWorld(), this));

    PlaySound(location,
              CLynx::GetBaseDirSound() + "rifle.ogg",
              800);
}

bool CThinkFuncRocket::DoThink(uint32_t leveltime)
{
    //CGameObjRocket* rocket = (CGameObjRocket*)GetObj();

    SetThinktime(leveltime + 250);
    return false;
}

