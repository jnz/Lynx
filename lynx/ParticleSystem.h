#pragma once

#include "lynx.h"
#include "math/vec3.h"
#include <string>
#include <map>
#include <vector>
#include "ResourceManager.h"

struct particle_t
{
    vec3_t  origin;
    vec3_t  vel;
    float   size;
    float   startsize;
    vec3_t  color;
    float   alpha;
    float   startalpha;
    float   lifetime;
    float   totallifetime;
};

typedef std::map<std::string, float> PROPERTYMAP;

class CParticleSystem
{
public:
    virtual ~CParticleSystem(void);

    void Render(const vec3_t& side, const vec3_t& up, const vec3_t& dir);
	virtual void Update(const float dt, const DWORD ticks);

    static CParticleSystem* CreateSystem(const std::string systemname,
                                         const PROPERTYMAP& properties,
                                         CResourceManager* resman);
    static void SetPropertyMap(const std::string config, PROPERTYMAP& properties); // Erzeugt eine PROPERTYMAP (für CreateSystem) aus einem Config String. Config String Bsp: "dx=0.1,dy=0.3,dz=0.5";

protected:
    CParticleSystem(void);

    bool GetProperty(const PROPERTYMAP& properties, std::string key,
                     float* f, const float default_value);

    std::vector<particle_t> m_particles;
    int m_texture;
};
