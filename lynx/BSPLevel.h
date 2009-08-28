#pragma once

#include "BSPBIN.h"
#include "ResourceManager.h"
#include <string>
#include "Frustum.h"

#define MAX_TRACE_DIST      99999.999f

struct bsp_sphere_trace_t
{
	vec3_t	start; // start point
    vec3_t  dir; // end point = start + dir
    float   radius;
	float	f; // impact = start + f*dir
	plane_t	p; // impact plane
};

class CBSPLevel
{
public:
    CBSPLevel(void);
    ~CBSPLevel(void);

	bool		Load(std::string file, CResourceManager* resman);
	void		Unload();

    bool        IsLoaded() const { return m_vertex != NULL; }

    std::string GetFilename() const { return m_filename; }
    int         GetLeafCount() const { return 0; }

    spawn_point_t GetRandomSpawnPoint() const;

    void		TraceSphere(bsp_sphere_trace_t* trace) const;

    void        RenderGL(const vec3_t& origin, const CFrustum& frustum) const;

protected:

    void        RenderNodeGL(const int node, const vec3_t& origin, const CFrustum& frustum) const;

    void		TraceSphere(bsp_sphere_trace_t* trace, const int node) const;
    inline bool GetTriIntersection(const int polyindex, 
                                   const vec3_t& start, 
                                   const vec3_t& dir, 
                                   float* f, 
                                   const float offset, 
                                   plane_t* hitplane) const;
    inline bool GetEdgeIntersection(const int firstvertex, 
                                    const int vertexcount, 
                                    const vec3_t& start, 
                                    const vec3_t& dir, 
                                    float* f, 
                                    const float radius, 
                                    vec3_t* normal,
                                    vec3_t* hitpoint) const;
    inline bool GetVertexIntersection(const int firstvertex,
                                      const int vertexcount,
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
    bspbin_poly_t*      m_poly;
    bspbin_vertex_t*    m_vertex;

    unsigned short*     m_indices;

    int m_planecount;
    int m_texcount;
    int m_nodecount;
    int m_leafcount;
    int m_polycount;
    int m_vertexcount;

    std::string m_filename;

    unsigned int m_vbo;
    unsigned int m_vboindex;
};
