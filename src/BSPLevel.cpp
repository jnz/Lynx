#include "lynx.h"
#include <stdio.h>
#include "BSPLevel.h"
#include <SDL/SDL.h>
#include <GL/glew.h>
#define NO_SDL_GLEXT
#include <SDL/SDL_opengl.h>
#include <memory>
#include "Renderer.h"

#define BSP_EPSILON         (0.1f)

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
    m_triangle = NULL;
    m_vertex = NULL;
    m_indices = NULL;
    m_spawnpoint = NULL;

    m_planecount = 0;
    m_texcount = 0;
    m_nodecount = 0;
    m_leafcount = 0;
    m_trianglecount = 0;
    m_vertexcount = 0;
    m_spawnpointcount = 0;

    m_vbo = 0;
    m_vboindex = 0;
}

CBSPLevel::~CBSPLevel(void)
{
    Unload();
}

bool CBSPLevel::Load(std::string file, CResourceManager* resman)
{
    static const vec3_t axis[3] = {vec3_t::xAxis,
                                   vec3_t::yAxis,
                                   vec3_t::zAxis};
    uint32_t i, j, k;
    int errcode;
    FILE* f = fopen(file.c_str(), "rb");
    if(!f)
        return false;

    fprintf(stderr, "Loading level: %s\n", file.c_str());

    Unload();
    // This code has three sections:
    // Reading the header
    // Reading the file allocation table
    // Reading the actual data

    // Reading header
    bspbin_header_t header;

    fread(&header, sizeof(header), 1, f);
    if(header.magic != BSPBIN_MAGIC)
    {
        fprintf(stderr, "BSP: Not a valid Lynx BSP file\n");
        return false;
    }
    if(header.version != BSPBIN_VERSION)
    {
        fprintf(stderr, "BSP: Wrong Lynx BSP file format version. Expecting: %i, got: %i\n", BSPBIN_VERSION, header.version);
        return false;
    }

    // Reading file allocation table
    bspbin_direntry_t dirplane;
    bspbin_direntry_t dirtextures;
    bspbin_direntry_t dirnodes;
    bspbin_direntry_t dirtriangles;
    bspbin_direntry_t dirvertices;
    bspbin_direntry_t dirspawnpoints;
    bspbin_direntry_t dirleafs;

    fread(&dirplane, sizeof(dirplane), 1, f);
    fread(&dirtextures, sizeof(dirtextures), 1, f);
    fread(&dirnodes, sizeof(dirnodes), 1, f);
    fread(&dirtriangles, sizeof(dirtriangles), 1, f);
    fread(&dirvertices, sizeof(dirvertices), 1, f);
    fread(&dirspawnpoints, sizeof(dirspawnpoints), 1, f);
    fread(&dirleafs, sizeof(dirleafs), 1, f);
    if(ftell(f) != BSPBIN_HEADER_LEN)
    {
        fprintf(stderr, "BSP: Error reading header\n");
        return false;
    }

    m_planecount = dirplane.length / sizeof(bspbin_plane_t);
    m_texcount = dirtextures.length / sizeof(bspbin_texture_t);
    m_nodecount = dirnodes.length / sizeof(bspbin_node_t);
    m_trianglecount = dirtriangles.length / sizeof(bspbin_triangle_t);
    m_vertexcount = dirvertices.length / sizeof(bspbin_vertex_t);
    m_spawnpointcount = dirspawnpoints.length / sizeof(bspbin_spawn_t);
    m_leafcount = dirleafs.length; // special case for the leafs

    m_plane = new plane_t[ m_planecount ];
    m_tex = new bspbin_texture_t[ m_texcount ];
    m_texid = new int[ m_texcount ];
    m_node = new bspbin_node_t[ m_nodecount ];
    m_triangle = new bspbin_triangle_t[ m_trianglecount ];
    m_vertex = new bspbin_vertex_t[ m_vertexcount ];
    m_spawnpoint = new bspbin_spawn_t[ m_spawnpointcount ];
    m_leaf = new bspbin_leaf_t[ m_leafcount ];

    if(!m_plane ||
       !m_tex ||
       !m_texid ||
       !m_node ||
       !m_leaf ||
       !m_triangle ||
       !m_vertex ||
       !m_spawnpoint)
    {
        Unload();
        fprintf(stderr, "BSP: Not enough memory to load Lynx BSP\n");
        return false;
    }

    // Reading data

    bspbin_plane_t kdplane;
    fseek(f, dirplane.offset, SEEK_SET);
    for(i=0; i<m_planecount; i++) // reconstruct the planes
    {
        fread(&kdplane, sizeof(kdplane), 1, f);
        if(kdplane.type > 2)
        {
            Unload();
            fprintf(stderr, "BSP: Unknown plane type\n");
            return false;
        }
        m_plane[i].m_d = kdplane.d;
        m_plane[i].m_n = axis[kdplane.type]; // lookup table
    }

    fseek(f, dirtextures.offset, SEEK_SET);
    fread(m_tex, sizeof(bspbin_texture_t), m_texcount, f);

    fseek(f, dirnodes.offset, SEEK_SET);
    fread(m_node, sizeof(bspbin_node_t), m_nodecount, f);

    fseek(f, dirtriangles.offset, SEEK_SET);
    fread(m_triangle, sizeof(bspbin_triangle_t), m_trianglecount, f);

    fseek(f, dirvertices.offset, SEEK_SET);
    fread(m_vertex, sizeof(bspbin_vertex_t), m_vertexcount, f);

    fseek(f, dirspawnpoints.offset, SEEK_SET);
    fread(m_spawnpoint, sizeof(bspbin_spawn_t), m_spawnpointcount, f);

    // Reading the triangle indices of the leafs
    fseek(f, dirleafs.offset, SEEK_SET);

    uint32_t trianglecount, triangleindex;
    for(i = 0; i < dirleafs.length; i++)
    {
        fread(&trianglecount, sizeof(trianglecount), 1, f);
        m_leaf[i].triangles.reserve(trianglecount);
        for(j=0; j<trianglecount; j++)
        {
            fread(&triangleindex, sizeof(triangleindex), 1, f);
            m_leaf[i].triangles.push_back(triangleindex);
        }
    }

    // integrity check, the last 4 bytes in a lbsp version >=5 file
    // is the magic pattern.
    uint32_t endmark;
    fread(&endmark, sizeof(endmark), 1, f);
    if(endmark != BSPBIN_MAGIC)
    {
        Unload();
        fprintf(stderr, "BSP: Error reading lumps from BSP file\n");
        return false;
    }

    // Check if tree file indices are within a valid range
    for(i=0;i<m_nodecount;i++)
    {
        for(k=0;k<2;k++)
        {
            if(m_node[i].children[k] < 0) // leaf index
            {
                if(-m_node[i].children[k]-1 >= (int)m_leafcount)
                {
                    fprintf(stderr, "BSP: Invalid leaf pointer\n");
                    Unload();
                    return false;
                }
            }
            else
            {
                if(m_node[i].children[k] >= (int)m_nodecount)
                {
                    fprintf(stderr, "BSP: Invalid node pointer\n");
                    Unload();
                    return false;
                }
            }
        }
    }

    m_filename = file;

    // Rendering stuff, if we are running as a server,
    // we can return now.
    if(!resman)
    {
        m_vbo = 0;
        m_vboindex = 0;
        return true;
    }

    // Setup indices
    const uint32_t indexcount = m_trianglecount*3;
    m_indices = new vertexindex_t[ indexcount ];
    if(!m_indices)
    {
        Unload();
        fprintf(stderr, "BSP: Not enough memory for index buffer array\n");
        return false;
    }

    // we use this normal map, if no texture_bump.jpg is available
    int texnormal_fallback = resman->GetTexture(CLynx::GetBaseDirTexture() + "normal.jpg");
    if(texnormal_fallback == 0)
    {
        fprintf(stderr, "Failed to load standard normal map");
        return false;
    }

    uint32_t vertexindex = 0;
    for(i=0;i<m_texcount;i++)
    {
        // loading all textures
        const std::string texpath = CLynx::GetDirectory(file) + m_tex[i].name;

        const std::string texpathnoext = CLynx::StripFileExtension(m_tex[i].name);
        const std::string texpathext = CLynx::GetFileExtension(m_tex[i].name);
        const std::string texpathnormal = CLynx::GetDirectory(file) + texpathnoext + "_bump" + texpathext;

        // we group all triangles with the same
        // texture in a complete batch of vertex indices
        bsp_texture_batch_t thisbatch;

        thisbatch.start = vertexindex;

        m_texid[i] = resman->GetTexture(texpath);
        thisbatch.texid = m_texid[i];
        thisbatch.texidnormal = resman->GetTexture(texpathnormal, true); // we only try to load the bump map
        if(thisbatch.texidnormal == 0) // no texture found, use fall back texture
            thisbatch.texidnormal = texnormal_fallback;

        for(j=0;j<m_trianglecount;j++)
        {
            if(m_triangle[j].tex == i) // this triangle is using the current texture
            {
                m_indices[vertexindex++] = m_triangle[j].v[0];
                m_indices[vertexindex++] = m_triangle[j].v[1];
                m_indices[vertexindex++] = m_triangle[j].v[2];
            }
        }
        thisbatch.count = vertexindex - thisbatch.start;
        m_texturebatch.push_back(thisbatch);
    }
    assert(indexcount == vertexindex);

    glGenBuffers(1, &m_vbo);
    if(m_vbo < 1)
    {
        Unload();
        fprintf(stderr, "BSP: Failed to generate VBO\n");
        return false;
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    errcode = glGetError();
    if(errcode != GL_NO_ERROR)
    {
        fprintf(stderr, "BSP: Failed to bind VBO: %i\n", errcode);
        assert(0);
        Unload();
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

    // the following depends on the bspbin_vertex_t struct
    // (byte offsets)
    glVertexPointer(3, GL_FLOAT, sizeof(bspbin_vertex_t), BUFFER_OFFSET(0));
    glNormalPointer(GL_FLOAT, sizeof(bspbin_vertex_t), BUFFER_OFFSET(12));
    glClientActiveTexture(GL_TEXTURE0);
    glTexCoordPointer(2, GL_FLOAT, sizeof(bspbin_vertex_t), BUFFER_OFFSET(24));
    glClientActiveTexture(GL_TEXTURE1);
    glTexCoordPointer(4, GL_FLOAT, sizeof(bspbin_vertex_t), BUFFER_OFFSET(32));

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

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(vertexindex_t) * indexcount, NULL, GL_STATIC_DRAW);
    if(glGetError() != GL_NO_ERROR)
    {
        Unload();
        fprintf(stderr, "BSP: Failed to buffer index data\n");
        return false;
    }

    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(vertexindex_t) * indexcount, m_indices);
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
    SAFE_RELEASE_ARRAY(m_triangle);
    SAFE_RELEASE_ARRAY(m_vertex);
    SAFE_RELEASE_ARRAY(m_indices);
    SAFE_RELEASE_ARRAY(m_spawnpoint);

    m_texturebatch.clear();

    m_filename = "";

    m_planecount = 0;
    m_texcount = 0;
    m_nodecount = 0;
    m_leafcount = 0;
    m_trianglecount = 0;
    m_vertexcount = 0;
    m_spawnpointcount = 0;

    if(m_vbo > 0)
        glDeleteBuffers(1, &m_vbo);
    if(m_vboindex > 0)
        glDeleteBuffers(1, &m_vboindex);

    m_vbo = 0;
    m_vboindex = 0;
}

static const bspbin_spawn_t s_spawn_default;
bspbin_spawn_t CBSPLevel::GetRandomSpawnPoint() const
{
    if(m_spawnpointcount < 1)
        return s_spawn_default;
    return m_spawnpoint[rand()%m_spawnpointcount];
}

void CBSPLevel::RenderGL(const vec3_t& origin, const CFrustum& frustum) const
{
    if(!IsLoaded())
        return;

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vboindex);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(bspbin_vertex_t), BUFFER_OFFSET(0));

    glEnableClientState(GL_NORMAL_ARRAY);
    glNormalPointer(GL_FLOAT, sizeof(bspbin_vertex_t), BUFFER_OFFSET(12));

    glClientActiveTexture(GL_TEXTURE0);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, sizeof(bspbin_vertex_t), BUFFER_OFFSET(24));

    glClientActiveTexture(GL_TEXTURE1);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(4, GL_FLOAT, sizeof(bspbin_vertex_t), BUFFER_OFFSET(32));

    const uint32_t batchcount = m_texturebatch.size();
    for(uint32_t i=0; i<batchcount; i++)
    {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_texturebatch[i].texidnormal);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texturebatch[i].texid);

        glDrawElements(GL_TRIANGLES,
                       m_texturebatch[i].count,
                       MY_GL_VERTEXINDEX_TYPE,
                       BUFFER_OFFSET(m_texturebatch[i].start * sizeof(vertexindex_t)));
    }

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);

    glClientActiveTexture(GL_TEXTURE1);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_TEXTURE_2D);

    glActiveTexture(GL_TEXTURE0);
    glClientActiveTexture(GL_TEXTURE0);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void CBSPLevel::RenderNormals() const
{
    unsigned int vindex;
    const vec3_t tanoff(0.06f); // offset for tangents
    const vec3_t bitanoff(-0.06f); // offset for tangents
    const float nscale = 0.3f;
    vec3_t bitangent;

    glBegin(GL_LINES);
    for(vindex = 0; vindex < m_vertexcount; vindex++)
    {
        glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
        glVertex3fv(m_vertex[vindex].v.v);
        glVertex3fv((nscale*m_vertex[vindex].n + m_vertex[vindex].v).v);

        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3fv((tanoff + m_vertex[vindex].v).v);
        glVertex3fv((nscale*m_vertex[vindex].t + m_vertex[vindex].v + tanoff).v);

        glColor3f(0.0f, 0.0f, 1.0f);
        bitangent = m_vertex[vindex].w * (m_vertex[vindex].n ^ m_vertex[vindex].t);
        glVertex3fv((bitanoff + m_vertex[vindex].v).v);
        glVertex3fv((nscale*bitangent + m_vertex[vindex].v + bitanoff).v);
    }
    glEnd();
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

bool CBSPLevel::GetTriIntersection(const int triangleindex,
                                   const vec3_t& start,
                                   const vec3_t& dir,
                                   float* f,
                                   const float offset,
                                   plane_t* hitplane,
                                   bool& needs_edge_test) const
{
    float cf;
    const int vertexindex1 = m_triangle[triangleindex].v[0];
    const int vertexindex2 = m_triangle[triangleindex].v[1];
    const int vertexindex3 = m_triangle[triangleindex].v[2];
    const vec3_t& P0 = m_vertex[vertexindex1].v;
    const vec3_t& P1 = m_vertex[vertexindex2].v;
    const vec3_t& P2 = m_vertex[vertexindex3].v;
    plane_t polyplane(P0, P1, P2); // Polygon Ebene  -  why don't we precalculate the triangle normal vector?
    float dsave = polyplane.m_d;
    polyplane.m_d -= offset; // Plane shift

    bool hit = polyplane.GetIntersection(&cf, start, dir);
    if(!hit || cf > 1.0f || cf < 0.0f)
    {
        needs_edge_test = !(cf > 1.0f);
        return false;
    }
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
        polyplane.m_d = dsave;
        *hitplane = polyplane;
    }
    needs_edge_test = !hit;

    return hit;
}

bool CBSPLevel::GetEdgeIntersection(const int triangleindex,
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
    for(int i=0;i<3;i++) // for every edge of triangle
    {
        const vec3_t fromvec = m_vertex[m_triangle[triangleindex].v[i]].v;
        const vec3_t tovec = m_vertex[m_triangle[triangleindex].v[(i+1)%3]].v;
        if(!vec3_t::RayCylinderIntersect(start,
                                         dir,
                                         fromvec,
                                         tovec,
                                         radius,
                                         &cf)) continue;
        if(cf < minf && cf >= 0.0f)
        {
            minf = cf;
            minindex = i;
        }
    }
    if(minf <= 1.0f && minindex >= 0)
    {
        *hitpoint = start + minf*dir;
        const vec3_t* a = &m_vertex[m_triangle[triangleindex].v[minindex]].v;
        const vec3_t* b = &m_vertex[m_triangle[triangleindex].v[(minindex+1)%3]].v;

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

bool CBSPLevel::GetVertexIntersection(const int triangleindex,
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
    for(int i=0;i<3;i++) // for every vertex of triangle
    {
        const vec3_t v = m_vertex[m_triangle[triangleindex].v[i]].v;
        if(!vec3_t::RaySphereIntersect(start,
                                       dir,
                                       v,
                                       radius,
                                       &cf))
        {
            continue;
        }
        if(cf < minf && cf >= 0.0f)
        {
            minindex = i;
            minf = cf;
        }
    }
    if(minf <= 1.0f && minindex >= 0)
    {
        *hitpoint = start + minf*dir;
        *normal = *hitpoint - m_vertex[m_triangle[triangleindex].v[minindex]].v;
        normal->Normalize();
        *f = minf;
        return true;
    }
    else
    {
        return false;
    }
}

#define	DIST_EPSILON	(0.06f)
void CBSPLevel::TraceSphere(bsp_sphere_trace_t* trace) const
{
    if(m_node == NULL)
    {
        fprintf(stderr, "Warning: Tracing in unloaded level\n");
        trace->f = MAX_TRACE_DIST;
        return;
    }
    trace->f = MAX_TRACE_DIST;
    TraceSphere(trace, 0);
    assert(trace->f == 0.0f || trace->f > 0.01f);
}

void CBSPLevel::TraceSphere(bsp_sphere_trace_t* trace, const int node) const
{
    if(node < 0) // have we reached a leaf?
    {
        int triangleindex;
        const int leafindex = -node-1;
        const unsigned int trianglecount = m_leaf[leafindex].triangles.size();
        float cf;
        float minf = trace->f;
        int minindex = -1;
        plane_t hitplane;
        vec3_t hitpoint;
        vec3_t normal;
        bool needs_edge_test;
        for(unsigned int i=0;i<trianglecount;i++)
        {
            triangleindex = m_leaf[leafindex].triangles[i];
            // - Check for triangle inner area
            // - Check for all three triangle edges
            // - Check for all three triangles vertices
            if(GetTriIntersection(triangleindex,
                                  trace->start,
                                  trace->dir,
                                  &cf,
                                  trace->radius,
                                  &hitplane,
                                  needs_edge_test))
            {
                if(cf < minf)
                {
                    minf = cf;
                    minindex = i;
                }
            }
            if(!needs_edge_test)
                continue;
            if(GetEdgeIntersection(triangleindex,
                                   trace->start,
                                   trace->dir,
                                   &cf, trace->radius,
                                   &normal, &hitpoint) ||
                GetVertexIntersection(triangleindex,
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
        if(minindex != -1)
        {
            // safety shift along the trace path.
            // this keeps the object DIST_EPSILON away from
            // the plane along the plane normal.
            float df = DIST_EPSILON/(hitplane.m_n * trace->dir);
            if(df > 0.0f)
                df = 0.0f;
            if(minf + df <= DIST_EPSILON)
                trace->f = 0.0f;
            else
                trace->f = minf + df;
            trace->p = hitplane;
        }
        return;
    }

    pointplane_t locstart;
    pointplane_t locend;

    // Prüfen, ob alles vor der Splitplane liegt
    plane_t tmpplane = m_plane[m_node[node].plane];
    tmpplane.m_d -= trace->radius;
    locstart = tmpplane.Classify(trace->start, BSP_EPSILON);
    locend = tmpplane.Classify(trace->start + trace->dir, BSP_EPSILON);
    if(locstart > POINT_ON_PLANE && locend > POINT_ON_PLANE)
    {
        TraceSphere(trace, m_node[node].children[0]);
        return;
    }

    // Prüfen, ob alles hinter der Splitplane liegt
    tmpplane.m_d = m_plane[m_node[node].plane].m_d + trace->radius;
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

