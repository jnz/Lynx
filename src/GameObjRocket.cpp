#include "GameObjRocket.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CGameObjRocket::CGameObjRocket(CWorld* world) : CGameObj(world)
{
    SetResource(CLynx::GetBaseDirModel() + "pknight/tris.md2");
    SetAnimation(0);
    SetRadius(0.1f);
    // rocket think function:
    m_think.AddFunc(new CThinkFuncRocket(GetWorld()->GetLeveltime() + 50, GetWorld(), this));
    // remove the rocket after 5 seconds:
    m_think.AddFunc(new CThinkFuncRemoveMe(GetWorld()->GetLeveltime() + 5000, GetWorld(), this));
    AddFlags(OBJ_FLAGS_NOGRAVITY);
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
    rocket->SpawnParticleDust(rocket->GetOrigin(), rocket->GetVel());

    SetThinktime(leveltime + 250); // set next think
    return false;
}
