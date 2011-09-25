#pragma once

#include "lynx.h"
#include "math/vec3.h"
#include <string>
#include <map>
#include <vector>
#include "ResourceManager.h"

/*
 * The basic principle is, that the server creates a particlesystem
 * by simply defining a string and set this string to the
 * object.state.particle property. (see CGameObj::SpawnParticle* functions)
 *
 * The network system takes care of distributing this property to all
 * clients. If a client sees, that the object.state.particle string
 * has changed, it will create a particle system object
 * by calling CreateSystem and SetPropertyMap.
 * The renderer will take this particlesystem and render it each frame on
 * the client.
 */

struct particle_t
{
    particle_t()
    {
        size = 1.0f;
        startsize = 1.0f;
        alpha = 1.0f;
        startalpha = 1.0f;
        lifetime = 1.0f;
        totallifetime = 2.0f;
        respawn = false;
    }

    vec3_t  origin;
    vec3_t  vel;
    float   size;
    float   startsize;
    vec3_t  color;
    float   alpha;
    float   startalpha;
    float   lifetime;
    float   totallifetime;
    bool    respawn;
};

typedef std::map<std::string, float> PROPERTYMAP;

class CParticleSystem
{
public:
    virtual ~CParticleSystem(void);

    void Render(const vec3_t& side, const vec3_t& up, const vec3_t& dir);
    virtual void Update(const float dt, const uint32_t ticks, const vec3_t& ownerpos);

    static CParticleSystem* CreateSystem(const std::string systemname,
                                         const PROPERTYMAP& properties,
                                         CResourceManager* resman,
                                         const vec3_t& ownerpos);
    static void SetPropertyMap(const std::string config, PROPERTYMAP& properties); // creates a PROPERTYMAP (to be used by CreateSystem) from a config string. config string example: "dx=0.1,dy=0.3,dz=0.5";

protected:
    CParticleSystem(void);

    virtual void InitParticle(particle_t& p, const vec3_t& ownerpos) = 0; // init a single particle
    virtual float GetGravity() { return 25.0f; }

    bool GetProperty(const PROPERTYMAP& properties, std::string key,
                     float* f, const float default_value);

    std::vector<particle_t> m_particles;
    int m_texture;
};

