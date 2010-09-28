#include "ParticleSystem.h"
#include "ParticleSystemBlood.h"
#include "ParticleSystemDust.h"
#include "GL/glew.h"
#define NO_SDL_GLEXT
#include <SDL/SDL_opengl.h>
#include <sstream>
#include "math/mathconst.h"
#include <math.h>

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CParticleSystem::CParticleSystem(void)
{
    m_texture = 0;
}

CParticleSystem::~CParticleSystem(void)
{
}

void CParticleSystem::Render(const vec3_t& side, const vec3_t& up, const vec3_t& dir)
{
    glBindTexture(GL_TEXTURE_2D, m_texture);

    std::vector<particle_t>::const_iterator iter;
    for(iter = m_particles.begin();iter != m_particles.end(); iter++)
    {
        if(iter->lifetime < 0.0f)
            continue;
        
        const vec3_t X = iter->size*0.5f*side;
        const vec3_t Y = iter->size*0.5f*up;
        const vec3_t Q1 =  X + Y + iter->origin;
        const vec3_t Q2 = -X + Y + iter->origin;
        const vec3_t Q3 = -X - Y + iter->origin;
        const vec3_t Q4 =  X - Y + iter->origin;

        glBegin(GL_QUADS);

        glColor4f(iter->color.x, iter->color.y, iter->color.z, iter->alpha);

        glTexCoord2f(1.0f, 1.0f);
        glVertex3fv(Q1.v);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3fv(Q2.v);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3fv(Q3.v);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3fv(Q4.v);

        glEnd();
    }
}

void CParticleSystem::Update(const float dt, const DWORD ticks)
{
    const float gravity = 25.5f;

    std::vector<particle_t>::iterator iter;
    for(iter = m_particles.begin();iter != m_particles.end(); iter++)
    {
        if(iter->lifetime < 0.0f)
            continue;

        iter->origin = iter->origin + iter->vel*dt - vec3_t(0,dt*dt*0.5f*gravity,0);
        iter->vel.y -= gravity*dt;
        iter->lifetime -= dt;
        const float f = iter->lifetime / iter->totallifetime;
        const float scale = sin(f * lynxmath::PI*0.5f);
        iter->alpha = iter->startalpha * scale;
        iter->size = scale * iter->startsize + 0.1f;
    }
}

CParticleSystem* CParticleSystem::CreateSystem(std::string systemname,
                                               const PROPERTYMAP& properties,
                                               CResourceManager* resman)
{
    if(systemname == "blood")
    {
        return new CParticleSystemBlood(properties, resman);
    }
    else if(systemname == "dust")
    {
        return new CParticleSystemDust(properties, resman);
    }
    assert(0);
    return NULL;
}

bool CParticleSystem::GetProperty(const PROPERTYMAP& properties, std::string key, float* f, const float default_value)
{
    PROPERTYMAP::const_iterator iter = properties.find(key);
    if(iter == properties.end())
    {
        *f = default_value;
        return false;
    }
    *f = iter->second;
    return true;
}

void CParticleSystem::SetPropertyMap(const std::string config, PROPERTYMAP& properties)
{
    if(config.size() < 1)
        return;

    std::istringstream iss(config);
    std::string token;
    while(getline(iss, token, ','))
    {
        std::istringstream iss2(token);
        std::string key;
        std::string value;
        if(getline(iss2, key, '=') && getline(iss2, value, '='))
        {
            std::stringstream floatconv(value);
            float f;
            if(floatconv >> f) 
            {
                properties[key] = f;
            }
            else
            {
                assert(0);
            }
        }
        else
        {
            assert(0);
        }
    }
}
