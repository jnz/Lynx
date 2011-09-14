#include "lynx.h"
#include "math/vec3.h"
#include "ModelMD5.h"
#include "BSPBIN.h" // we use the bspbin_vertex_t type
#include <stdio.h>
#include <GL/glew.h>
#define NO_SDL_GLEXT
#include <SDL/SDL_opengl.h>

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#define MD5_SCALE           (0.05f)

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
    m_baseSkel = NULL;
    m_meshes = NULL;
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

    if(m_meshes)
    {
        assert(m_num_meshes > 0);
        for(int i=0;i<m_num_meshes;i++)
        {
            SAFE_RELEASE_ARRAY(m_meshes[i].vertices);
            SAFE_RELEASE_ARRAY(m_meshes[i].triangles);
            SAFE_RELEASE_ARRAY(m_meshes[i].weights);
        }
    }
    SAFE_RELEASE_ARRAY(m_meshes);
    SAFE_RELEASE_ARRAY(m_baseSkel);

    m_num_joints = 0;
    m_num_meshes = 0;
    m_max_verts = 0;
    m_max_tris = 0;

    // Delete animations
    std::map<animation_t, md5_anim_t*>::iterator animiter;
    for(animiter=m_animations.begin();animiter!=m_animations.end();animiter++)
        delete (*animiter).second;
    m_animations.clear();
}

void CModelMD5::RenderSkeleton(const md5_joint_t* skel) const
{
    int i;

    glPointSize(5.0f);
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_POINTS);
    for(i=0; i<m_num_joints; i++)
        glVertex3fv(skel[i].pos.v);
    glEnd ();
    glPointSize(1.0f);

    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_LINES);
    for(i=0; i<m_num_joints; i++)
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
    m_vertex_buffer = new bspbin_vertex_t[m_max_verts];
    m_vertex_index_buffer = new vertexindex_t[m_max_tris*3];

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

    glBufferData(GL_ARRAY_BUFFER, sizeof(bspbin_vertex_t) * vertexcount, NULL, GL_DYNAMIC_DRAW);
    if(glGetError() != GL_NO_ERROR)
    {
        fprintf(stderr, "MD5: Failed to buffer data\n");
        return false;
    }

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bspbin_vertex_t) * vertexcount, m_vertex_buffer);
    if(glGetError() != GL_NO_ERROR)
    {
        fprintf(stderr, "MD5: Failed to upload VBO data\n");
        return false;
    }

    // the following depends on the bspbin_vertex_t struct
    // (byte offsets)
    glVertexPointer(3, GL_FLOAT, sizeof(bspbin_vertex_t), BUFFER_OFFSET(0));
    glNormalPointer(GL_FLOAT, sizeof(bspbin_vertex_t), BUFFER_OFFSET(12));
    glClientActiveTexture(GL_TEXTURE0);
    glTexCoordPointer(2, GL_FLOAT, sizeof(bspbin_vertex_t), BUFFER_OFFSET(24));
    glClientActiveTexture(GL_TEXTURE1);
    glTexCoordPointer(4, GL_FLOAT, sizeof(bspbin_vertex_t), BUFFER_OFFSET(36));

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

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(vertexindex_t) * indexcount, NULL, GL_DYNAMIC_DRAW);
    if(glGetError() != GL_NO_ERROR)
    {
        fprintf(stderr, "MD5: Failed to buffer index data\n");
        return false;
    }

    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(vertexindex_t) * indexcount, m_vertex_index_buffer);
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
    glRotatef( 90.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

    // glFrontFace(GL_CW);
    // glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);

    /* Draw each mesh of the model */
    for(int i = 0; i < m_num_meshes; ++i)
    {
        PrepareMesh(&m_meshes[i], state->skel);

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vboindex);

        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, sizeof(bspbin_vertex_t), BUFFER_OFFSET(0));

        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, sizeof(bspbin_vertex_t), BUFFER_OFFSET(12));

        glClientActiveTexture(GL_TEXTURE0);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, sizeof(bspbin_vertex_t), BUFFER_OFFSET(24));

        //glClientActiveTexture(GL_TEXTURE1);
        //glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        //glTexCoordPointer(4, GL_FLOAT, sizeof(bspbin_vertex_t), BUFFER_OFFSET(32));

        //glActiveTexture(GL_TEXTURE1);
        //glBindTexture(GL_TEXTURE_2D, m_texturebatch[i].texidnormal);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_meshes[i].tex);

        glDrawElements(GL_TRIANGLES,
                       m_meshes[i].num_tris*3,
                       MY_GL_VERTEXINDEX_TYPE,
                       BUFFER_OFFSET(0 * sizeof(vertexindex_t)));

        //glActiveTexture(GL_TEXTURE1);
        //glBindTexture(GL_TEXTURE_2D, 0);

        //glClientActiveTexture(GL_TEXTURE1);
        //glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        //glDisable(GL_TEXTURE_2D);

        glActiveTexture(GL_TEXTURE0);
        glClientActiveTexture(GL_TEXTURE0);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);

        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    //RenderSkeleton(m_baseSkel);
    //glFrontFace(GL_CCW);
    //glPolygonMode (GL_FRONT, GL_FILL);
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

    const float frametime = 1.0f/state->animdata->frameRate;
    state->time += dt;
    while(state->time >= frametime)
    {
        state->time = state->time - frametime;
        state->curr_frame++;
        state->next_frame++;

        const int maxFrames = state->animdata->num_frames - 1;
        if(state->curr_frame > maxFrames)
            state->curr_frame = 0;
        if(state->next_frame > maxFrames)
            state->next_frame = 0;
    }
}

void CModelMD5::SetAnimation(md5_state_t* state, const animation_t animation)
{
    state->curr_frame = 0;
    state->next_frame = 1;
    state->animation = animation;
    state->animdata = GetAnimation(animation, false);
    state->time = 0.0f;
    assert((animation != ANIMATION_NONE) ? (state->animdata!=NULL) : 1);
    if(!state->animdata)
        return;
    state->skel.resize(state->animdata->num_joints);
    Animate(state, 0.0f);
    fprintf(stderr, "Anim\n");
}

void CModelMD5::PrepareMesh(const md5_mesh_t *mesh, const std::vector<md5_joint_t>& skeleton)
{
    int i, j, k;

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

        /* Calculate final vertex to draw with weights */
        for (j = 0; j < mesh->vertices[i].count; ++j)
        {
            const md5_weight_t *weight = &mesh->weights[mesh->vertices[i].start + j];
            const md5_joint_t *joint = &skeleton[weight->joint];

            /* Calculate transformed vertex for this weight */
            vec3_t wv;
            joint->orient.Vec3Multiply(weight->pos, &wv);

            /* The sum of all weight->bias should be 1.0 */
            finalVertex.x += (joint->pos.x + wv.x) * weight->bias;
            finalVertex.y += (joint->pos.y + wv.y) * weight->bias;
            finalVertex.z += (joint->pos.z + wv.z) * weight->bias;
        }

        m_vertex_buffer[i].v = finalVertex;
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
            if(m_num_joints > 0 && m_baseSkel == NULL)
            {
                /* Allocate memory for base skeleton joints */
                m_baseSkel = new md5_joint_t[m_num_joints];
            }
            else
            {
                fprintf(stderr, "%s\n", "MD5: Number of joints < 1\n");
                goto loaderr;
            }
        }
        else if(sscanf(buff, " numMeshes %d", &m_num_meshes) == 1)
        {
            if(m_num_meshes > 0 && m_meshes == NULL)
            {
                /* Allocate memory for meshes */
                m_meshes = new md5_mesh_t[m_num_meshes];
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
                             if(mesh->tex == 0)
                             {
                                 fprintf(stderr, "MD5 warning: no texture found for model: %s\n", texpath.c_str());
                             }
                         }
                    }

                }
                else if(sscanf (buff, " numverts %d", &mesh->num_verts) == 1)
                {
                    if(mesh->num_verts > 0)
                    {
                        /* Allocate memory for vertices */
                        mesh->vertices = new md5_vertex_t[mesh->num_verts];
                    }

                    if(mesh->num_verts > m_max_verts)
                        m_max_verts = mesh->num_verts;
                }
                else if(sscanf (buff, " numtris %d", &mesh->num_tris) == 1)
                {
                    if(mesh->num_tris > 0)
                    {
                        /* Allocate memory for triangles */
                        mesh->triangles = new md5_triangle_t[mesh->num_tris];
                    }

                    if(mesh->num_tris > m_max_tris)
                        m_max_tris = mesh->num_tris;
                }
                else if(sscanf (buff, " numweights %d", &mesh->num_weights) == 1)
                {
                    if(mesh->num_weights > 0)
                    {
                        /* Allocate memory for vertex weights */
                        mesh->weights = new md5_weight_t[mesh->num_weights];
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

            curr_mesh++;
        }
    }

    // thanks to m_max_tris and m_max_verts we know
    // how large our vertex buffer has to be
    if(loadtexture)
        AllocVertexBuffer();

    ReadAnimation(ANIMATION_IDLE,   CLynx::GetDirectory(path) + "idle.md5anim");
    ReadAnimation(ANIMATION_IDLE1,  CLynx::GetDirectory(path) + "idle1.md5anim");
    ReadAnimation(ANIMATION_IDLE2,  CLynx::GetDirectory(path) + "idle2.md5anim");
    ReadAnimation(ANIMATION_RUN,    CLynx::GetDirectory(path) + "run.md5anim");
    ReadAnimation(ANIMATION_ATTACK, CLynx::GetDirectory(path) + "attack.md5anim");

    fclose (f);

    return true;

loaderr:
    fclose(f);
    Unload();
    return false;
}

static void BuildFrameSkeleton(const joint_info_t *jointInfos,
		                       const baseframe_joint_t *baseFrame,
		                       const float* animFrameData,
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
            thisJoint->pos = animatedPos;
            thisJoint->orient = animatedOrient;
        }
        else
        {
            md5_joint_t *parentJoint = &skelFrame[parent];
            vec3_t rpos; /* Rotated position */

            /* Add positions */
            parentJoint->orient.Vec3Multiply(animatedPos, &rpos);
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
    joint_info_t *jointInfos = NULL;
    baseframe_joint_t *baseFrame = NULL;
    float *animFrameData = NULL;
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
                jointInfos = new joint_info_t[anim->num_joints];
                baseFrame = new baseframe_joint_t[anim->num_joints];
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
                animFrameData = new float[numAnimatedComponents];
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

    /* Free temporary data allocated */
    SAFE_RELEASE(animFrameData);
    SAFE_RELEASE(baseFrame);
    SAFE_RELEASE(jointInfos);

    return true;
}

