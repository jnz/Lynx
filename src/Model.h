#pragma once

#include "math/vec3.h"
class CModel;
#include "ResourceManager.h"

struct model_state_t
{
    model_state_t()
    {
        curr_frame = 0;
        next_frame = 1;
        animation = ANIMATION_NONE;
        time = 0.0f;
    }
    virtual ~model_state_t(){}
    // abstract base struct

    int curr_frame;
    int next_frame;
    animation_t animation; // animation id
    float time; // from 0.0 s to num_frames/framerate
};

class CModel
{
public:
    CModel();
    virtual ~CModel();

    virtual bool    Load(const char *path, CResourceManager* resman, bool loadtexture=true) = 0;
    virtual void    Unload() = 0;

    virtual void    Render(const model_state_t* state) = 0;
    virtual void    RenderNormals(const model_state_t* state) = 0;
    virtual void    Animate(model_state_t* state, const float dt) const = 0;
    virtual void    SetAnimation(model_state_t* state, const animation_t animation) = 0;

    virtual float   GetSphere() const = 0;
};

