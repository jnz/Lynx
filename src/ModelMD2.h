#pragma once

#include "math/vec3.h"

class CModelMD2;
#include "Model.h"
#include "ResourceManager.h"
#include <vector>

struct md2_state_t : public model_state_t
{
    md2_state_t() {
        model_state_t();
        md2anim = 0;
    }
    int md2anim;
};

struct md2_vertex_t;
struct md2_frame_t;
struct md2_anim_t;
struct md2_texcoord_t;
struct md2_triangle_t;

// md2_vbo_start_end:
// index of current vertices in vbo
// the vbo contains all vertices, for all frames.
// this struct keeps track of the vertex indices for
// the current frame.
struct md2_vbo_start_end_t
{
    unsigned int start;
    unsigned int end;
};

class CModelMD2 : public CModel
{
public:
    CModelMD2(void);
    ~CModelMD2(void);

    bool    Load(const char *path, CResourceManager* resman, bool loadtexture=true);
    void    Unload();

    void    Render(const model_state_t* state);
    void    RenderNormals(const model_state_t* state) {}; // not implemented
    void    Animate(model_state_t* state, const float dt) const;
    void    SetAnimation(model_state_t* state, const animation_t animation) const;
    float   GetAnimationTime(const animation_t animation) const;

    float   GetSphere() const { return 2.0f; }

private:
    int     GetNextFrameInAnim(const md2_state_t* state, int increment) const;

    void    RenderFixed(const model_state_t* state) const;

    md2_frame_t*    m_frames;
    unsigned int    m_framecount;
    md2_vertex_t*   m_vertices;
    unsigned int    m_vertices_per_frame;
    md2_anim_t*     m_anims;
    unsigned int    m_animcount;
    md2_texcoord_t* m_texcoords;
    unsigned int    m_texcoordscount;
    md2_triangle_t* m_triangles;
    unsigned int    m_trianglecount;

    unsigned int    m_animmap[ANIMATION_COUNT]; // map lynx animation id (ANIMATION_IDLE...) to md2 m_anim index

    float           m_fps;
    float           m_invfps; // 1/fps

    unsigned int    m_tex; // diffuse texture
    unsigned int    m_normalmap; // tangent space normal mapping

    bool            m_shaderactive; // use shader or fixed pipeline

    // VBO stuff
    bool            AllocVertexBuffer();
    void            DeallocVertexBuffer();

    unsigned int    m_vbo_vertex;
    unsigned int    m_vboindex;

    // with the m_vbo_frame_table you can do the following
    // vboindexstart = m_vbo_frame_table[cur_frame].start;
    // vboindexend = m_vbo_frame_table[cur_frame].end;
    std::vector<md2_vbo_start_end_t> m_vbo_frame_table;

    // Shader stuff
    bool            InitShader();
    static int      m_program;
    static int      m_interp; // interp uniform for md2 vertex shader

    // Rule of three
    CModelMD2(const CModelMD2&);
    CModelMD2& operator=(const CModelMD2&);
};

