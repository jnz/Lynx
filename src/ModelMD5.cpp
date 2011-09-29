#include "lynx.h"
#include "math/vec3.h"
#include "ModelMD5.h"
#include <stdio.h>
#include <GL/glew.h>
#define NO_SDL_GLEXT
#include <SDL/SDL_opengl.h>

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#define MD5_SCALE                (1.0f)   // scale while loading
#define MD5_SCALE_F              (0.065f) // scale by glScalef
#define MD5_RESET_BASE_POSITION // keep the model base position at 0,0,0 during the animation

/* Animation Joint info */
struct joint_info_t
{
    char name[64];
    int parent;
    int flags;
    int startIndex;
};

/* Animation Base frame joint */
struct baseframe_joint_t
{
    vec3_t pos;
    quaternion_t orient;
};

CModelMD5::CModelMD5(void)
{
    m_num_joints = 0;
    m_num_meshes = 0;
    m_max_verts = 0;
    m_max_tris = 0;
    m_vbo = 0;
    m_vboindex = 0;
    m_vertex_buffer = NULL;
    m_vertex_index_buffer = NULL;
}

CModelMD5::~CModelMD5(void)
{
    Unload();
}

void CModelMD5::Unload()
{
    DeallocVertexBuffer();

    m_meshes.clear();
    m_baseSkel.clear();

    m_num_joints = 0;
    m_num_meshes = 0;
    m_max_verts = 0;
    m_max_tris = 0;

    // Delete animations
    std::map<animation_t, md5_anim_t*>::iterator animiter;
    for(animiter=m_animations.begin();animiter!=m_animations.end();++animiter)
        delete (*animiter).second;
    m_animations.clear();
}

void CModelMD5::RenderSkeleton(const std::vector<md5_joint_t>& skel) const
{
    int i;
    const int num_joints = (int)skel.size();

    glPointSize(5.0f);
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_POINTS);
    for(i=0; i<num_joints; i++)
        glVertex3fv(skel[i].pos.v);
    glEnd ();
    glPointSize(1.0f);

    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_LINES);
    for(i=0; i<num_joints; i++)
    {
        if(skel[i].parent != -1)
        {
            glVertex3fv(skel[skel[i].parent].pos.v);
            glVertex3fv(skel[i].pos.v);
        }
    }
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
}

bool CModelMD5::AllocVertexBuffer()
{
    m_vertex_buffer = new md5_vbo_vertex_t[m_max_verts];
    m_vertex_index_buffer = new md5_vertexindex_t[m_max_tris*3];

    glGenBuffers(1, &m_vbo);
    if(m_vbo < 1)
    {
        fprintf(stderr, "MD5: Failed to generate VBO\n");
        return false;
    }

    glGenBuffers(1, &m_vboindex);
    if(m_vboindex < 1)
    {
        fprintf(stderr, "MD5: Failed to generate index VBO\n");
        return false;
    }

    return true;
}

void CModelMD5::DeallocVertexBuffer()
{
    if(m_vbo > 0)
    {
        m_vbo = 0;
        glDeleteBuffers(1, &m_vbo);
    }

    if(m_vboindex > 0)
    {
        glDeleteBuffers(1, &m_vboindex);
        m_vboindex = 0;
    }

    SAFE_RELEASE_ARRAY(m_vertex_buffer);
    SAFE_RELEASE_ARRAY(m_vertex_index_buffer);
}

bool CModelMD5::UploadVertexBuffer(unsigned int vertexcount, unsigned int indexcount) const
{
    assert((int)vertexcount <= m_max_verts);
    assert((int)indexcount <= m_max_tris*3);

    int errcode;

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    errcode = glGetError();
    if(errcode != GL_NO_ERROR)
    {
        fprintf(stderr, "MD5: Failed to bind VBO: %i\n", errcode);
        return false;
    }

    glBufferData(GL_ARRAY_BUFFER, sizeof(md5_vbo_vertex_t) * vertexcount, NULL, GL_DYNAMIC_DRAW);
    if(glGetError() != GL_NO_ERROR)
    {
        fprintf(stderr, "MD5: Failed to buffer data\n");
        return false;
    }

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(md5_vbo_vertex_t) * vertexcount, m_vertex_buffer);
    if(glGetError() != GL_NO_ERROR)
    {
        fprintf(stderr, "MD5: Failed to upload VBO data\n");
        return false;
    }

    if(glGetError() != GL_NO_ERROR)
    {
        fprintf(stderr, "MD5: Failed to init VBO\n");
        return false;
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vboindex);
    if(glGetError() != GL_NO_ERROR)
    {
        fprintf(stderr, "MD5: Failed to bind index VBO\n");
        return false;
    }

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(md5_vertexindex_t) * indexcount, NULL, GL_DYNAMIC_DRAW);
    if(glGetError() != GL_NO_ERROR)
    {
        fprintf(stderr, "MD5: Failed to buffer index data\n");
        return false;
    }

    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(md5_vertexindex_t) * indexcount, m_vertex_index_buffer);
    if(glGetError() != GL_NO_ERROR)
    {
        fprintf(stderr, "MD5: Failed to upload index VBO data\n");
        return false;
    }

    return true;
}

void CModelMD5::Render(const md5_state_t* state)
{
    // we use the opengl coordinate system here and nothing else
    glPushMatrix();
#ifdef BIG_STUPID_DOOM3_ROCKETLAUNCHER_HACK
    glTranslatef(m_big_stupid_doom3_rocketlauncher_hack_offset.x,
                 m_big_stupid_doom3_rocketlauncher_hack_offset.y,
                 m_big_stupid_doom3_rocketlauncher_hack_offset.z);
#endif
    glScalef(MD5_SCALE_F, MD5_SCALE_F, MD5_SCALE_F);
    glRotatef( 90.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

    /* Draw each mesh of the model */
    for(int i = 0; i < m_num_meshes; ++i)
    {
        if(state->animdata)
            PrepareMesh(&m_meshes[i], state->skel);
        else
            PrepareMesh(&m_meshes[i], m_baseSkel); // for static md5s with no animation

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vboindex);

        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, sizeof(md5_vbo_vertex_t), BUFFER_OFFSET(0));

        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, sizeof(md5_vbo_vertex_t), BUFFER_OFFSET(12));

        glClientActiveTexture(GL_TEXTURE0);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, sizeof(md5_vbo_vertex_t), BUFFER_OFFSET(24));

        glClientActiveTexture(GL_TEXTURE1);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(4, GL_FLOAT, sizeof(md5_vbo_vertex_t), BUFFER_OFFSET(32));

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_meshes[i].texnormal);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_meshes[i].tex);

        glDrawElements(GL_TRIANGLES,
                       m_meshes[i].num_tris*3,
                       MD5_GL_VERTEXINDEX_TYPE,
                       BUFFER_OFFSET(0 * sizeof(md5_vertexindex_t)));

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

        // Debug: draw the interpolated md5 joints
        // RenderSkeleton(state->skel);
    }
    glPopMatrix();
}

void CModelMD5::RenderNormals(const md5_state_t* state)
{
    glPushMatrix();
    glScalef(MD5_SCALE_F, MD5_SCALE_F, MD5_SCALE_F);
    glRotatef( 90.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

    glDisable(GL_LIGHTING);
    glBindTexture(GL_TEXTURE_2D, 0);
    for(int i = 0; i < m_num_meshes; ++i)
    {
        md5_mesh_t* mesh = &m_meshes[i];

        if(state->animdata)
            PrepareMesh(mesh, state->skel);
        else
            PrepareMesh(mesh, m_baseSkel); // for static md5s with no animation

        for (int j = 0; j < mesh->num_verts; ++j)
        {
            const vec3_t& vertex = m_vertex_buffer[j].v;
            const vec3_t& normal = m_vertex_buffer[j].n;
            const vec3_t& tangent = m_vertex_buffer[j].t;
            const float w = m_vertex_buffer[j].w;
            const vec3_t bitangent = w * (normal ^ tangent);

            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
            vec3_t to = vertex + 0.2f*normal;
            vec3_t to2 = vertex + 0.25f*normal;
            glBegin(GL_LINES);
                glVertex3fv(vertex.v);
                glVertex3fv(to.v);
            glEnd();
            glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
            glBegin(GL_LINES);
                glVertex3fv(to.v);
                glVertex3fv(to2.v);
            glEnd();

            glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
            to = vertex + 0.2f*tangent;
            to2 = vertex + 0.25f*tangent;
            glBegin(GL_LINES);
                glVertex3fv(vertex.v);
                glVertex3fv(to.v);
            glEnd();
            glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
            glBegin(GL_LINES);
                glVertex3fv(to.v);
                glVertex3fv(to2.v);
            glEnd();

            glColor4f(1.0f, 0.0f, 1.0f, 1.0f);
            to = vertex + 0.2f*bitangent;
            to2 = vertex + 0.25f*bitangent;
            glBegin(GL_LINES);
                glVertex3fv(vertex.v);
                glVertex3fv(to.v);
            glEnd();
            glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
            glBegin(GL_LINES);
                glVertex3fv(to.v);
                glVertex3fv(to2.v);
            glEnd();
        }
    }
    glEnable(GL_LIGHTING);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glPopMatrix();
}

void CModelMD5::Animate(md5_state_t* state, const float dt) const
{
    if(state->animdata == NULL)
        return;

    const md5_joint_t* skelA = &(state->animdata->skelFrames[state->curr_frame])[0];
    const md5_joint_t* skelB = &(state->animdata->skelFrames[state->next_frame])[0];
    const int num_joints = state->animdata->num_joints;
    const float interp = state->time * state->animdata->frameRate;
    int i;
    assert(interp >= 0.0f && interp <= 1.0f);
    assert((int)state->skel.size() == num_joints);

    for (i = 0; i < num_joints; ++i)
    {
        /* Copy parent index */
        state->skel[i].parent = skelA[i].parent;

        /* Linear interpolation for position */
        state->skel[i].pos.x = skelA[i].pos.x + interp * (skelB[i].pos.x - skelA[i].pos.x);
        state->skel[i].pos.y = skelA[i].pos.y + interp * (skelB[i].pos.y - skelA[i].pos.y);
        state->skel[i].pos.z = skelA[i].pos.z + interp * (skelB[i].pos.z - skelA[i].pos.z);

        /* Spherical linear interpolation for orientation */
        quaternion_t::Slerp(&state->skel[i].orient, skelA[i].orient, skelB[i].orient, interp);
    }

    assert(state->animdata->frameRate > 0);

    const float frametime = 1.0f/state->animdata->frameRate;
    state->time += dt;
    while(state->time >= frametime)
    {
        state->time = state->time - frametime;
        assert(state->time >= 0.0f);

        const int maxFrames = state->animdata->num_frames-1; // FIXME: is -1 correct?
        state->curr_frame = ((state->curr_frame+1) % maxFrames);
        state->next_frame = ((state->next_frame+1) % maxFrames);
    }
}

void CModelMD5::SetAnimation(md5_state_t* state, const animation_t animation)
{
    if(state->animation == animation)
    {
        return;
    }

    state->curr_frame = 0;
    state->next_frame = 1;
    state->animation = animation;
    state->animdata = GetAnimation(animation, false);
    state->time = 0.0f;
    assert((animation != ANIMATION_NONE) ? (state->animdata!=NULL) : 1);
    if(!state->animdata)
        return;
    if(state->skel.size() != (size_t)state->animdata->num_joints)
        state->skel.resize(state->animdata->num_joints);
    Animate(state, 0.0f);
}

static void md5_calculate_tangent(const md5_vertex_t& v0,
                                  const md5_vertex_t& v1,
                                  const md5_vertex_t& v2,
                                  vec3_t& facenormal,
                                  vec3_t& tangent,
                                  vec3_t& bitangent)
{
    const vec3_t& p0 = v0.vertex;
    const vec3_t& p1 = v1.vertex;
    const vec3_t& p2 = v2.vertex;
    const float s1 = v1.u - v0.u;
    const float s2 = v2.u - v0.u;
    const float t1 = v1.v - v0.v;
    const float t2 = v2.v - v0.v;
    const float det = s1*t2 - s2*t1;
    const vec3_t Q1(p1 - p0);
    const vec3_t Q2(p2 - p0);
    if(fabsf(det) <= lynxmath::EPSILON)
    {
        //fprintf(stderr, "Warning: Unable to compute tangent + bitangent\n");
        tangent = vec3_t(1.0f, 0.0f, 0.0f);
        bitangent = vec3_t(0.0f, 1.0f, 0.0f);
        return;
    }
    const float idet = 1 / det;
    tangent = vec3_t(idet*(t2*Q1.x - t1*Q2.x),
                     idet*(t2*Q1.y - t1*Q2.y),
                     idet*(t2*Q1.z - t1*Q2.z));
    bitangent = vec3_t(idet*(-s2*Q1.x + s1*Q2.x),
                       idet*(-s2*Q1.y + s1*Q2.y),
                       idet*(-s2*Q1.z + s1*Q2.z));
    facenormal = (p2-p0)^(p1-p0);

    assert(tangent.Abs()>0.01f);
    assert(bitangent.Abs()>0.01f);
}

// Calculate the vertex position and the
// normals in the bind pose (baseSkel)
void CModelMD5::PrepareBindPoseNormals(md5_mesh_t *mesh)
{
    int i, j;
    vec3_t wv;

    /* Setup vertices */
    for(i = 0; i < mesh->num_verts; ++i)
    {
        vec3_t finalVertex(0.0f, 0.0f, 0.0f);

        /* Calculate final vertex to draw with weights */
        for(j = 0; j < mesh->vertices[i].count; ++j)
        {
            assert(mesh->vertices[i].start + j < (int)mesh->weights.size());
            const md5_weight_t *weight = &mesh->weights[mesh->vertices[i].start + j];
            assert(weight->joint < (int)m_baseSkel.size());
            const md5_joint_t *joint = &m_baseSkel[weight->joint];

            /* Calculate transformed vertex for this weight */
            wv = joint->orient.Vec3Multiply(weight->pos);

            /* The sum of all weight->bias should be 1.0 */
            finalVertex += (joint->pos + wv) * weight->bias;
        }

        mesh->vertices[i].vertex = finalVertex;
    }

    // Compute the vertex normals in the mesh's bind pose
    vec3_t normal, tangent, bitangent;
    for(i = 0; i < mesh->num_tris; ++i)
    {
        md5_vertex_t& v0 = mesh->vertices[mesh->triangles[i].index[0]];
        md5_vertex_t& v1 = mesh->vertices[mesh->triangles[i].index[1]];
        md5_vertex_t& v2 = mesh->vertices[mesh->triangles[i].index[2]];

        // cross product
        md5_calculate_tangent(v0, v1, v2, normal, tangent, bitangent);

        v0.normal += normal;
        v1.normal += normal;
        v2.normal += normal;
        v0.tangent += tangent;
        v1.tangent += tangent;
        v2.tangent += tangent;
        v0.bitangent += bitangent;
        v1.bitangent += bitangent;
        v2.bitangent += bitangent;
    }

    // Reset weight normal
    for(i = 0; i< mesh->num_weights; i++)
    {
        md5_weight_t *weight = &mesh->weights[i];
        weight->normal = vec3_t::origin;
        weight->tangent = vec3_t::origin;
        weight->bitangent = vec3_t::origin;
    }

    // Normalize the normals
    for(i = 0; i < mesh->num_verts; ++i)
    {
        mesh->vertices[i].normal.Normalize();
        mesh->vertices[i].tangent.Normalize();
        mesh->vertices[i].bitangent.Normalize();
        mesh->vertices[i].w = 1.0f;
        // FIXME not correct, should be something like
        // (((n^t) * bitangents[vindex]) < 0.0f) ? -1.0f : 1.0f

        // Calculate the normal in joint-local space
        // so the animated normal can be computed faster later
        for(j = 0; j < mesh->vertices[i].count; ++j)
        {
            md5_weight_t *weight = &mesh->weights[mesh->vertices[i].start + j];
            const md5_joint_t *joint = &m_baseSkel[weight->joint];

            quaternion_t invq = joint->orient;
            invq.Invert();
            weight->normal += invq.Vec3Multiply(mesh->vertices[i].normal);
            weight->tangent += invq.Vec3Multiply(mesh->vertices[i].tangent);
            weight->bitangent += invq.Vec3Multiply(mesh->vertices[i].bitangent);
        }
    }

    // Normalize the normal in joint local space
    for(i = 0; i< mesh->num_weights; i++)
    {
        md5_weight_t *weight = &mesh->weights[i];
        weight->normal.Normalize();
        weight->tangent.Normalize();
        weight->bitangent.Normalize();

        const vec3_t& n = weight->normal;
        const vec3_t& t = weight->tangent;
        // Gram-Schmidt
        weight->tangent = (t - n * (n*t)).Normalized();
        if(!weight->tangent.IsNormalized())
            weight->tangent = vec3_t::yAxis;
    }
}

void CModelMD5::PrepareMesh(const md5_mesh_t *mesh, const std::vector<md5_joint_t>& skeleton)
{
    int i, j, k;
    vec3_t wv;

    /* Setup vertex indices */
    for(k = 0, i = 0; i < mesh->num_tris; ++i)
    {
        for(j = 0; j < 3; ++j, ++k)
        {
            m_vertex_index_buffer[k] = mesh->triangles[i].index[j];
        }
    }
    assert(k <= m_max_tris*3);

    /* Setup vertices */
    for (i = 0; i < mesh->num_verts; ++i)
    {
        vec3_t finalVertex(0.0f, 0.0f, 0.0f);
        vec3_t finalNormal(0.0f, 0.0f, 0.0f);
        vec3_t finalTangent(0.0f, 0.0f, 0.0f);

        /* Calculate final vertex to draw with weights */
        for (j = 0; j < mesh->vertices[i].count; ++j)
        {
            const md5_weight_t *weight = &mesh->weights[mesh->vertices[i].start + j];
            const md5_joint_t *joint = &skeleton[weight->joint];

            /* Calculate transformed vertex for this weight */
            wv = joint->orient.Vec3Multiply(weight->pos);

            /* The sum of all weight->bias should be 1.0 */
            finalVertex += (joint->pos + wv) * weight->bias;
            finalNormal += joint->orient.Vec3Multiply(weight->normal) * weight->bias;
            finalTangent += joint->orient.Vec3Multiply(weight->tangent) * weight->bias;
        }

        m_vertex_buffer[i].v = finalVertex;
        m_vertex_buffer[i].n = finalNormal.Normalized();
        m_vertex_buffer[i].t = finalTangent.Normalized();
        m_vertex_buffer[i].w = mesh->vertices[i].w;
        m_vertex_buffer[i].tu = mesh->vertices[i].u;
        m_vertex_buffer[i].tv = mesh->vertices[i].v;
    }

    UploadVertexBuffer(mesh->num_verts, mesh->num_tris*3);
}

bool CModelMD5::Load(char *path, CResourceManager* resman, bool loadtexture)
{
    FILE* f;
    char buff[1024];
    int version;
    int curr_mesh = 0;
    int i;

#ifdef BIG_STUPID_DOOM3_ROCKETLAUNCHER_HACK
    std::string path_hack = path;
    if(path_hack.find("rocketlauncher") != std::string::npos)
    {
        m_big_stupid_doom3_rocketlauncher_hack_offset.y -= 3.7f;
        m_big_stupid_doom3_rocketlauncher_hack_offset.x += 0.5f;
        m_big_stupid_doom3_rocketlauncher_hack_offset.z -= 0.0f;
    }
#endif

    f = fopen(path, "rb");
    if(!f)
    {
        fprintf(stderr, "MD5: Failed to open file: %s\n", path);
        goto loaderr;
    }

    Unload();

    while(!feof(f))
    {
        /* Read whole line */
        fgets(buff, sizeof(buff), f);

        if(sscanf(buff, " MD5Version %d", &version) == 1)
        {
            if(version != 10)
            {
                /* Bad version */
                fprintf(stderr, "Error: bad model version\n");
                fclose(f);
                return 0;
            }
        }
        else if(sscanf (buff, " numJoints %d", &m_num_joints) == 1)
        {
            if(m_num_joints > 0 && m_baseSkel.size() == 0)
            {
                /* Allocate memory for base skeleton joints */
                m_baseSkel.resize(m_num_joints);
            }
            else
            {
                fprintf(stderr, "%s\n", "MD5: Number of joints < 1\n");
                goto loaderr;
            }
        }
        else if(sscanf(buff, " numMeshes %d", &m_num_meshes) == 1)
        {
            if(m_num_meshes > 0 && m_meshes.size() == 0)
            {
                /* Allocate memory for meshes */
                m_meshes.resize(m_num_meshes);
            }
            else
            {
                fprintf(stderr, "%s\n", "MD5: Number of meshes < 1\n");
                goto loaderr;
            }
        }
        else if(strncmp (buff, "joints {", 8) == 0)
        {
            /* Read each joint */
            for(i = 0;i < m_num_joints;i++)
            {
                assert(i < (int)m_baseSkel.size());
                md5_joint_t *joint = &m_baseSkel[i];
                vec3_t tmppos;

                /* Read whole line */
                fgets(buff, sizeof(buff), f);

                /* FIXME possible buffer overflow here for joint->name */
                if(sscanf(buff, "%s %d ( %f %f %f ) ( %f %f %f )",
                          joint->name, &joint->parent, &tmppos.x,
                          &tmppos.y, &tmppos.z, &joint->orient.x,
                          &joint->orient.y, &joint->orient.z) == 8)
                {
                    // MD5 only stores x, y and z of the unit length quaternion
                    joint->orient.ComputeW(); // w = -sqrt(1 - x*x + y*y + z*z)
                    joint->pos.x =  tmppos.x * MD5_SCALE;
                    joint->pos.y =  tmppos.y * MD5_SCALE;
                    joint->pos.z =  tmppos.z * MD5_SCALE;
                    //joint->pos.x =  tmppos.y * MD5_SCALE;
                    //joint->pos.y =  tmppos.z * MD5_SCALE;
                    //joint->pos.z = -tmppos.x * MD5_SCALE;
                }
            }
        }
        else if(strncmp (buff, "mesh {", 6) == 0)
        {
            if(curr_mesh >= m_num_meshes)
            {
                fprintf(stderr, "%s\n", "MD5 Invalid mesh index!\n");
                goto loaderr;
            }

            md5_mesh_t *mesh = &m_meshes[curr_mesh];
            int vert_index = 0;
            int tri_index = 0;
            int weight_index = 0;
            float fdata[4];
            int idata[3];

            while((buff[0] != '}') && !feof(f))
            {
                /* Read whole line */
                fgets(buff, sizeof(buff), f);

                if(strstr(buff, "shader "))
                {
                    int quote = 0, j = 0;

                    /* Copy the shader name whithout the quote marks */
                    for(i = 0; i < (int)sizeof(buff) && (quote < 2); ++i)
                    {
                        if(buff[i] == '\"')
                            quote++;

                        if((quote == 1) && (buff[i] != '\"'))
                        {
                            mesh->shader[j] = buff[i];
                            j++;
                        }
                    }
                    assert(j < (int)sizeof(mesh->shader));
                    mesh->shader[j] = NULL;
                    if(loadtexture) // The server needs no texture
                    {
                        // disable error messages for the gettexture function
                        std::string texpath(CLynx::GetDirectory(path) + CLynx::GetFilename(mesh->shader));
                        if(texpath.length() > 0)
                        {
                            mesh->tex = resman->GetTexture(CLynx::ChangeFileExtension(texpath, "jpg"), true);
                            if(mesh->tex == 0) // let's try: jpeg
                                mesh->tex = resman->GetTexture(CLynx::ChangeFileExtension(texpath, "jpeg"), true);
                            if(mesh->tex == 0) // let's try: tga
                                mesh->tex = resman->GetTexture(CLynx::ChangeFileExtension(texpath, "tga"), true);
                            if(mesh->tex == 0) // let's try: png
                                mesh->tex = resman->GetTexture(CLynx::ChangeFileExtension(texpath, "png"), true);
                            if(mesh->tex == 0)
                            {
                                fprintf(stderr, "MD5 warning: no texture found for model: %s\n", texpath.c_str());
                            }

                            mesh->texnormal = resman->GetTexture(CLynx::ChangeFileExtension(texpath, "_bump.jpg"), true);
                            if(mesh->texnormal == 0) // let's try: jpeg
                                mesh->texnormal = resman->GetTexture(CLynx::ChangeFileExtension(texpath, "_bump.jpeg"), true);
                            if(mesh->texnormal == 0) // let's try: tga
                                mesh->texnormal = resman->GetTexture(CLynx::ChangeFileExtension(texpath, "_bump.tga"), true);
                            if(mesh->texnormal == 0) // let's try: png
                                mesh->texnormal = resman->GetTexture(CLynx::ChangeFileExtension(texpath, "_bump.png"), true);
                            if(mesh->texnormal == 0)
                            {
                                //fprintf(stderr, "MD5 warning: no normal texture found for model: %s\n", texpath.c_str());
                                mesh->texnormal = resman->GetTexture(CLynx::GetBaseDirTexture() + "normal.jpg");
                            }
                        }
                    }

                }
                else if(sscanf (buff, " numverts %d", &mesh->num_verts) == 1)
                {
                    if(mesh->num_verts > 0)
                    {
                        /* Allocate memory for vertices */
                        mesh->vertices.resize(mesh->num_verts);
                    }

                    if(mesh->num_verts > m_max_verts)
                        m_max_verts = mesh->num_verts;
                }
                else if(sscanf (buff, " numtris %d", &mesh->num_tris) == 1)
                {
                    if(mesh->num_tris > 0)
                    {
                        /* Allocate memory for triangles */
                        mesh->triangles.resize(mesh->num_tris);
                    }

                    if(mesh->num_tris > m_max_tris)
                        m_max_tris = mesh->num_tris;
                }
                else if(sscanf (buff, " numweights %d", &mesh->num_weights) == 1)
                {
                    if(mesh->num_weights > 0)
                    {
                        /* Allocate memory for vertex weights */
                        mesh->weights.resize(mesh->num_weights);
                    }
                }
                else if(sscanf (buff, " vert %d ( %f %f ) %d %d", &vert_index,
                            &fdata[0], &fdata[1], &idata[0], &idata[1]) == 5)
                {
                    /* Copy vertex data */
                    mesh->vertices[vert_index].u = fdata[0];
                    mesh->vertices[vert_index].v = 1.0f - fdata[1];
                    mesh->vertices[vert_index].start = idata[0];
                    mesh->vertices[vert_index].count = idata[1];
                }
                else if(sscanf (buff, " tri %d %d %d %d", &tri_index,
                            &idata[0], &idata[1], &idata[2]) == 4)
                {
                    /* Copy triangle data */
                    mesh->triangles[tri_index].index[0] = idata[2];
                    mesh->triangles[tri_index].index[1] = idata[1];
                    mesh->triangles[tri_index].index[2] = idata[0];
                }
                else if(sscanf (buff, " weight %d %d %f ( %f %f %f )",
                            &weight_index, &idata[0], &fdata[3],
                            &fdata[0], &fdata[1], &fdata[2]) == 6)
                {
                    /* Copy vertex data */
                    mesh->weights[weight_index].joint =  idata[0];
                    mesh->weights[weight_index].bias  =  fdata[3];
                    mesh->weights[weight_index].pos.x =  fdata[0] * MD5_SCALE;
                    mesh->weights[weight_index].pos.y =  fdata[1] * MD5_SCALE;
                    mesh->weights[weight_index].pos.z =  fdata[2] * MD5_SCALE;
                    //mesh->weights[weight_index].pos.x =  fdata[1] * MD5_SCALE;
                    //mesh->weights[weight_index].pos.y =  fdata[2] * MD5_SCALE;
                    //mesh->weights[weight_index].pos.z = -fdata[0] * MD5_SCALE;
                }
            }

            // Lets prepare the normals in the bind pose
            PrepareBindPoseNormals(mesh);
            curr_mesh++;
        }
    }
    fclose(f);

    // thanks to m_max_tris and m_max_verts we know
    // how large our vertex buffer has to be
    if(loadtexture)
        AllocVertexBuffer();

    ReadAnimation(ANIMATION_IDLE,   CLynx::GetDirectory(path) + "idle.md5anim");
    ReadAnimation(ANIMATION_IDLE1,  CLynx::GetDirectory(path) + "idle1.md5anim");
    ReadAnimation(ANIMATION_IDLE2,  CLynx::GetDirectory(path) + "idle2.md5anim");
    ReadAnimation(ANIMATION_RUN,    CLynx::GetDirectory(path) + "run.md5anim");
    ReadAnimation(ANIMATION_ATTACK, CLynx::GetDirectory(path) + "attack.md5anim");
    ReadAnimation(ANIMATION_FIRE,   CLynx::GetDirectory(path) + "fire.md5anim");

    return true;

loaderr:
    if(f)
        fclose(f);
    Unload();
    return false;
}

static void BuildFrameSkeleton(const std::vector<joint_info_t>& jointInfos,
                               const std::vector<baseframe_joint_t>& baseFrame,
                               const std::vector<float>& animFrameData,
                               std::vector<md5_joint_t>& skelFrame,
                               int num_joints)
{
    int i;
    assert((int)skelFrame.size() == num_joints);

    for (i = 0; i < num_joints; ++i)
    {
        const baseframe_joint_t *baseJoint = &baseFrame[i];
        vec3_t animatedPos = baseJoint->pos;
        quaternion_t animatedOrient = baseJoint->orient;
        int j = 0;

        if (jointInfos[i].flags & 1) /* Tx */
        {
            animatedPos.v[0] = animFrameData[jointInfos[i].startIndex + j] * MD5_SCALE;
            ++j;
        }

        if (jointInfos[i].flags & 2) /* Ty */
        {
            animatedPos.v[1] = animFrameData[jointInfos[i].startIndex + j] * MD5_SCALE;
            ++j;
        }

        if (jointInfos[i].flags & 4) /* Tz */
        {
            animatedPos.v[2] = animFrameData[jointInfos[i].startIndex + j] * MD5_SCALE;
            ++j;
        }

        if (jointInfos[i].flags & 8) /* Qx */
        {
            animatedOrient.x = animFrameData[jointInfos[i].startIndex + j];
            ++j;
        }

        if (jointInfos[i].flags & 16) /* Qy */
        {
            animatedOrient.y = animFrameData[jointInfos[i].startIndex + j];
            ++j;
        }

        if (jointInfos[i].flags & 32) /* Qz */
        {
            animatedOrient.z = animFrameData[jointInfos[i].startIndex + j];
            ++j;
        }

        /* Compute orient quaternion's w value */
        animatedOrient.ComputeW();

        /* NOTE: we assume that this joint's parent has
           already been calculated, i.e. joint's ID should
           never be smaller than its parent ID. */
        md5_joint_t *thisJoint = &skelFrame[i];

        int parent = jointInfos[i].parent;
        thisJoint->parent = parent;
        strcpy (thisJoint->name, jointInfos[i].name);

        /* Has parent? */
        if (thisJoint->parent < 0)
        {
#ifdef MD5_RESET_BASE_POSITION   // prevent model from floating
            assert(baseFrame.size()>0);
            thisJoint->pos = baseFrame[0].pos;
#else
            thisJoint->pos = animatedPos;
#endif
            thisJoint->orient = animatedOrient;
        }
        else
        {
            md5_joint_t *parentJoint = &skelFrame[parent];
            vec3_t rpos; /* Rotated position */

            /* Add positions */
            rpos = parentJoint->orient.Vec3Multiply(animatedPos);
            thisJoint->pos.v[0] = rpos.v[0] + parentJoint->pos.v[0];
            thisJoint->pos.v[1] = rpos.v[1] + parentJoint->pos.v[1];
            thisJoint->pos.v[2] = rpos.v[2] + parentJoint->pos.v[2];

            /* Concatenate rotations */
            thisJoint->orient = parentJoint->orient * animatedOrient;
            thisJoint->orient.Normalize();
        }
    }
}

md5_anim_t* CModelMD5::GetAnimation(const animation_t animation, bool createnew)
{
    md5_anim_t* panim;
    if(animation == ANIMATION_NONE)
        return NULL;

    const std::map<animation_t, md5_anim_t*>::const_iterator iter = m_animations.find(animation);
    if(iter == m_animations.end())
    {
        if(createnew)
        {
            panim = new md5_anim_t;
            m_animations[animation] = panim;
        }
        else
        {
            fprintf(stderr, "Animation not found: %s\n", CResourceManager::GetStringFromAnimation(animation).c_str());
            panim = NULL;
        }
    }
    else
    {
        if(createnew)
        {
            fprintf(stderr, "Animation already loaded: %s\n", CResourceManager::GetStringFromAnimation(animation).c_str());
        }

        panim = (*iter).second;
    }

    return panim;
}

bool CModelMD5::ReadAnimation(const animation_t animation, const std::string filename)
{
    FILE* f = NULL;
    char buff[512];
    std::vector<joint_info_t> jointInfos;
    std::vector<baseframe_joint_t> baseFrame;
    std::vector<float> animFrameData;
    int version;
    int numAnimatedComponents;
    int frame_index;
    int i;
    md5_anim_t* anim;

    f = fopen(filename.c_str(), "rb");
    if(!f)
    {
        //fprintf (stderr, "error: couldn't open \"%s\"!\n", filename.c_str());
        return false;
    }

    anim = GetAnimation(animation, true); // create a new animation
    if(anim == NULL)
    {
        assert(0);
        return false; // problem
    }

    while(!feof(f))
    {
        /* Read whole line */
        fgets(buff, sizeof (buff), f);

        if(sscanf(buff, " MD5Version %d", &version) == 1)
        {
            if(version != 10)
            {
                /* Bad version */
                fprintf (stderr, "MD5 Error: bad animation version\n");
                fclose (f);
                return false;
            }
        }
        else if(sscanf (buff, " numFrames %d", &anim->num_frames) == 1)
        {
            /* Allocate memory for skeleton frames and bounding boxes */
            if(anim->num_frames > 0)
            {
                anim->skelFrames.resize(anim->num_frames);
                anim->bboxes.resize(anim->num_frames);
            }
        }
        else if(sscanf (buff, " numJoints %d", &anim->num_joints) == 1)
        {
            if(anim->num_joints > 0)
            {
                for (i = 0; i < anim->num_frames; ++i)
                {
                    /* Allocate memory for joints of each frame */
                    anim->skelFrames[i].resize(anim->num_joints);
                }

                /* Allocate temporary memory for building skeleton frames */
                jointInfos.resize(anim->num_joints);
                baseFrame.resize(anim->num_joints);
            }
        }
        else if(sscanf (buff, " frameRate %d", &anim->frameRate) == 1)
        {
            //printf ("md5anim: animation's frame rate is %d\n", anim->frameRate);
        }
        else if(sscanf (buff, " numAnimatedComponents %d", &numAnimatedComponents) == 1)
        {
            if(numAnimatedComponents > 0)
            {
                /* Allocate memory for animation frame data */
                animFrameData.resize(numAnimatedComponents);
            }
        }
        else if(strncmp (buff, "hierarchy {", 11) == 0)
        {
            for (i = 0; i < anim->num_joints; ++i)
            {
                /* Read whole line */
                fgets (buff, sizeof (buff), f);

                /* Read joint info */
                sscanf (buff, " %s %d %d %d", jointInfos[i].name, &jointInfos[i].parent,
                        &jointInfos[i].flags, &jointInfos[i].startIndex);
            }
        }
        else if(strncmp (buff, "bounds {", 8) == 0)
        {
            for (i = 0; i < anim->num_frames; ++i)
            {
                /* Read whole line */
                fgets (buff, sizeof (buff), f);

                /* Read bounding box */
                sscanf (buff, " ( %f %f %f ) ( %f %f %f )",
                        &anim->bboxes[i].min.v[0], &anim->bboxes[i].min.v[1],
                        &anim->bboxes[i].min.v[2], &anim->bboxes[i].max.v[0],
                        &anim->bboxes[i].max.v[1], &anim->bboxes[i].max.v[2]);
                anim->bboxes[i].min *= MD5_SCALE;
                anim->bboxes[i].max *= MD5_SCALE;
            }
        }
        else if(strncmp (buff, "baseframe {", 10) == 0)
        {
            for (i = 0; i < anim->num_joints; ++i)
            {
                /* Read whole line */
                fgets (buff, sizeof (buff), f);

                /* Read base frame joint */
                if(sscanf (buff, " ( %f %f %f ) ( %f %f %f )",
                            &baseFrame[i].pos.v[0], &baseFrame[i].pos.v[1],
                            &baseFrame[i].pos.v[2], &baseFrame[i].orient.x,
                            &baseFrame[i].orient.y, &baseFrame[i].orient.z) == 6)
                {
                    /* Compute the w component */
                    baseFrame[i].pos *= MD5_SCALE;
                    baseFrame[i].orient.ComputeW();
                }
            }
        }
        else if(sscanf (buff, " frame %d", &frame_index) == 1)
        {
            /* Read frame data */
            for (i = 0; i < numAnimatedComponents; ++i)
                fscanf (f, "%f", &animFrameData[i]);

            /* Build frame skeleton from the collected data */
            BuildFrameSkeleton(jointInfos, baseFrame, animFrameData,
                               anim->skelFrames[frame_index], anim->num_joints);
        }
    }

    fclose (f);

    return true;
}

