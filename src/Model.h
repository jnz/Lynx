#pragma once

#include "math/vec3.h"
class CModel;
#include "ResourceManager.h"

typedef int16_t animation_t;         // animation id
enum
{
    // if you change the order here, change the
    // animation registry in Model.cpp too.
    // if you forget, an assertion error will remind you.
    ANIMATION_NONE = 0,
    ANIMATION_IDLE,
    ANIMATION_RUN,
    ANIMATION_ATTACK,
    ANIMATION_FIRE,
    ANIMATION_COUNT // make this the last item
};

struct model_state_t
{
    model_state_t()
    {
        curr_frame = 0;
        next_frame = 1;
        animation = ANIMATION_NONE;
        time = 0.0f;
        play_count = 0;
    }
    virtual ~model_state_t(){}
    // abstract base struct

    int curr_frame;
    int next_frame;
    animation_t animation; // animation id
    float time; // from 0.0 s to num_frames/framerate
    int play_count; // how many times is the current animation played
};

class CModel
{
public:
    CModel();
    virtual ~CModel();

    virtual bool    Load(const char *path, CResourceManager* resman, bool loadtexture=true) = 0;
    virtual void    Unload() = 0;

    virtual void    Render(const model_state_t* state) = 0; // FIXME make this const complete
    virtual void    RenderNormals(const model_state_t* state) = 0; // FIXME make this const complete
    virtual void    Animate(model_state_t* state, const float dt) const = 0;
    virtual void    SetAnimation(model_state_t* state, const animation_t animation) const = 0;
    virtual float   GetAnimationTime(const animation_t animation) const = 0;

    virtual float   GetSphere() const = 0;

    static animation_t GetAnimationFromString(const std::string& animation_name);
    static std::string GetStringFromAnimation(const animation_t animation);
    static animation_t GetNextAnimation(const animation_t animation);
};

