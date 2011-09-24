#pragma once
#include "ParticleSystem.h"

class CParticleSystemRocket :
    public CParticleSystem
{
public:
    CParticleSystemRocket(const PROPERTYMAP& properties, CResourceManager* resman);
    ~CParticleSystemRocket(void);

    static std::string GetConfigString(const vec3_t& dir);
};
