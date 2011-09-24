#include "ParticleSystemDust.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CParticleSystemDust::CParticleSystemDust(const PROPERTYMAP& properties, CResourceManager* resman)
{
    vec3_t dir;

    const int particles = 6;
    m_particles.reserve(particles);

    m_texture = resman->GetTexture(CLynx::GetBaseDirFX() + "dust.tga");

    GetProperty(properties, "dx", &dir.x, 0);
    GetProperty(properties, "dy", &dir.y, 0);
    GetProperty(properties, "dz", &dir.z, -1.0f);

    dir = dir.Normalized() * 12.0f;

    for(int i=0;i<particles;i++)
    {
        particle_t p;

        p.startalpha = p.alpha = 1.0f;
        p.color = vec3_t(1,1,1);
        p.totallifetime = p.lifetime = 0.3f + CLynx::randfabs()*0.3f;
        p.origin = vec3_t::rand(0.05f,0.05f,0.05f);
        p.size = p.startsize = 1.00f;
        p.vel = (dir + vec3_t::rand(1.5f, 5.5f, 1.5f)).Normalized() * 10.0f;

        m_particles.push_back(p);
    }
}

CParticleSystemDust::~CParticleSystemDust(void)
{
}

std::string CParticleSystemDust::GetConfigString(const vec3_t& dir)
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


