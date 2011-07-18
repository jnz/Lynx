#include "ParticleSystemExplosion.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CParticleSystemExplosion::CParticleSystemExplosion(const PROPERTYMAP& properties, CResourceManager* resman)
{
    vec3_t dir;

    const int particles = 1;
    m_particles.reserve(particles);

    m_texture = resman->GetTexture(CLynx::GetBaseDirFX() + "expl.tga");

    GetProperty(properties, "dx", &dir.x, 0);
    GetProperty(properties, "dy", &dir.y, 0);
    GetProperty(properties, "dz", &dir.z, -1.0f);

    dir = dir.Normalized() * 22.0f;

    for(int i=0;i<particles;i++)
    {
        particle_t p;

        p.startalpha = p.alpha = 0.6f;
        p.color = vec3_t(1,1,1);
        p.totallifetime = p.lifetime = 0.1f + CLynx::randfabs()*0.2f;
        p.origin = vec3_t::rand(0.05f,0.05f,0.05f);
        p.size = p.startsize = 8.00f;
        p.vel = vec3_t::origin;

        m_particles.push_back(p);
    }
}

CParticleSystemExplosion::~CParticleSystemExplosion(void)
{
}

std::string CParticleSystemExplosion::GetConfigString(const vec3_t& dir)
{
    std::string s = "";
    s += "dx=";
    s += CLynx::FloatToString(dir.x, 4);
    s += ",dy=";
    s += CLynx::FloatToString(dir.y, 4);
    s += ",dz=";
    s += CLynx::FloatToString(dir.z, 4);

    return s;
}

