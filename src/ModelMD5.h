#pragma once

#include "math/vec3.h"
#include "math/quaternion.h"
#include "BSPBIN.h"
#include <map>

class CModelMD5;
#include "ResourceManager.h"

/* This code is based upon the great MD5 documentation
 * and source code by David Henry */

/* Joint */
struct md5_joint_t
{
    char name[64];
    int parent;

    vec3_t pos;
    quaternion_t orient;
};

/* Vertex */
struct md5_vertex_t
{
    float u, v;

    int start; /* start weight */
    int count; /* weight count */
};

/* Triangle */
struct md5_triangle_t
{
    int index[3];
};

/* Weight */
struct md5_weight_t
{
    int joint;
    float bias;

    vec3_t pos;
};

/* Bounding box */
struct md5_bbox_t
{
    vec3_t min;
    vec3_t max;
};

/* MD5 mesh */
struct md5_mesh_t
{
    md5_vertex_t *vertices;
    md5_triangle_t *triangles;
    md5_weight_t *weights;

    int num_verts;
    int num_tris;
    int num_weights;

    char shader[256];
};

/* Animation data */
struct md5_anim_t
{
    int num_frames;
    int num_joints;
    int frameRate;

    std::vector< std::vector<md5_joint_t> > skelFrames;
    std::vector<md5_bbox_t> bboxes;
};

/* md5 animation state */
struct md5_state_t
{
    int curr_frame;
    int next_frame;
    std::string animation; // string "run", "walk" etc.
    md5_anim_t* animdata;
    float time; // from 0.0 s to num_frames/framerate

    std::vector<md5_joint_t> skel;
};

class CModelMD5
{
public:
    CModelMD5(void);
    ~CModelMD5(void);

    bool    Load(char *path, CResourceManager* resman, bool loadtexture=true);
    void    Unload();

    void    Render(const md5_state_t* state);
    void    Animate(md5_state_t* state, const float dt) const;
    void    SetAnimation(md5_state_t* state, const std::string animation);

    float   GetSphere() const { return 1.0f; }; // FIXME

protected:
    void    RenderSkeleton(const md5_joint_t* skel) const;

    void    PrepareMesh(const md5_mesh_t *mesh, const std::vector<md5_joint_t>& skeleton);

    bool    AllocVertexBuffer();
    void    DeallocVertexBuffer();
    bool    UploadVertexBuffer(unsigned int vertexcount, unsigned int indexcount) const;

    bool    ReadAnimation(const std::string animation, const std::string filepath); // animation = "run", "idle", "walk" etc.
    md5_anim_t* GetAnimation(const std::string animation, bool createnew); // Get md5_anim_t associated with animation string or create a new one

private:
    md5_joint_t* m_baseSkel;
    md5_mesh_t*  m_meshes;
    int          m_num_joints;
    int          m_num_meshes;

    // data for vertex buffer objects
    int          m_max_verts;
    int          m_max_tris;
    unsigned int m_vbo;
    unsigned int m_vboindex;

    bspbin_vertex_t*  m_vertex_buffer;
    vertexindex_t*    m_vertex_index_buffer;

    int          m_tex; // texture

    // Animations
    std::map<std::string, md5_anim_t*> m_animations;
};

