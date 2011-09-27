#include "GameObj.h"
#include <math.h> // atan2
#include "ParticleSystemBlood.h"
#include "ParticleSystemDust.h"
#include "ParticleSystemExplosion.h"
#include "ParticleSystemRocket.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CGameObj::CGameObj(CWorld* world) : CObj(world)
{
    m_health = 100;
}

CGameObj::~CGameObj(void)
{
}

void CGameObj::DealDamage(int damage,
                          const vec3_t& hitpoint,
                          const vec3_t& dir,
                          CGameObj* dealer,
                          bool& killed_me)
{
    killed_me = (m_health > 0) && (m_health - damage <= 0);
    AddHealth(-damage);
}

quaternion_t CGameObj::TurnTo(const vec3_t& location) const
{
    const float dx = location.x - GetOrigin().x;
    const float dz = location.z - GetOrigin().z;
    const float riwi = atan2(dx, dz);
    return quaternion_t(vec3_t::yAxis, riwi+lynxmath::PI);
}

void CGameObj::SpawnParticleBlood(const vec3_t& location, const vec3_t& dir, const float size)
{
    static const uint32_t BLOOD_LIFETIME = 1000; // ms

    CGameObj* blood = new CGameObj(GetWorld());
    blood->SetRadius(0.0f);
    blood->SetOrigin(location);
    blood->SetParticleSystem("blood|" + CParticleSystemBlood::GetConfigString(dir, size));
    blood->m_think.AddFunc(new CThinkFuncRemoveMe(GetWorld()->GetLeveltime() + BLOOD_LIFETIME, GetWorld(), blood));
    blood->AddFlags(OBJ_FLAGS_NOGRAVITY);
    GetWorld()->AddObj(blood);
}

void CGameObj::SpawnParticleDust(const vec3_t& location, const vec3_t& dir)
{
    static const uint32_t DUST_LIFETIME = 600; // ms

    CGameObj* dust = new CGameObj(GetWorld());
    dust->SetRadius(0.0f);
    dust->SetOrigin(location);
    dust->SetParticleSystem("dust|" + CParticleSystemDust::GetConfigString(dir));
    dust->m_think.AddFunc(new CThinkFuncRemoveMe(GetWorld()->GetLeveltime() + DUST_LIFETIME, GetWorld(), dust));
    dust->AddFlags(OBJ_FLAGS_NOGRAVITY);
    GetWorld()->AddObj(dust);
}

void CGameObj::SpawnParticleRocket(const vec3_t& location, const vec3_t& dir)
{
    static const uint32_t ROCKETTRAIL_LIFETIME = 500; // ms

    CGameObj* rockettrail = new CGameObj(GetWorld());
    rockettrail->SetRadius(0.0f);
    rockettrail->SetOrigin(location);
    rockettrail->SetParticleSystem("rock|" + CParticleSystemRocket::GetConfigString(dir));
    rockettrail->m_think.AddFunc(new CThinkFuncRemoveMe(GetWorld()->GetLeveltime() + ROCKETTRAIL_LIFETIME, GetWorld(), rockettrail));
    rockettrail->AddFlags(OBJ_FLAGS_NOGRAVITY);
    GetWorld()->AddObj(rockettrail);
}

void CGameObj::SpawnParticleExplosion(const vec3_t& location, const float size)
{
    static const uint32_t EXPL_LIFETIME = 800; // ms

    CGameObj* explosion = new CGameObj(GetWorld());
    explosion->SetRadius(0.0f);
    explosion->SetOrigin(location);
    explosion->SetParticleSystem("expl|" + CParticleSystemExplosion::GetConfigString(size));
    explosion->m_think.AddFunc(new CThinkFuncRemoveMe(GetWorld()->GetLeveltime() + EXPL_LIFETIME, GetWorld(), explosion));
    explosion->AddFlags(OBJ_FLAGS_NOGRAVITY);
    GetWorld()->AddObj(explosion);
}

int CGameObj::CreateSoundObj(const vec3_t& location, const std::string& soundpath, uint32_t lifetime) // FIXME is lifetime useful?
{
    CGameObj* sound = new CGameObj(GetWorld());
    sound->SetRadius(0.0f);
    sound->SetOrigin(location);
    sound->m_think.AddFunc(new CThinkFuncRemoveMe(GetWorld()->GetLeveltime() + lifetime, GetWorld(), sound));
    sound->AddFlags(OBJ_FLAGS_GHOST | OBJ_FLAGS_NOGRAVITY);
    sound->SetResource(soundpath);
    GetWorld()->AddObj(sound, true);
    return sound->GetID();
}

