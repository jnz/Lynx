#include "lynx.h"
#include "math/vec3.h"
#include "ModelMD2.h"
#include <stdio.h>
#include <memory.h>
#include <GL/glew.h>
#define NO_SDL_GLEXT
#include <SDL/SDL_opengl.h>
#include <list>

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#pragma warning(push)
#pragma warning(disable: 4305)
#define NUMVERTEXNORMALS 162
typedef float vec3array[3];
static const vec3array g_bytedirs[NUMVERTEXNORMALS] = // quake 2 normal lookup table
{
    #include "anorms.h"
};
#pragma warning(pop)

#define MD2_FPS         6             // frames per second
#define MAX_SKINNAME    64            // max. characters for skin names
#define MD2_LYNX_SCALE  (0.07f)

#define BUFFER_OFFSET(i)    ((char *)NULL + (i)) // VBO Index Access

#pragma pack(push, 1)

// every struct here, that is byte aligned, is "on-disk" layout

struct md2_file_header_t // (from quake2 src)
{
    int32_t         ident;
    int32_t         version;

    int32_t         skinwidth;
    int32_t         skinheight;
    int32_t         framesize;      // byte size of each frame

    int32_t         num_skins;
    int32_t         num_xyz;
    int32_t         num_st;         // greater than num_xyz for seams
    int32_t         num_tris;
    int32_t         num_glcmds;     // dwords in strip/fan command list
    int32_t         num_frames;

    int32_t         ofs_skins;      // each skin is a MAX_SKINNAME string
    int32_t         ofs_st;         // byte offset from start for stverts
    int32_t         ofs_tris;       // offset for dtriangles
    int32_t         ofs_frames;     // offset for first frame
    int32_t         ofs_glcmds;
    int32_t         ofs_end;        // end of file
};

// was md2vertex_t
struct md2_file_vertex_t // on disk layout
{
    uint8_t v[3];      // compressed vertex
    uint8_t n;         // normalindex
};

// was md2frame_t
struct md2_file_frame_t // on disk layout
{
    vec3_t            scale;
    vec3_t            translate;
    char              name[16];
    md2_file_vertex_t v[1];
};

struct md2_file_texcoord_t
{
    int16_t s; // u = (float)s / header.skinwidth
    int16_t t; // v = 1.0f - (float)t / header.skinheight
};

struct md2_file_triangle_t
{
    int16_t index_xyz[3]; // indexes to triangle's vertices
    int16_t index_st[3];  // indexes to vertices' texture coorinates
};

#pragma pack(pop)

// md2_vertex_t is the vertex format used for VBO
struct md2_vertex_t
{
    vec3_t v;      // position
    vec3_t n;      // normal vector
    float tu, tv;  // texcoord u, v
    vec3_t t;      // tangent vector
    float w;       // direction of bitangent (-1 or 1): bitangent = cross(n, t)*w
    vec3_t vdir;   // lerp direction of vertex position in the next frame: vnext = v + vdir
};

struct md2_texcoord_t
{
    float u, v;
};

struct md2_triangle_t
{
    unsigned int v[3];  // vertex indices
    unsigned int uv[3]; // uv indices
};

struct md2_frame_t
{
    char          name[16]; // frame name
    int32_t       num_xyz;  // number of vertices
    md2_vertex_t* vertices; // pointer to vertices for this frame
};

struct md2_anim_t
{
    char          name[16]; // animation sequence name
    int32_t       start;    // frame number of first frame in this animation
    int32_t       end;      // last frame in this animation
};

CModelMD2::CModelMD2(void)
{
    m_frames = NULL;
    m_framecount = 0;
    m_vertices = NULL;
    m_vertices_per_frame = 0;
    m_anims = NULL;
    m_animcount = 0;
    m_texcoords = NULL;
    m_texcoordscount = 0;
    m_triangles = NULL;
    m_trianglecount = 0;

    m_fps = (float)MD2_FPS;
    m_invfps = 1.0f/m_fps;

    m_tex = 0;
    m_normalmap = 0;

    m_shaderactive = CLynx::cfg.GetVarAsInt("useshader", 1) ? true : false;

    // VBO
    m_vbo_vertex = 0;
    m_vboindex = 0;
}

CModelMD2::~CModelMD2(void)
{
    Unload();
}

void CModelMD2::Render(const model_state_t* mstate)
{
    if(!m_shaderactive)
    {
        RenderFixed(mstate);
        return;
    }
    md2_state_t* state = (md2_state_t*)mstate;

    int curprg;
    glGetIntegerv(GL_CURRENT_PROGRAM, &curprg);
    glUseProgram(m_program);

        const float interp = state->time * m_fps;
        glUniform1f(m_interp, interp); // set MD2 interpolation factor for shader 0..1

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertex);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vboindex);

        glEnableClientState(GL_VERTEX_ARRAY); // vertex
        glVertexPointer(3, GL_FLOAT, sizeof(md2_vertex_t), BUFFER_OFFSET(0));

        glEnableClientState(GL_NORMAL_ARRAY); // normal vector
        glNormalPointer(GL_FLOAT, sizeof(md2_vertex_t), BUFFER_OFFSET(12));

        glClientActiveTexture(GL_TEXTURE0); // texture coordinates u, v
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, sizeof(md2_vertex_t), BUFFER_OFFSET(24));

        glClientActiveTexture(GL_TEXTURE1); // tangent and bitangent
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(4, GL_FLOAT, sizeof(md2_vertex_t), BUFFER_OFFSET(32));

        glClientActiveTexture(GL_TEXTURE2); // vertex dir for lerping
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(3, GL_FLOAT, sizeof(md2_vertex_t), BUFFER_OFFSET(48));

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_normalmap);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_tex);

        const unsigned int indexstart = m_vbo_frame_table[state->curr_frame].start;
        const unsigned int indexend = m_vbo_frame_table[state->curr_frame].end;

        glDrawElements(GL_TRIANGLES,
                       indexend-indexstart+1,
                       GL_UNSIGNED_INT,
                       BUFFER_OFFSET(indexstart * sizeof(uint32_t)));

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);

        glClientActiveTexture(GL_TEXTURE1);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisable(GL_TEXTURE_2D);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, 0);

        glClientActiveTexture(GL_TEXTURE2);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisable(GL_TEXTURE_2D);

        glActiveTexture(GL_TEXTURE0);
        glClientActiveTexture(GL_TEXTURE0);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);

        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glUseProgram(curprg);
}

// draw the model with the OpenGL fixed function.
// this is straightforward but slow code to render triangles
// if no shader support is available.
// the MD2 format has support for triangle_strips and
// is faster than this, but currently we ignore this mode (glcmds)
void CModelMD2::RenderFixed(const model_state_t* state) const
{
    md2_vertex_t* cur_vertices  = m_frames[state->curr_frame].vertices;
    md2_vertex_t* next_vertices = m_frames[state->next_frame].vertices;
    md2_vertex_t* cur_vertex;
    md2_vertex_t* next_vertex;
    vec3_t inter_xyz, inter_n; // interpolated

    unsigned int i, j;
    int uvindex;
    int vindex;

    int curprg;

    if(m_shaderactive)
    {
        glGetIntegerv(GL_CURRENT_PROGRAM, &curprg);
        glUseProgram(0);
    }

    // FIXME activate normal mapping for MD2 too
    //glActiveTexture(GL_TEXTURE1);
    //glBindTexture(GL_TEXTURE_2D, m_normalmap);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_tex);

    glBegin(GL_TRIANGLES);

    for(i=0;i<m_trianglecount;i++)
    {
        for(j=0;j<3;j++)
        {
            vindex = m_triangles[i].v[j];
            uvindex = m_triangles[i].uv[j];

            glTexCoord2f(m_texcoords[uvindex].u,
                         m_texcoords[uvindex].v);

            cur_vertex = &cur_vertices[vindex];
            next_vertex = &next_vertices[vindex];

            inter_n = cur_vertex->n +
                (next_vertex->n - cur_vertex->n) *
                state->time * m_fps;
            glNormal3fv(inter_n.GetPointer());

            inter_xyz = cur_vertex->v +
                (next_vertex->v - cur_vertex->v) *
                state->time * m_fps;
            glVertex3fv(inter_xyz.GetPointer());

        }
    }

    glEnd();

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);

    if(m_shaderactive)
        glUseProgram(curprg);
}

bool CModelMD2::Load(const char *path, CResourceManager* resman, bool loadtexture)
{
    FILE* f;
    int i;
    uint8_t* frame = NULL;
    md2_file_frame_t* framehead;
    md2_file_header_t header;
    md2_file_texcoord_t* texcoords = NULL;
    md2_file_triangle_t* triangles = NULL;
    md2_vertex_t* vertex;
    std::list<md2_anim_t>::iterator iter;
    std::list<md2_anim_t> animlist;
    md2_anim_t curanim;
    char* tmpname;
    char skinname[MAX_SKINNAME];

    Unload();

    if(loadtexture)
    {
        if(m_shaderactive)
        {
            if(!InitShader())
                return false;
        }

        m_tex = resman->GetTexture(CLynx::ChangeFileExtension(path, "jpg"));
        m_normalmap = resman->GetTexture(CLynx::GetBaseDirTexture() + "normal.jpg");
    }

    f = fopen(path, "rb");
    if(!f)
    {
        fprintf(stderr, "MD2: Failed to open file: %s\n", path);
        goto loaderr;
    }
    if(fread(&header, 1, sizeof(header), f) != sizeof(header))
    {
        fprintf(stderr, "MD2: Header invalid\n");
        goto loaderr;
    }

    if(header.ident != 844121161 || header.version != 8)
    {
        fprintf(stderr, "MD2: Unknown format\n");
        goto loaderr;
    }

    // skins
    fseek(f, header.ofs_skins, SEEK_SET);
    for(i=0;i<header.num_skins;i++)
    {
        if(fread(skinname, 1, MAX_SKINNAME, f) != MAX_SKINNAME)
        {
            fprintf(stderr, "MD2: Skinname length invalid\n");
            goto loaderr;
        }
    }

    // vertices
    m_vertices = new md2_vertex_t[header.num_frames * header.num_xyz];
    if(!m_vertices)
    {
        fprintf(stderr, "MD2: Out of memory for vertices: %i\n", header.num_frames * header.num_xyz);
        goto loaderr;
    }
    m_vertices_per_frame = header.num_xyz;

    // frames
    frame = new uint8_t[header.framesize];
    if(!frame)
    {
        fprintf(stderr, "MD2: Out of memory for frame buffer: %i\n", header.framesize);
        goto loaderr;
    }
    framehead = (md2_file_frame_t*)frame;
    m_frames = new md2_frame_t[header.num_frames];
    if(!m_frames)
    {
        fprintf(stderr, "MD2: Out of memory for frames: %i\n", header.num_frames);
        goto loaderr;
    }
    m_framecount = header.num_frames;

    // texcoords
    texcoords = new md2_file_texcoord_t[header.num_st]; // tmp buffer
    if(texcoords == NULL)
    {
        fprintf(stderr, "MD2: Out of memory for st coordinates: %i\n", header.num_st);
        goto loaderr;
    }
    fseek(f, header.ofs_st, SEEK_SET);
    if(fread(texcoords, sizeof(md2_file_texcoord_t), header.num_st, f) !=
             (size_t)header.num_st)
    {
        fprintf(stderr, "MD2: Failed to read st coordinates: %i\n", header.num_st);
        goto loaderr;
    }
    m_texcoords = new md2_texcoord_t[header.num_st];
    m_texcoordscount = header.num_st;
    if(!m_texcoords)
    {
        fprintf(stderr, "MD2: Out of memory\n");
        goto loaderr;
    }
    for(i=0;i<header.num_st;i++)
    {
        m_texcoords[i].u = (float)texcoords[i].s / (float)header.skinwidth;
        m_texcoords[i].v = 1.0f - (float)texcoords[i].t / (float)header.skinheight;
    }
    SAFE_RELEASE_ARRAY(texcoords);

    // triangles
    triangles = new md2_file_triangle_t[header.num_tris]; // tmp
    if(triangles == NULL)
    {
        fprintf(stderr, "MD2: Out of memory for triangles: %i\n", header.num_tris);
        goto loaderr;
    }
    fseek(f, header.ofs_tris, SEEK_SET);
    if(fread(triangles, sizeof(md2_file_triangle_t), header.num_tris, f) !=
            (size_t)header.num_tris)
    {
        fprintf(stderr, "MD2: Failed to read triangles: %i\n", header.num_tris);
        goto loaderr;
    }
    m_trianglecount = header.num_tris;
    m_triangles = new md2_triangle_t[header.num_tris];
    if(m_triangles == NULL)
    {
        fprintf(stderr, "MD2: Out of memory for triangles: %i\n", header.num_tris);
        goto loaderr;
    }
    for(i=0;i<header.num_tris;i++)
    {
        m_triangles[i].v[0] = triangles[i].index_xyz[0];
        m_triangles[i].v[1] = triangles[i].index_xyz[1];
        m_triangles[i].v[2] = triangles[i].index_xyz[2];

        m_triangles[i].uv[0] = triangles[i].index_st[0];
        m_triangles[i].uv[1] = triangles[i].index_st[1];
        m_triangles[i].uv[2] = triangles[i].index_st[2];
    }
    SAFE_RELEASE_ARRAY(triangles);

    // frames
    fseek(f, header.ofs_frames, SEEK_SET);
    curanim.name[0] = NULL;
    for(i=0;i<header.num_frames;i++) // iterating through every frame
    {
        m_frames[i].num_xyz = header.num_xyz; // always the same?

        if(fread(frame, 1, header.framesize, f) != (uint32_t)header.framesize) // read complete frame to memory
        {
            fprintf(stderr, "MD2: Error reading frame from model\n");
            goto loaderr;
        }
        strcpy(m_frames[i].name, framehead->name);
        if((tmpname = strtok(framehead->name, "0123456789"))) // keep track of animation sequence name
        {
            if(strcmp(tmpname, curanim.name))
            {
                if(curanim.name[0] != 0)
                {
                    curanim.end = i-1;
                    animlist.push_back(curanim);
                }
                curanim.start = i;
                strcpy(curanim.name, tmpname);
            }
        }

        m_frames[i].vertices = &m_vertices[header.num_xyz * i];
        for(int j=0;j<header.num_xyz;j++) // convert compressed vertices into our normal (sane) format
        {
            vertex = &m_frames[i].vertices[j];
            vertex->v.z = -(framehead->v[j].v[0] * framehead->scale.x +
                                    framehead->translate.x);
            vertex->v.x = framehead->v[j].v[1] * framehead->scale.y +
                                    framehead->translate.y;
            vertex->v.y = framehead->v[j].v[2] * framehead->scale.z +
                                    framehead->translate.z;

            vertex->v *= MD2_LYNX_SCALE;

            if(framehead->v[j].n >= NUMVERTEXNORMALS)
            {
                fprintf(stderr, "MD2: Vertexnormal out of bounds. Frame: %i, vertex: %i\n", i, j);
                goto loaderr;
            }
            vertex->n.z = -(g_bytedirs[framehead->v[j].n][0]);
            vertex->n.x =  (g_bytedirs[framehead->v[j].n][1]);
            vertex->n.y =  (g_bytedirs[framehead->v[j].n][2]);
        }
    }
    if(curanim.name[0] != 0)
    {
        curanim.end = i-1;
        animlist.push_back(curanim);
    }
    SAFE_RELEASE_ARRAY(frame);
    fclose(f);
    f = NULL;

    m_anims = new md2_anim_t[animlist.size()];
    if(!m_anims)
    {
        fprintf(stderr, "MD2: Failed to create animation table - out of memory\n");
        goto loaderr;
    }
    m_animcount = (int)animlist.size();
    memset(m_animmap, 0, sizeof(m_animmap));
    for(i = 0, iter=animlist.begin(); iter!=animlist.end(); iter++)
    {
        m_anims[i] = (*iter);
        char* name = m_anims[i].name;

        if(strcmp(name, "stand")==0)
            m_animmap[ANIMATION_IDLE] = i;
        else if(strcmp(name, "run")==0 || strcmp(name, "walk")==0)
            m_animmap[ANIMATION_RUN] = i;
        else if(strcmp(name, "attack")==0 || strcmp(name, "atk")==0)
            m_animmap[ANIMATION_ATTACK] = i;

        i++;
    }

    if(m_shaderactive)
        return AllocVertexBuffer();
    else
        return true;

loaderr:
    SAFE_RELEASE_ARRAY(texcoords);
    SAFE_RELEASE_ARRAY(frame);
    SAFE_RELEASE_ARRAY(triangles);
    if(f)
        fclose(f);
    Unload();
    return false;
}

void CModelMD2::Unload()
{
    SAFE_RELEASE_ARRAY(m_frames);
    SAFE_RELEASE_ARRAY(m_vertices);
    SAFE_RELEASE_ARRAY(m_anims);
    SAFE_RELEASE_ARRAY(m_texcoords);
    SAFE_RELEASE_ARRAY(m_triangles);

    m_framecount = 0;
    m_vertices_per_frame = 0;
    m_animcount = 0;
    m_texcoordscount = 0;
    m_trianglecount = 0;

    m_tex = 0;
    m_normalmap = 0;

    // VBO dealloc
    DeallocVertexBuffer();
}

void CModelMD2::Animate(model_state_t* mstate, const float dt) const
{
    md2_state_t* state = (md2_state_t*)mstate;

    state->time += dt;
    if(state->time >= m_invfps)
    {
        state->time = 0.0f;
        state->curr_frame = GetNextFrameInAnim(state, 1);
        state->next_frame = GetNextFrameInAnim(state, 1);
        state->play_count++;
    }
}

int CModelMD2::GetNextFrameInAnim(const md2_state_t* state, int increment) const
{
    int size = m_anims[state->md2anim].end - m_anims[state->md2anim].start + 1;
    int step = state->curr_frame - m_anims[state->md2anim].start + increment;
    return m_anims[state->md2anim].start + (step % size);
}

void CModelMD2::SetAnimation(model_state_t* mstate, const animation_t animation) const
{
    md2_state_t* state = (md2_state_t*)mstate;

    //if(state->animation == animation)
    //    return;

    state->md2anim = m_animmap[animation % ANIMATION_COUNT];
    state->animation = animation;
    state->time = 0.0f;
    state->curr_frame = m_anims[state->md2anim].start;
    state->next_frame = GetNextFrameInAnim(state, 1);
    state->play_count = 0;
}

float CModelMD2::GetAnimationTime(const animation_t animation) const
{
    const int md2anim = m_animmap[animation % ANIMATION_COUNT];
    const int len = m_anims[md2anim].end - m_anims[md2anim].start;
    return (float)len*m_invfps;
}

void CModelMD2::DeallocVertexBuffer()
{
    if(m_vbo_vertex > 0)
    {
        m_vbo_vertex = 0;
        glDeleteBuffers(1, &m_vbo_vertex);
    }

    if(m_vboindex > 0)
    {
        glDeleteBuffers(1, &m_vboindex);
        m_vboindex = 0;
    }

    m_vbo_frame_table.clear();
}

// takes the data from m_vertices, m_texxcoords
// and m_triangles and creates VBOs for the data
bool CModelMD2::AllocVertexBuffer()
{
    if(glGetError() != GL_NO_ERROR)
    {
        assert(0); // something is wrong
        return false;
    }

    unsigned int i, j, f, a;
    // from the md2 file, the vertices have a position and
    // a normal, but the texture coordinates are stored
    // separate. we try to generate unique vertices and indices
    std::vector<md2_vertex_t> vbovertices;
    std::vector<uint32_t> vboindices;
    vbovertices.reserve(m_vertices_per_frame*m_framecount);
    vboindices.reserve(m_trianglecount*3*m_framecount);
    m_vbo_frame_table.clear();
    m_vbo_frame_table.resize(m_framecount);

    // for each animation
    for(a=0; a<m_animcount; a++)
    {
        const unsigned int framestart = m_anims[a].start;
        const unsigned int frameend = m_anims[a].end;

        // for each frame in animation
        for(f=framestart;f<=frameend;f++)
        {
            // get vertices for current frame
            // and next vertices
            const md2_vertex_t* cur_vertices = m_frames[f].vertices;
            md2_vertex_t* next_vertices;
            if(f+1 > frameend) // loop back to the animation start
                next_vertices = m_frames[framestart].vertices;
            else
                next_vertices = m_frames[f+1].vertices;

            md2_vbo_start_end_t vboindextable; // remember the position in the vboindex table for this frame
            vboindextable.start = vboindices.size();
            // for each triangle in model
            for(i=0;i<m_trianglecount;i++)
            {
                const md2_triangle_t* tri = &m_triangles[i];
                // for each vertex of triangle
                for(j=0;j<3;j++)
                {
                    // get the vertex from the md2 structure
                    uint32_t vindex = tri->v[j];
                    const md2_vertex_t* pvertex = &cur_vertices[vindex];
                    const md2_vertex_t* pvertexnext = &next_vertices[vindex];
                    const md2_texcoord_t* ptexcoord = &m_texcoords[tri->uv[j]];

                    // build the required vertex
                    md2_vertex_t vertex; // new vertex
                    vertex.v = pvertex->v; // set vertex coordinates
                    vertex.n = pvertex->n; // set normal
                    vertex.tu = ptexcoord->u;
                    vertex.tv = ptexcoord->v;
                    vertex.vdir = pvertexnext->v - pvertex->v;

                    // check if we already have the vertex
                    // otherwise create a new one
                    if(vbovertices.size() <= vindex ||
                       vertex.v != vbovertices[vindex].v ||
                       vertex.n != vbovertices[vindex].n ||
                       vertex.tu != vbovertices[vindex].tu ||
                       vertex.tv != vbovertices[vindex].tv ||
                       vertex.vdir != vbovertices[vindex].vdir)
                    {
                        vbovertices.push_back(vertex);
                        vindex = vbovertices.size()-1;
                    }
                    // build index buffer
                    vboindices.push_back(vindex);
                }
            }
            vboindextable.end = vboindices.size()-1;
            assert(vboindextable.end > vboindextable.start);

            assert(f < m_vbo_frame_table.size());
            m_vbo_frame_table[f] = vboindextable;
        }
    }

    glGenBuffers(1, &m_vbo_vertex);
    glGenBuffers(1, &m_vboindex);

    if(m_vbo_vertex < 1 || m_vboindex < 1)
    {
        fprintf(stderr, "MD2: Failed to generate VBO\n");
        return false;
    }

    fprintf(stderr, "MD2 Vertices: %i\n", (int)vbovertices.size());
    // VBO VERTEX DATA
    const unsigned int totalvbosize =
        sizeof(md2_vertex_t)*vbovertices.size();

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertex); assert(glGetError() == GL_NO_ERROR);
    // Reserve the data
    glBufferData(GL_ARRAY_BUFFER,
                 totalvbosize,
                 0,
                 GL_STATIC_DRAW); assert(glGetError() == GL_NO_ERROR);
    // Upload the data
    // 2nd parameter: VBO data offset
    // 3rd parameter: size of source data
    // 4th parameter: data pointer
    glBufferSubData(GL_ARRAY_BUFFER, 0, totalvbosize, &vbovertices[0]); assert(glGetError() == GL_NO_ERROR);
    glBindBuffer(GL_ARRAY_BUFFER, 0); assert(glGetError() == GL_NO_ERROR);
    // Finished with upload

    // INDEX BUFFER
    const unsigned int totalvboindexsize =
        sizeof(uint32_t)*vboindices.size();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vboindex); assert(glGetError() == GL_NO_ERROR);
    // Reserve the data
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 totalvboindexsize,
                 0,
                 GL_STATIC_DRAW); assert(glGetError() == GL_NO_ERROR);
    // Upload the data
    // 2nd parameter: VBO data offset
    // 3rd parameter: size of source data
    // 4th parameter: data pointer
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, totalvboindexsize, &vboindices[0]); assert(glGetError() == GL_NO_ERROR);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); assert(glGetError() == GL_NO_ERROR);
    // Finished with upload

    assert(glGetError() == GL_NO_ERROR);
    return (glGetError() == GL_NO_ERROR);
}

static GLuint LoadAndCompileShader(const unsigned int type, const std::string& path)
{
    unsigned int shader = glCreateShader(type);
    if(shader < 1)
        return shader;

    std::string shadersrc = CLynx::ReadCompleteFile(path);
    if(shadersrc == "")
    {
        fprintf(stderr, "Failed to load shader\n");
        return 0;
    }

    const char* strarray[] = { shadersrc.c_str(), NULL };
    glShaderSource(shader, 1, strarray, NULL);
    glCompileShader(shader);

    int status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(status != GL_TRUE)
    {
        char logbuffer[1024];
        int usedlogbuffer;
        glGetShaderInfoLog(shader, sizeof(logbuffer), &usedlogbuffer, logbuffer);
        fprintf(stderr, "GL Compile Shader Error: %s\n", logbuffer);
        return 0;
    }

    return shader;
}

int CModelMD2::m_program = 0; // keep this static
int CModelMD2::m_interp = 0;
bool CModelMD2::InitShader()
{
    if(m_program != 0) // we already have this shader loaded
        return true;

    int glerr = 0;
    int islinked = GL_FALSE;

    fprintf(stderr, "Compiling MD2 vertex shader...\n");
    int vshader = LoadAndCompileShader(GL_VERTEX_SHADER, CLynx::GetBaseDirFX() + "md2vshader.txt");
    if(vshader < 1)
        return false;

    fprintf(stderr, "Compiling MD2 fragment shader...\n");
    int fshader = LoadAndCompileShader(GL_FRAGMENT_SHADER, CLynx::GetBaseDirFX() + "md2fshader.txt");
    if(fshader < 1)
        return false;

    m_program = glCreateProgram();

    glAttachShader(m_program, vshader);
    if((glerr = glGetError()) != 0)
    {
        fprintf(stderr, "Failed to attach vertex shader\n");
        return false;
    }
    glAttachShader(m_program, fshader);
    if((glerr = glGetError()) != 0)
    {
        fprintf(stderr, "Failed to attach fragment shader\n");
        return false;
    }

    glLinkProgram(m_program);
    glGetProgramiv(m_program, GL_LINK_STATUS, &islinked);
    if(islinked != GL_TRUE || (glerr = glGetError()) != 0)
    {
        char logbuffer[1024];
        int usedlogbuffer;
        glGetProgramInfoLog(m_program, sizeof(logbuffer), &usedlogbuffer, logbuffer);
        fprintf(stderr, "GL Program Link Error:\n%s\n", logbuffer);
        return false;
    }

    glUseProgram(m_program);
    if((glerr = glGetError()) != 0)
    {
        if(glerr == GL_INVALID_VALUE)
            fprintf(stderr, "%s\n", "Error: No Shader program");
        else if(glerr == GL_INVALID_OPERATION)
            fprintf(stderr, "%s\n", "Error: invalid operation");
        else
            fprintf(stderr, "Failed to use program %i\n", glerr);
        return false;
    }
    int shadowMapUniform = glGetUniformLocation(m_program, "ShadowMap");
    int tex = glGetUniformLocation(m_program, "tex");
    int normalMap = glGetUniformLocation(m_program, "normalMap");
    m_interp = glGetUniformLocation(m_program, "interp");
    glUniform1i(tex, 0); // diffuse texture to channel 0
    glUniform1i(normalMap, 1); // tangent space normal map in channel 1
    glUniform1i(shadowMapUniform, 7); // shadow space matrix to channel 7
    glUniform1f(m_interp, 0.0f); // MD2 interpolation factor
    glUseProgram(0);

    int isValid;
    glValidateProgram(m_program);
    glGetProgramiv(m_program, GL_VALIDATE_STATUS, &isValid);
    if(isValid != GL_TRUE)
    {
        char logbuffer[1024];
        int usedlogbuffer;
        glGetProgramInfoLog(m_program, sizeof(logbuffer), &usedlogbuffer, logbuffer);
        fprintf(stderr, "GL Program Error:\n%s\n", logbuffer);
        return false;
    }

    return true;
}

/*

Animation: stand (0-39)
Animation: run (40-45)
Animation: attack (46-53)
Animation: pain (54-65)
Animation: jump (66-71)
Animation: flip (72-83)
Animation: salute (84-94)
Animation: taunt (95-111)
Animation: wave (112-122)
Animation: point (123-134)
Animation: crstand (135-153)
Animation: crwalk (154-159)
Animation: crattack (160-168)
Animation: crpain (169-172)
Animation: crdeath (173-177)

*/

