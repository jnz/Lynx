#pragma once
#include "ParticleSystem.h"

class CParticleSystemRocket :
    public CParticleSystem
{
public:
    CParticleSystemRocket(const PROPERTYMAP& properties, CResourceManager* resman, const vec3_t& ownerpos);
    ~CParticleSystemRocket(void);

    static std::string GetConfigString(const vec3_t& dir);

protected:
    void InitParticle(particle_t& p, const vec3_t& ownerpos); // init a single particle
    float GetGravity() { return 1.0f; }

private:
    vec3_t m_dir; // set by property map settings from the server
};
