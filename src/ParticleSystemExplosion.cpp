#include "ParticleSystemExplosion.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CParticleSystemExplosion::CParticleSystemExplosion(const PROPERTYMAP& properties,
                                                   CResourceManager* resman,
                                                   const vec3_t& ownerpos)
{
    const int particles = 1;
    m_particles.reserve(particles);

    m_texture = resman->GetTexture(CLynx::GetBaseDirFX() + "expl.tga");
    GetProperty(properties, "size", &m_size, 8.0f);

    for(int i=0;i<particles;i++)
    {
        particle_t p;
        InitParticle(p, ownerpos);
        m_particles.push_back(p);
    }
}

void CParticleSystemExplosion::InitParticle(particle_t& p, const vec3_t& ownerpos)
{
    p.startalpha = p.alpha = 0.6f;
    p.color = vec3_t(1,1,1);
    p.totallifetime = p.lifetime = 0.1f + CLynx::randfabs()*0.2f;
    p.origin = vec3_t::rand(0.05f,0.05f,0.05f) + ownerpos;
    p.size = p.startsize = m_size;
    p.vel = vec3_t::origin;
}

CParticleSystemExplosion::~CParticleSystemExplosion(void)
{

}

std::string CParticleSystemExplosion::GetConfigString(const float size)
{
    std::string s = "";
    s += "size=";
    s += CLynx::FloatToString(size, 3);
    return s;
}

