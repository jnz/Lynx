#pragma once

#include "BSPBIN.h"
#include "ResourceManager.h"
#include <string>
#include <vector>
#include "Frustum.h"

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
    uint32_t start; // vertex index start
    uint32_t count; // vertex index count
    int texid;
    int texidnormal; // normal map
};

// what data type should we use for the
// VBO index buffer? for less than 0xffff vertices
// uint16_t is sufficient
typedef uint32_t vertexindex_t; // if you change this, change MY_GL_VERTEXINDEX_TYPE too
#define MY_GL_VERTEXINDEX_TYPE  GL_UNSIGNED_INT
// #define MY_GL_VERTEXINDEX_TYPE  GL_UNSIGNED_SHORT // for uint16_t vertexindex_t

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
    void        RenderNormals() const;

protected:

    void        TraceSphere(bsp_sphere_trace_t* trace, const int node) const;
    inline bool GetTriIntersection(const int triangleindex,
                                   const vec3_t& start,
                                   const vec3_t& dir,
                                   float* f,
                                   const float offset,
                                   plane_t* hitplane,
                                   bool& needs_edge_test) const;

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
    plane_t*            m_plane;
    bspbin_texture_t*   m_tex;
    int*                m_texid;
    bspbin_node_t*      m_node;
    bspbin_triangle_t*  m_triangle;
    bspbin_vertex_t*    m_vertex;
    bspbin_spawn_t*     m_spawnpoint;
    bspbin_leaf_t*      m_leaf;

    vertexindex_t*      m_indices;
    std::vector<bsp_texture_batch_t> m_texturebatch;

    uint32_t m_planecount;
    uint32_t m_texcount;
    uint32_t m_nodecount;
    uint32_t m_leafcount;
    uint32_t m_trianglecount;
    uint32_t m_vertexcount;
    uint32_t m_spawnpointcount;

    std::string m_filename;

    unsigned int m_vbo;
    unsigned int m_vboindex;
};
