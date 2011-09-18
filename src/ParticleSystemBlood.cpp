#include "ParticleSystemBlood.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CParticleSystemBlood::CParticleSystemBlood(const PROPERTYMAP& properties, CResourceManager* resman)
{
    vec3_t dir;
    float size;

    const int particles = 8;
    m_particles.reserve(particles);

    m_texture = resman->GetTexture(CLynx::GetBaseDirFX() + "blood.tga");

    GetProperty(properties, "dx", &dir.x, 0);
    GetProperty(properties, "dy", &dir.y, 0);
    GetProperty(properties, "dz", &dir.z, -1.0f);
    GetProperty(properties, "size", &size, 4.8f);

    dir = dir.Normalized() * 6.0f;

    for(int i=0;i<particles;i++)
    {
        particle_t p;

        p.startalpha = p.alpha = 0.2f + 0.4f*CLynx::randfabs();
        p.color = vec3_t(1,1,1);
        p.totallifetime = p.lifetime = 0.4f + CLynx::randfabs()*0.4f;
        p.origin = vec3_t::rand(0.8f,0.4f,0.8f) + dir*0.5f;
        p.size = p.startsize = size;
        p.vel = (dir + vec3_t::rand(1.5f, 1.5f, 1.5f)).Normalized() * 10.0f;

        m_particles.push_back(p);
    }
}

CParticleSystemBlood::~CParticleSystemBlood(void)
{
}

std::string CParticleSystemBlood::GetConfigString(const vec3_t& dir, const float size)
{
    std::string s = "";
    s += "dx=";
    s += CLynx::FloatToString(dir.x, 3);
    s += ",dy=";
    s += CLynx::FloatToString(dir.y, 3);
    s += ",dz=";
    s += CLynx::FloatToString(dir.z, 3);
    s += ",size=";
    s += CLynx::FloatToString(size, 2);

    return s;
}


