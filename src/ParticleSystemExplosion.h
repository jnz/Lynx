#pragma once
#include "ParticleSystem.h"

class CParticleSystemExplosion :
    public CParticleSystem
{
public:
    CParticleSystemExplosion(const PROPERTYMAP& properties, CResourceManager* resman, const vec3_t& ownerpos);
    ~CParticleSystemExplosion(void);

    static std::string GetConfigString(const float size);
protected:
    void InitParticle(particle_t& p, const vec3_t& ownerpos); // init a single particle

private:
    float m_size;
};
