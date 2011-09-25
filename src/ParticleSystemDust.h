#pragma once
#include "ParticleSystem.h"

class CParticleSystemDust :
    public CParticleSystem
{
public:
    CParticleSystemDust(const PROPERTYMAP& properties, CResourceManager* resman, const vec3_t& ownerpos);
    ~CParticleSystemDust(void);

    static std::string GetConfigString(const vec3_t& dir);
protected:
    void InitParticle(particle_t& p, const vec3_t& ownerpos); // init a single particle

private:
    vec3_t m_dir; // set by property map settings from the server
};
