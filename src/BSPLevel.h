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

class CBSPLevel
{
public:
    CBSPLevel(void);
    ~CBSPLevel(void);

    bool        Load(std::string file, CResourceManager* resman);
    void        Unload();

    bool        IsLoaded() const { return (m_trianglecount > 0); }

    std::string GetFilename() const { return m_filename; }
    int         GetLeafCount() const { return 0; }

    bspbin_spawn_t GetRandomSpawnPoint() const;

    void        TraceSphere(bsp_sphere_trace_t* trace) const;
    bool        IsSphereStuck(const vec3_t& position, const float radius) const;

    void        RenderGL(const vec3_t& origin, const CFrustum& frustum) const;
    void        RenderNormals() const;

protected:

    void        TraceSphere(bsp_sphere_trace_t* trace, const int node) const;
    bool        IsSphereStuck(const vec3_t& position, const float radius, const int node) const;

    // Static check
    bool        SphereTriangleIntersectStatic(const int triangleindex,
                                              const vec3_t& sphere_pos,
                                              const float sphere_radius) const; // used by IsSphereStuck

    // Sweeping check
    inline bool SphereTriangleIntersect(const int triangleindex,
                                        const vec3_t& sphere_pos,
                                        const float sphere_radius,
                                        const vec3_t& dir,
                                        float* f,
                                        vec3_t* hitnormal) const;


    bool                m_uselightmap;
    int                 m_lightmap; // lightmap texture id
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

private:
    // Rule of three
    CBSPLevel(const CBSPLevel&);
    CBSPLevel& operator=(const CBSPLevel&);
};
