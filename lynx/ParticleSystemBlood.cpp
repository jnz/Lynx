#include "ParticleSystemBlood.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CParticleSystemBlood::CParticleSystemBlood(const PROPERTYMAP& properties, CResourceManager* resman)
{
    vec3_t dir;

    m_texture = resman->GetTexture(CLynx::GetBaseDirFX() + "blood.tga");

    GetProperty(properties, "dx", &dir.x, 0);
    GetProperty(properties, "dy", &dir.y, 0);
    GetProperty(properties, "dz", &dir.z, -1.0f);
    
    dir = dir.Normalized() * 6.0f;

    for(int i=0;i<12;i++)
    {
        particle_t p;
        
        p.startalpha = p.alpha = 0.9f;
        p.color = vec3_t(1,1,1);
        p.totallifetime = p.lifetime = 0.4f + CLynx::randfabs()*0.4f;
        p.origin = vec3_t::rand(0.4f,0.2f,0.4f) + dir*0.5f;
        p.size = p.startsize = 0.8f;
        p.vel = (dir + vec3_t::rand(1.5f, 1.5f, 1.5f)).Normalized() * 10.0f;

        m_particles.push_back(p);
    }
}

CParticleSystemBlood::~CParticleSystemBlood(void)
{
}

std::string CParticleSystemBlood::GetConfigString(const vec3_t& dir)
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


