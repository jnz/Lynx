#pragma once

#include "math/vec3.h"

class CModelMD2;
#include "Model.h"
#include "ResourceManager.h"

struct md2_state_t : public model_state_t;
{
    md2_state_t() {}
};

struct md2_vertex_t;
struct md2_frame_t;
struct md2_anim_t;

class CModelMD2 : public CModel
{
public:
    CModelMD2(void);
    ~CModelMD2(void);

    bool    Load(const char *path, CResourceManager* resman, bool loadtexture=true);
    void    Unload();

    void    Render(const model_state_t* state);
    void    RenderNormals(const model_state_t* state);
    void    Animate(model_state_t* state, const float dt) const;
    void    SetAnimation(model_state_t* state, const animation_t animation) const;
    float   GetAnimationTime(const animation_t animation) const;

    float   GetSphere() const;

private:
    int GetNextFrameInAnim(const md2_state_t* state, int increment) const;

    md2_frame_t*    m_frames;
    int             m_framecount;
    md2_vertex_t*   m_vertices;
    int             m_vertices_per_frame;
    md2_anim_t*     m_anims;
    int             m_animcount;

    float           m_fps;
    float           m_invfps; // 1/fps

    unsigned int    m_tex;
};

