#pragma once
#include "ParticleSystem.h"

class CParticleSystemBlood :
    public CParticleSystem
{
public:
    CParticleSystemBlood(const PROPERTYMAP& properties, CResourceManager* resman);
    ~CParticleSystemBlood(void);

    //virtual void Update(const float dt, const uint32_t ticks);

    static std::string GetConfigString(const vec3_t& dir);

protected:
};
