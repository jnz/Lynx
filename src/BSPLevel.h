#pragma once

#include "BSPBIN.h"
#include "ResourceManager.h"
#include <string>
#include <vector>
#include "Frustum.h"

#define MAX_TRACE_DIST      99999.999f

struct bsp_sphere_trace_t
{
    vec3_t  start; // start point
    vec3_t  dir; // end point = start + dir
    float   radius;
    float   f; // impact = start + f*dir
    plane_t p; // impact plane
};

struct bsp_texture_batch_t
{
    uint16_t start; // vertex index start
    uint16_t count; // vertex index count
    int texid;
};

class CBSPLevel
{
public:
    CBSPLevel(void);
    ~CBSPLevel(void);

    bool        Load(std::string file, CResourceManager* resman);
    void        Unload();

    bool        IsLoaded() const { return m_vertex != NULL; }

    std::string GetFilename() const { return m_filename; }
    int         GetLeafCount() const { return 0; }

    bspbin_spawn_t GetRandomSpawnPoint() const;

    void        TraceSphere(bsp_sphere_trace_t* trace) const;

    void        RenderGL(const vec3_t& origin, const CFrustum& frustum) const;

protected:

    void        TraceSphere(bsp_sphere_trace_t* trace, const int node) const;
    inline bool GetTriIntersection(const int triangleindex, 
                                   const vec3_t& start, 
                                   const vec3_t& dir, 
                                   float* f, 
                                   const float offset, 
                                   plane_t* hitplane) const;
    inline bool GetEdgeIntersection(const int triangleindex, 
                                    const vec3_t& start, 
                                    const vec3_t& dir, 
                                    float* f, 
                                    const float radius, 
                                    vec3_t* normal,
                                    vec3_t* hitpoint) const;
    inline bool GetVertexIntersection(const int triangleindex,
                                      const vec3_t& start, 
                                      const vec3_t& dir,
                                      float* f, 
                                      const float radius, 
                                      vec3_t* normal, 
                                      vec3_t* hitpoint) const;

    // Data
    bspbin_plane_t*     m_plane;
    bspbin_texture_t*   m_tex;
    int*                m_texid;
    bspbin_node_t*      m_node;
    bspbin_leaf_t*      m_leaf;
    bspbin_triangle_t*  m_triangle;
    bspbin_vertex_t*    m_vertex;
    bspbin_spawn_t*     m_spawnpoint;

    uint16_t*           m_indices;
    std::vector<bsp_texture_batch_t> m_texturebatch;

    int m_planecount;
    int m_texcount;
    int m_nodecount;
    int m_leafcount;
    int m_trianglecount;
    int m_vertexcount;
    int m_spawnpointcount;

    std::string m_filename;

    unsigned int m_vbo;
    unsigned int m_vboindex;
};
