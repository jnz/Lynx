#include "ParticleSystemRocket.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CParticleSystemRocket::CParticleSystemRocket(const PROPERTYMAP& properties, CResourceManager* resman)
{
    vec3_t dir;

    const int particles = 1;
    m_particles.reserve(particles);

    m_texture = resman->GetTexture(CLynx::GetBaseDirFX() + "rock.tga");

    GetProperty(properties, "dx", &dir.x, 0);
    GetProperty(properties, "dy", &dir.y, 0);
    GetProperty(properties, "dz", &dir.z, -1.0f);

    dir = dir.Normalized() * 5.0f;

    for(int i=0;i<particles;i++)
    {
        particle_t p;

        p.startalpha = p.alpha = 0.5f;
        p.color = vec3_t(1,1,1);
        p.totallifetime = p.lifetime = 0.4f + CLynx::randfabs()*0.1f;
        p.origin = (float)(i) * dir + vec3_t::rand(0.03f,0.01f,0.03f);
        p.size = p.startsize = 3.00f;
        p.vel = dir + vec3_t::rand(1.03f, 1.03f, 1.03f);

        m_particles.push_back(p);
    }
}

CParticleSystemRocket::~CParticleSystemRocket(void)
{
}

std::string CParticleSystemRocket::GetConfigString(const vec3_t& dir)
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

