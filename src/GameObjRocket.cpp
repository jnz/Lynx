#include "GameObjRocket.h"

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
    m_think.AddFunc(new CThinkFuncRocket(GetWorld()->GetLeveltime() + 50, GetWorld(), this));
    // remove the rocket after 7 seconds:
    m_think.AddFunc(new CThinkFuncRemoveMe(GetWorld()->GetLeveltime() + 7000, GetWorld(), this));
    AddFlags(OBJ_FLAGS_NOGRAVITY); // feynman would be angry, but this is what we like
}

CGameObjRocket::~CGameObjRocket(void)
{

}

void CGameObjRocket::OnHitWall(const vec3_t location, const vec3_t normal)
{
    SpawnParticleExplosion(location, normal);
    GetWorld()->DelObj(this->GetID());
}

bool CThinkFuncRocket::DoThink(uint32_t leveltime)
{
    CGameObjRocket* rocket = (CGameObjRocket*)GetObj();
    rocket->SpawnParticleRocket(rocket->GetOrigin(), -rocket->GetVel());

    SetThinktime(leveltime + 250); // more particles in 250 ms
    return false;
}

