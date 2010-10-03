#include "GameObj.h"
#include <math.h> // atan2
#include "ParticleSystemBlood.h"
#include "ParticleSystemDust.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif


CGameObj::~CGameObj(void)
{
}

quaternion_t CGameObj::TurnTo(const vec3_t& location) const
{
    const float dx = location.x - GetOrigin().x;
    const float dz = location.z - GetOrigin().z;
    const float riwi = atan2(dx, dz);
    return quaternion_t(vec3_t::yAxis, riwi+lynxmath::PI);
}

void CGameObj::SpawnParticleBlood(const vec3_t& location, const vec3_t& dir)
{
    CGameObj* blood = new CGameObj(GetWorld());
    blood->SetRadius(0.0f);
    blood->SetOrigin(location);
    blood->SetParticleSystem("blood|" + CParticleSystemBlood::GetConfigString(dir));
    blood->m_think.AddFunc(new CThinkFuncRemoveMe(GetWorld()->GetLeveltime() + 1000, GetWorld(), blood));
    blood->AddFlags(OBJ_FLAGS_NOGRAVITY);
    GetWorld()->AddObj(blood);
}

void CGameObj::SpawnParticleDust(const vec3_t& location, const vec3_t& dir)
{
    CGameObj* dust = new CGameObj(GetWorld());
    dust->SetRadius(0.0f);
    dust->SetOrigin(location);
    dust->SetParticleSystem("dust|" + CParticleSystemDust::GetConfigString(dir));
    dust->m_think.AddFunc(new CThinkFuncRemoveMe(GetWorld()->GetLeveltime() + 600, GetWorld(), dust));
    dust->AddFlags(OBJ_FLAGS_NOGRAVITY);
    GetWorld()->AddObj(dust);
}

void CGameObj::PlaySound(const vec3_t& location, const std::string& soundpath, uint32_t lifetime) // FIXME is lifetime useful?
{
    CGameObj* sound = new CGameObj(GetWorld());
    sound->SetRadius(0.0f);
    sound->SetOrigin(location);
    sound->m_think.AddFunc(new CThinkFuncRemoveMe(GetWorld()->GetLeveltime() + lifetime, GetWorld(), sound));
    sound->AddFlags(OBJ_FLAGS_NOGRAVITY);
    sound->SetResource(soundpath);
    GetWorld()->AddObj(sound, true);
}

