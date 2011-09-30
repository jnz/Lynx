#include "ParticleSystemRocket.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CParticleSystemRocket::CParticleSystemRocket(const PROPERTYMAP& properties, CResourceManager* resman, const vec3_t& ownerpos)
{
    const int particles = 32;
    m_particles.reserve(particles);

    m_texture = resman->GetTexture(CLynx::GetBaseDirFX() + "rock.tga");

    GetProperty(properties, "dx", &m_dir.x, 0);
    GetProperty(properties, "dy", &m_dir.y, 0);
    GetProperty(properties, "dz", &m_dir.z, -1.0f);

    if(m_dir.AbsSquared() > 0.1f) // dir might be zero
        m_dir = m_dir.Normalized() * 5.0f;

    for(int i=0;i<particles;i++)
    {
        particle_t p;
        InitParticle(p, ownerpos);
        // to create a trail, the lifetime is calculated in a way, that
        // the respawn in the right way. if you change the lifetime here,
        // change the lifetime in InitParticle too (for visual reasons)
        p.totallifetime = p.lifetime = (float)i/(float)particles * 1.5f + 0.1f; // create trail
        m_particles.push_back(p);
    }
}

void CParticleSystemRocket::InitParticle(particle_t& p, const vec3_t& ownerpos)
{
    p.startalpha = p.alpha = 0.1f + CLynx::randfabs()*0.4f;
    p.color = vec3_t(1,1,1);
    // when the rocket hits a wall, it waits a while as ghost
    // to give the particle system time to fade out. if you change
    // the lifetime here, for visual reasons you should also change
    // the CGameObjRocket::DestroyRocket function too.
    p.totallifetime = p.lifetime = 0.4f + CLynx::randfabs()*1.6f;
    p.origin = vec3_t::rand(0.3f,0.1f,0.3f) + ownerpos;
    p.size = p.startsize = 0.9f + CLynx::randfabs() * 4.0f;
    p.vel = m_dir + vec3_t::rand(0.5f,0.5f,0.5f);
    p.respawn = true;
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

