#include <stdio.h>
#include "lynx.h"
#include "BSPLevel.h"
#include "SDL.h"
#include "GL/glew.h"
#define NO_SDL_GLEXT
#include "SDL_opengl.h"

#define BUFFER_OFFSET(i)    ((char *)NULL + (i)) // VBO Index Access

#define BSP_EPSILON         (0.003125f)

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CBSPLevel::CBSPLevel(void)
{
    m_plane = NULL;
    m_tex = NULL;
    m_texid = NULL;
    m_node = NULL;
    m_leaf = NULL;
    m_poly = NULL;
    m_vertex = NULL;
    m_indices = NULL;

    m_planecount = 0;
    m_texcount = 0;
    m_nodecount = 0;
    m_leafcount = 0;
    m_polycount = 0;
    m_vertexcount = 0;

    m_vbo = 0;
    m_vboindex = 0;
}

CBSPLevel::~CBSPLevel(void)
{
    Unload();
}

bool CBSPLevel::Load(std::string file, CResourceManager* resman)
{
    int i;
    FILE* f = fopen(file.c_str(), "rb");
    if(!f)
        return false;

    Unload();

    bspbin_header_t header;

    fread(&header, sizeof(header), 1, f);
    if(header.magic != BSPBIN_MAGIC)
    {
        fprintf(stderr, "BSP: Not a valid Lynx BSP file\n");
        return false;
    }
    if(header.version != BSPBIN_VERSION)
    {
        fprintf(stderr, "BSP: Wrong Lynx BSP file format version\n");
        return false;
    }

    bspbin_direntry_t dirplane;
    bspbin_direntry_t dirtextures;
    bspbin_direntry_t dirnodes;
    bspbin_direntry_t dirleafs;
    bspbin_direntry_t dirpoly;
    bspbin_direntry_t dirvertices;

    fread(&dirplane, sizeof(dirplane), 1, f);
    fread(&dirtextures, sizeof(dirtextures), 1, f);
    fread(&dirnodes, sizeof(dirnodes), 1, f);
    fread(&dirleafs, sizeof(dirleafs), 1, f);
    fread(&dirpoly, sizeof(dirpoly), 1, f);
    fread(&dirvertices, sizeof(dirvertices), 1, f);
    if(ftell(f) != BSPBIN_HEADER_LEN)
    {
        fprintf(stderr, "BSP: Error reading header\n");
        return false;
    }

    m_planecount = dirplane.length / sizeof(bspbin_plane_t);
    m_texcount = dirtextures.length / sizeof(bspbin_texture_t);
    m_nodecount = dirnodes.length / sizeof(bspbin_node_t);
    m_leafcount = dirleafs.length / sizeof(bspbin_leaf_t);
    m_polycount = dirpoly.length / sizeof(bspbin_poly_t);
    m_vertexcount = dirvertices.length / sizeof(bspbin_vertex_t);

    m_plane = new bspbin_plane_t[ m_planecount ];
    m_tex = new bspbin_texture_t[ m_texcount ];
    m_texid = new int[ m_texcount ];
    m_node = new bspbin_node_t[ m_nodecount ];
    m_leaf = new bspbin_leaf_t[ m_leafcount ];
    m_poly = new bspbin_poly_t[ m_polycount ];
    m_vertex = new bspbin_vertex_t[ m_vertexcount ];

    if(!m_plane || !m_tex || !m_texid || !m_node || !m_leaf || !m_poly || !m_vertex)
    {
        Unload();
        fprintf(stderr, "BSP: Not enough memory to load Lynx BSP\n");
        return false;
    }

    fseek(f, dirplane.offset, SEEK_SET);
    fread(m_plane, sizeof(bspbin_plane_t), m_planecount, f);

    fseek(f, dirtextures.offset, SEEK_SET);
    fread(m_tex, sizeof(bspbin_texture_t), m_texcount, f);

    fseek(f, dirnodes.offset, SEEK_SET);
    fread(m_node, sizeof(bspbin_node_t), m_nodecount, f);

    fseek(f, dirleafs.offset, SEEK_SET);
    fread(m_leaf, sizeof(bspbin_leaf_t), m_leafcount, f);

    fseek(f, dirpoly.offset, SEEK_SET);
    fread(m_poly, sizeof(bspbin_poly_t), m_polycount, f);

    fseek(f, dirvertices.offset, SEEK_SET);
    fread(m_vertex, sizeof(bspbin_vertex_t), m_vertexcount, f);

    if(ftell(f) != dirvertices.offset + dirvertices.length)
    {
        Unload();
        fprintf(stderr, "BSP: Error reading lumps from BSP file\n");
        return false;
    }

    // Prüfen, ob die index Zeiger in gültige Bereiche zeigen
    for(i=0;i<m_nodecount;i++)
    {
        for(int k=0;k<2;k++)
        {
            if(m_node[i].children[k] < 0) // leaf index
            {
                if(-m_node[i].children[k]-1 >= m_leafcount)
                {
                    fprintf(stderr, "BSP: Invalid leaf pointer\n");
                    return false;
                }
            }
            else
            {
                if(m_node[i].children[k] >= m_nodecount)
                {
                    fprintf(stderr, "BSP: Invalid node pointer\n");
                }
            }
        }
    }

    m_filename = file;

    // Rendering Stuff
    if(!resman)
    {
        m_vbo = 0;
        m_vboindex = 0;
        m_indices = NULL;
        return true;
    }

    // Setup Indices
    m_indices = new unsigned short[ m_vertexcount ];
    if(!m_indices)
    {
        Unload();
        fprintf(stderr, "BSP: Not enough memory for index buffer array\n");
        return false;
    }

    for(i=0;i<m_polycount;i++)
    {
        const int runto = m_poly[i].firstvertex+m_poly[i].vertexcount;
        const std::string texpath = CLynx::GetDirectory(file) + m_tex[m_poly[i].tex].name;
        m_texid[m_poly[i].tex] = resman->GetTexture(texpath); // könnte das direkt in m_poly[i].tex gehen?

        for(int j=m_poly[i].firstvertex;j<runto;j++)
        {
            m_indices[j] = j;
        }
    }

    // Setup OpenGL Vertex Buffer Objects

    glGenBuffers(1, &m_vbo);
    if(m_vbo < 1)
    {
        Unload();
        fprintf(stderr, "BSP: Failed to generate VBO\n");
        return false;
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    if(glGetError() != GL_NO_ERROR)
    {
        Unload();
        fprintf(stderr, "BSP: Failed to bind VBO\n");
        return false;
    }

    glBufferData(GL_ARRAY_BUFFER, sizeof(bspbin_vertex_t) * m_vertexcount, NULL, GL_STATIC_DRAW);
    if(glGetError() != GL_NO_ERROR)
    {
        Unload();
        fprintf(stderr, "BSP: Failed to buffer data\n");
        return false;
    }

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bspbin_vertex_t) * m_vertexcount, m_vertex);
    if(glGetError() != GL_NO_ERROR)
    {
        Unload();
        fprintf(stderr, "BSP: Failed to upload VBO data\n");
        return false;
    }

    glNormalPointer(GL_FLOAT, sizeof(bspbin_vertex_t), BUFFER_OFFSET(12));
    glTexCoordPointer(2, GL_FLOAT, sizeof(bspbin_vertex_t), BUFFER_OFFSET(24));
    glVertexPointer(3, GL_FLOAT, sizeof(bspbin_vertex_t), BUFFER_OFFSET(0));
    if(glGetError() != GL_NO_ERROR)
    {
        Unload();
        fprintf(stderr, "BSP: Failed to init VBO\n");
        return false;
    }

    glGenBuffers(1, &m_vboindex);
    if(m_vboindex < 1)
    {
        Unload();
        fprintf(stderr, "BSP: Failed to generate index VBO\n");
        return false;
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vboindex);
    if(glGetError() != GL_NO_ERROR)
    {
        Unload();
        fprintf(stderr, "BSP: Failed to bind index VBO\n");
        return false;
    }

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * m_vertexcount, NULL, GL_STATIC_DRAW);
    if(glGetError() != GL_NO_ERROR)
    {
        Unload();
        fprintf(stderr, "BSP: Failed to buffer index data\n");
        return false;
    }

    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(unsigned short) * m_vertexcount, m_indices);
    if(glGetError() != GL_NO_ERROR)
    {
        Unload();
        fprintf(stderr, "BSP: Failed to upload index VBO data\n");
        return false;
    }

    return true;
}

void CBSPLevel::Unload()
{
    SAFE_RELEASE_ARRAY(m_plane);
    SAFE_RELEASE_ARRAY(m_tex);
    SAFE_RELEASE_ARRAY(m_texid);
    SAFE_RELEASE_ARRAY(m_node);
    SAFE_RELEASE_ARRAY(m_leaf);
    SAFE_RELEASE_ARRAY(m_poly);
    SAFE_RELEASE_ARRAY(m_vertex);
    SAFE_RELEASE_ARRAY(m_indices);

    m_filename = "";

    m_planecount = 0;
    m_texcount = 0;
    m_nodecount = 0;
    m_leafcount = 0;
    m_polycount = 0;
    m_vertexcount = 0;

    if(m_vbo > 0)
        glDeleteBuffers(1, &m_vbo);
    if(m_vboindex > 0)
        glDeleteBuffers(1, &m_vboindex);

    m_vbo = 0;
    m_vboindex = 0;
}

spawn_point_t CBSPLevel::GetRandomSpawnPoint() const
{
    spawn_point_t p;
    p.origin = vec3_t(0, 10.0f, 0); // FIXME
    return p;
}

void CBSPLevel::RenderNodeGL(const int node, const vec3_t& origin, const CFrustum& frustum) const
{
	if(node < 0) // is leaf
	{
        const int leafindex = -node-1;
        const int polyindex = m_leaf[leafindex].firstpoly;
        const int maxpolyindex = polyindex + m_leaf[leafindex].polycount;

        for(int i=polyindex;i<maxpolyindex;i++)
        {
            glBindTexture(GL_TEXTURE_2D, m_texid[m_poly[i].tex]);
            
            glDrawElements(GL_TRIANGLES, 
                           m_poly[i].vertexcount, 
                           GL_UNSIGNED_SHORT, 
                           BUFFER_OFFSET(m_poly[i].firstvertex * 2));
        }

		return;
	}

	if(!frustum.TestSphere(m_node[node].sphere_origin, m_node[node].radius))
		return;

    const int planeindex = m_node[node].plane;
	switch(m_plane[planeindex].p.Classify(origin))
	{
	case POINTPLANE_FRONT:
	    RenderNodeGL(m_node[node].children[1], origin, frustum);
	    RenderNodeGL(m_node[node].children[0], origin, frustum);
		break;
	case POINTPLANE_BACK:
	case POINT_ON_PLANE:
	    RenderNodeGL(m_node[node].children[0], origin, frustum);
	    RenderNodeGL(m_node[node].children[1], origin, frustum);
		break;
	}
}

void CBSPLevel::RenderGL(const vec3_t& origin, const CFrustum& frustum) const
{
    if(!IsLoaded())
        return;

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vboindex);
     
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
     
    glNormalPointer(GL_FLOAT, sizeof(bspbin_vertex_t), BUFFER_OFFSET(12));
    glTexCoordPointer(2, GL_FLOAT, sizeof(bspbin_vertex_t), BUFFER_OFFSET(24));
    glVertexPointer(3, GL_FLOAT, sizeof(bspbin_vertex_t), BUFFER_OFFSET(0));

	// glDepthFunc(GL_ALWAYS); // We do this, because the tree is already sorted, but we need the z-buffer info for the models

    RenderNodeGL(0, origin, frustum);
    //glBindTexture(GL_TEXTURE_2D, m_texid[0]);
    //glDrawElements(GL_TRIANGLES, m_vertexcount, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));

    // glDepthFunc(GL_LEQUAL);

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
}

void CBSPLevel::TraceSphere(bsp_sphere_trace_t* trace) const
{
    if(m_node == NULL)
    {
        trace->f = MAX_TRACE_DIST;
        return;
    }
    TraceSphere(trace, 0);
}

bool CBSPLevel::GetTriIntersection(const int polyindex, 
                                   const vec3_t& start, 
                                   const vec3_t& dir, 
                                   float* f, 
                                   const float offset,
                                   plane_t* hitplane) const
{
    float cf;
    const int vertexindex = m_poly[polyindex].firstvertex;
    const vec3_t& P0 = m_vertex[vertexindex + 0].v;
    const vec3_t& P1 = m_vertex[vertexindex + 1].v;
    const vec3_t& P2 = m_vertex[vertexindex + 2].v;
    plane_t polyplane(P2, P1, P0); // Polygon Ebene
    polyplane.m_d -= offset; // Plane shift

    // Nur Dreiecke sind erlaubt
    assert(m_poly[polyindex].vertexcount == 3);

    bool hit = polyplane.GetIntersection(&cf, start, dir);
	if(!hit || cf > 1.0f || cf < -BSP_EPSILON)
		return false;
	vec3_t tmpintersect = start + dir*cf - polyplane.m_n*offset;
    *f = cf;

    // Berechnung über Barycentric coordinates (math for 3d game programming p. 144)
    const vec3_t R = tmpintersect - P0;
    const vec3_t Q1 = P1 - P0;
    const vec3_t Q2 = P2 - P0;
    const float Q1Q2 = Q1*Q2;
    const float Q1_sqr = Q1.AbsSquared();
    const float Q2_sqr = Q2.AbsSquared();
    const float invdet = 1/(Q1_sqr*Q2_sqr - Q1Q2*Q1Q2);
    const float RQ1 = R * Q1;
    const float RQ2 = R * Q2;
    const float w1 = invdet*(Q2_sqr*RQ1 - Q1Q2*RQ2);
    const float w2 = invdet*(-Q1Q2*RQ1  + Q1_sqr*RQ2);

    hit = w1 >= 0 && w2 >= 0 && (w1 + w2 <= 1);
    if(hit)
    {
        polyplane.m_d += offset;
        *hitplane = polyplane;
    }
    return hit;
}

bool CBSPLevel::GetEdgeIntersection(const int firstvertex,
                                    const int vertexcount,
                                    const vec3_t& start, 
                                    const vec3_t& dir,
                                    float* f, 
                                    const float radius, 
                                    vec3_t* normal,
                                    vec3_t* hitpoint) const
{
    float minf = MAX_TRACE_DIST;
    float cf;
    int minindex = -1;
	for(int i=0;i<vertexcount;i++)
	{
        if(!vec3_t::RayCylinderIntersect(start, 
                                         dir, 
                                         m_vertex[firstvertex + i].v, 
                                         m_vertex[firstvertex + ((i+1)%vertexcount)].v, 
                                         radius, 
                                         &cf))
            continue;
        if(cf < minf && cf >= -BSP_EPSILON)
        {
            minf = cf;
            minindex = i;
        }
    }
    if(minf <= 1.0f)
    {
        *hitpoint = start + minf*dir;
		const vec3_t* a = &m_vertex[firstvertex + minindex].v;
		const vec3_t* b = &m_vertex[firstvertex + ((minindex+1)%vertexcount)].v;

        *normal = ((*a-*hitpoint)^(*b-*hitpoint)) ^ (*b-*a);
        normal->Normalize();
        *f = minf;
        return true;
    }
    else
    {
        return false;
	}
}

bool CBSPLevel::GetVertexIntersection(const int firstvertex,
                                      const int vertexcount,
                                      const vec3_t& start, 
                                      const vec3_t& dir,
                                      float* f, 
                                      const float radius, 
                                      vec3_t* normal, 
                                      vec3_t* hitpoint) const
{
    float minf = MAX_TRACE_DIST;
    float cf;
    int minindex = -1;
    for(int i=0;i<vertexcount;i++)
    {
        if(!vec3_t::RaySphereIntersect(start, 
                                       dir,
                                       m_vertex[firstvertex + i].v, 
                                       radius, 
                                       &cf))
        {
            continue;
        }
        if(cf < minf && cf >= -BSP_EPSILON)
        {
            minindex = i;
            minf = cf;
        }
    }
    if(minf <= 1.0f)
    {
        *hitpoint = start + minf*dir;
        *normal = *hitpoint - m_vertex[firstvertex + minindex].v;
        normal->Normalize();
        *f = minf;
        return true;
    }
    else
    {
        return false;
    }
}

void CBSPLevel::TraceSphere(bsp_sphere_trace_t* trace, const int node) const
{
	if(node < 0) // Sind wir an einem Blatt angelangt?
	{
        const int leafindex = -node-1;
        const int polycount = m_leaf[leafindex].polycount;
		float cf;
		float minf = MAX_TRACE_DIST;
		int minindex = -1;
        plane_t hitplane;
        vec3_t hitpoint;
        vec3_t normal;
		for(int i=0;i<polycount;i++)
		{
            const int polyindex = m_leaf[leafindex].firstpoly + i;
            // - Prüfen ob Polygonfläche getroffen wird
            // - Prüfen ob Polygon Edge getroffen wird
            // - Prüfen ob Polygon Vertex getroffen wird
            if(GetTriIntersection(polyindex,
                                  trace->start, 
                                  trace->dir, 
                                  &cf, 
                                  trace->radius,
                                  &hitplane))
            {
				if(cf < minf)
				{
					minf = cf;
					minindex = i;
				}
            }
            else if(GetEdgeIntersection(m_poly[polyindex].firstvertex,
                                        m_poly[polyindex].vertexcount,
                                        trace->start,
                                        trace->dir,
                                        &cf, trace->radius, 
                                        &normal, &hitpoint) || 
                    GetVertexIntersection(m_poly[polyindex].firstvertex,
                                          m_poly[polyindex].vertexcount,
                                          trace->start,
                                          trace->dir,
                                          &cf, trace->radius, 
                                          &normal, &hitpoint))
            {
				if(cf < minf)
				{
					minf = cf;
					minindex = i;
                    hitplane.SetupPlane(hitpoint, normal);
				}
            }
		}
		trace->f = minf;
		if(minindex != -1)
		{
            trace->p = hitplane;
		}
		return;
	}

    pointplane_t locstart;
    pointplane_t locend;

    // Prüfen, ob alles vor der Splitplane liegt
    plane_t tmpplane = m_plane[m_node[node].plane].p;
    tmpplane.m_d -= trace->radius;
	locstart = tmpplane.Classify(trace->start, BSP_EPSILON);
	locend = tmpplane.Classify(trace->start + trace->dir, BSP_EPSILON);
	if(locstart > POINT_ON_PLANE && locend > POINT_ON_PLANE)
	{
        TraceSphere(trace, m_node[node].children[0]);
		return;
	}

    // Prüfen, ob alles hinter der Splitplane liegt
    tmpplane.m_d += 2*trace->radius;
	locstart = tmpplane.Classify(trace->start, BSP_EPSILON);
	locend = tmpplane.Classify(trace->start + trace->dir, BSP_EPSILON);
	if(locstart < POINT_ON_PLANE && locend < POINT_ON_PLANE)
	{
        TraceSphere(trace, m_node[node].children[1]);
		return;
	}

	bsp_sphere_trace_t trace1 = *trace;
    bsp_sphere_trace_t trace2 = *trace;

    TraceSphere(&trace1, m_node[node].children[0]);
    TraceSphere(&trace2, m_node[node].children[1]);

	if(trace1.f < trace2.f)
		*trace = trace1;
	else
		*trace = trace2;
}
