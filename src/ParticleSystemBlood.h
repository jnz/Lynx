#pragma once
#include "ParticleSystem.h"

class CParticleSystemBlood :
    public CParticleSystem
{
public:
    CParticleSystemBlood(const PROPERTYMAP& properties, CResourceManager* resman, const vec3_t& ownerpos);
    ~CParticleSystemBlood(void);

    //virtual void Update(const float dt, const uint32_t ticks);

    static std::string GetConfigString(const vec3_t& dir, const float size);
protected:
    void InitParticle(particle_t& p, const vec3_t& ownerpos);

private:
    vec3_t m_dir;
    float m_size;
};
