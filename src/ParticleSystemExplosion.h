#pragma once
#include "ParticleSystem.h"

class CParticleSystemExplosion :
    public CParticleSystem
{
public:
    CParticleSystemExplosion(const PROPERTYMAP& properties, CResourceManager* resman);
    ~CParticleSystemExplosion(void);

    static std::string GetConfigString(const vec3_t& dir);
};
