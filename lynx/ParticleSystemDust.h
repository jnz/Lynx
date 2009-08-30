#pragma once
#include "particlesystem.h"

class CParticleSystemDust :
    public CParticleSystem
{
public:
    CParticleSystemDust(const PROPERTYMAP& properties, CResourceManager* resman);
    ~CParticleSystemDust(void);

    static std::string GetConfigString(const vec3_t& dir);

protected:
};
