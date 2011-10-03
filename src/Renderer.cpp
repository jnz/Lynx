#include <SDL/SDL.h>
#include <GL/glew.h>
#define NO_SDL_GLEXT
#include <SDL/SDL_opengl.h>
#include "lynx.h"
#include "Renderer.h"
#include "Frustum.h"
#include <stdio.h>

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#define PLANE_NEAR          0.1f
#define PLANE_FAR           800.0f
#define RENDERER_FOV        90.0f
#define SHADOW_MAP_RATIO    1.0f

// #define DRAW_NORMALS
// #define DRAW_SHADOWMAP // debug mode: draw a window with the cam perspective of the first light

#ifdef DRAW_SHADOWMAP
unsigned int g_fboShadowCamColor = 0; // debug
#endif

// Shadow mapping bias matrix
static const float g_shadowBias[16] = { // Moving from unit cube [-1,1] to [0,1]
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 0.5, 0.0,
    0.5, 0.5, 0.5, 1.0 };

CRenderer::CRenderer(CWorldClient* world)
{
    m_world = world;
    m_vshader = 0;
    m_fshader = 0;
    m_program = 0;

    // Shadow mapping
    m_useShadows = true;
    m_shadowMapUniform = 0;
    m_fboId = 0;
    m_depthTextureId = 0;

    // Crosshair default
    m_crosshair = 0;
    m_crosshair_width = 0;
    m_crosshair_height = 0;

    // Config
    m_lightmapactive = CLynx::cfg.GetVarAsInt("uselightmap", 1);
    m_shaderactive = CLynx::cfg.GetVarAsInt("useshader", 1);
}

CRenderer::~CRenderer(void)
{
    if(m_shaderactive)
    {
        glDetachShader(m_program, m_vshader);
        glDeleteShader(m_vshader);
        m_vshader = 0;
        glDetachShader(m_program, m_fshader);
        glDeleteShader(m_fshader);
        m_fshader = 0;
        glDeleteProgram(m_program);
        m_program = 0;
    }

    Shutdown();
}

bool CRenderer::Init(int width, int height, int bpp, int fullscreen)
{
    m_width = width;
    m_height = height;

    UpdatePerspective();
    glLoadIdentity(); // identity for modelview matrix

    // Vertex Lighting
    float mat_specular[] = {1,1,1,1};
    float mat_shininess[] = { 50 };
    float mat_diff[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    float light_pos[] = { 0, 1, 1, 1 };
    float white_light[] = {1,1,1,1};
    float lmodel_ambient[] = { 0.8f, 0.8f, 0.8f, 1.0f };

    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diff);
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
    glLightfv(GL_LIGHT0, GL_SPECULAR, white_light);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);

    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);

    if(m_shaderactive)
    {
        bool shader = InitShader();
        if(!shader)
        {
            fprintf(stderr, "Init shader failed\n");
            return false;
        }
        fprintf(stderr, "GLSL shader loaded\n");

        if(!glewIsSupported("GL_EXT_framebuffer_object"))
        {
            fprintf(stderr, "No framebuffer support\n");
            m_useShadows = false;
        }

        if(m_useShadows && !CreateShadowFBO())
        {
            fprintf(stderr, "Init shadow mapping failed\n");
            m_useShadows = false;
        }
        else
        {
            fprintf(stderr, "Shadow mapping active\n");
        }
    }
    else
    {
        fprintf(stderr, "Shader disabled\n");
        m_useShadows = false;
    }

    // loading the crosshair
    std::string pathcross = CLynx::GetBaseDirTexture() + "crosshair.tga";
    m_crosshair = m_world->GetResourceManager()->GetTexture(pathcross);
    if(m_crosshair == 0)
        fprintf(stderr, "No crosshair found: %s\n", pathcross.c_str());
    m_world->GetResourceManager()->GetTextureDimension(pathcross,
             &m_crosshair_width, &m_crosshair_height);
    // loading the font
    m_font.Init("font.png", 16, 24, m_world->GetResourceManager());

    return true;
}

void CRenderer::Shutdown()
{

}

void CRenderer::DrawScene(const CFrustum& frustum,
                          CWorld* world,
                          int localctrlid,
                          bool generateShadowMap)
{
    CObj* obj;
    OBJITER iter;

    // Draw the level
    if(!generateShadowMap && world->GetBSP()->IsLoaded())
    {
        // Draw the level with lightmapping?
        if(m_lightmapactive && m_shaderactive)
            glUniform1i(m_uselightmap, 1);

        world->GetBSP()->RenderGL(frustum.pos, frustum);

        if(m_shaderactive)
            glUniform1i(m_uselightmap, 0);
    }

    // Draw every object
    for(iter=world->ObjBegin();iter!=world->ObjEnd();++iter)
    {
        obj = (*iter).second;

        if((obj->GetFlags() & OBJ_FLAGS_GHOST) || // ghosts are invisible - duh
            obj->GetID() == localctrlid || // don't draw the player object
           !obj->GetMesh()) // object has no md5 model
            continue;

        // check if object is in view frustum
        if(!generateShadowMap && !frustum.TestSphere(obj->GetOrigin(), obj->GetRadius()))
        {
            stat_obj_hidden++;
            continue;
        }
        if(!generateShadowMap)
            stat_obj_visible++;

        glPushMatrix();
        glTranslatef(obj->GetOrigin().x, obj->GetOrigin().y, obj->GetOrigin().z);
        glTranslatef(0.0f, -obj->GetRadius(), 0.0f);
        glMultMatrixf(obj->GetRotMatrix()->pm);

        obj->GetMesh()->Render(obj->GetMeshState());
#ifdef DRAW_NORMALS
        obj->GetMesh()->RenderNormals(obj->GetMeshState()); // not implemented for md2s?
#endif
        glPopMatrix();
    }
}

void CRenderer::PrepareShadowMap(const vec3_t& lightpos,
                                 const quaternion_t& lightrot,
                                 CWorld* world, int localctrlid)
{
    CFrustum frustum;
    vec3_t dir, up, side;
    matrix_t mviewlight;
    mviewlight.SetCamTransform(lightpos, lightrot);

    mviewlight.GetVec3Cam(&dir, &up, &side);
    dir = -dir;
    frustum.Setup(lightpos, dir, up, side,
                  RENDERER_FOV, (float)m_width/(float)m_height,
                  PLANE_NEAR,
                  PLANE_FAR);
    glViewport(0, 0, (int)(m_width * SHADOW_MAP_RATIO),
                     (int)(m_height * SHADOW_MAP_RATIO));
    float projection[16];
    glMatrixMode(GL_PROJECTION);
    // Shadow mapping with ortho projection can be useful
    //const float lightDistance = PLANE_FAR*0.1f;
    //glLoadIdentity();
    //glOrtho(-35.0f, 35.0f, -35.0f, 35.0f, 0.0f, PLANE_FAR*0.1f); // dimension: light area in m
    glGetFloatv(GL_PROJECTION_MATRIX, projection);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(mviewlight.pm); // set camera to light pos

    glMatrixMode(GL_TEXTURE);
    glActiveTexture(GL_TEXTURE7);
    glLoadMatrixf(g_shadowBias); // to map from -1..1 to 0..1
    glMultMatrixf(projection);
    glMultMatrixf(mviewlight.pm);
    glMatrixMode(GL_MODELVIEW);

    // Render to FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_BLEND);
#ifndef DRAW_SHADOWMAP
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
#endif
    //glCullFace(GL_FRONT);
    glPolygonOffset( 1.1f, 4.0f );
    glEnable(GL_POLYGON_OFFSET_FILL);

    DrawScene(frustum, world, localctrlid, true);

    glDisable(GL_POLYGON_OFFSET_FILL);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_width, m_height);
#ifndef DRAW_SHADOWMAP
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
#endif
    //glCullFace(GL_BACK);
    glEnable(GL_LIGHTING);
    glEnable(GL_BLEND);

    UpdatePerspective(); // restore standard projection
}

// gets called every frame to draw the scene
void CRenderer::Update(const float dt, const uint32_t ticks)
{
    CObj* obj, *localctrl;
    int localctrlid;
    OBJITER iter;
    matrix_t m;
    vec3_t dir, up, side;
    CFrustum frustum;
    CWorld* world;

    localctrl = m_world->GetLocalController();
    localctrlid = m_world->GetLocalObj()->GetID();
    world = m_world->GetInterpWorld();

    const vec3_t campos = localctrl->GetOrigin()+localctrl->GetEyePos();
    const quaternion_t camrot = localctrl->GetRot();

    m.SetCamTransform(campos, camrot);
    m.GetVec3Cam(&dir, &up, &side);
    dir = -dir;
    frustum.Setup(campos, dir, up, side,
                  RENDERER_FOV, (float)m_width/(float)m_height,
                  PLANE_NEAR,
                  PLANE_FAR);

    // light floating above the players head
    const vec3_t lightpos0(campos + vec3_t(0,25.0f,0));
    const quaternion_t lightrot0(quaternion_t(vec3_t::xAxis, -90.0f*lynxmath::DEGTORAD));
    const float lightpos0_4f[4] = {lightpos0.x, lightpos0.y, lightpos0.z, 1};
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos0_4f);
    if(m_shaderactive)
    {
        if(m_useShadows)
        {
            glUseProgram(0); // draw shadowmap with fixed function pipeline
            PrepareShadowMap(lightpos0,
                    lightrot0,
                    world,
                    localctrlid); // the player is the light
        }
        glUseProgram(m_program); // activate shader
    }

    stat_obj_hidden = 0;
    stat_obj_visible = 0;

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(m.pm);

    glClear(GL_DEPTH_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0); // normal texture channel

    if(m_shaderactive)
    {
        glUniform1i(m_tex, 0); // good old textures: GL_TEXTURE0
        glUniform1i(m_normalMap, 1); // normal maps are GL_TEXTURE1
        glUniform1i(m_lightmap, 2); // lightmap

        if(m_useShadows)
        {
            glUniform1i(m_shadowMapUniform, 7);
            glActiveTexture(GL_TEXTURE7); // shadow mapping texture GL_TEXTURE7
            glBindTexture(GL_TEXTURE_2D, m_depthTextureId);
            glActiveTexture(GL_TEXTURE0);
        }

#ifdef DRAW_NORMALS
        glUseProgram(0); // use fixed pipeline for this debug mode
#endif
    }

    // Main drawing
    DrawScene(frustum, world, localctrlid, false);

    if(m_shaderactive)
        glUseProgram(0); // don't use shader for particles

    // Particle Draw
    glDisable(GL_LIGHTING);
    glDepthMask(false);
    for(iter=world->ObjBegin();iter!=world->ObjEnd();++iter)
    {
        obj = (*iter).second;

        if(obj->GetMesh())
        {
            // Animate mesh is done in the particle loop
            // and not in DrawScene, because DrawScene
            // can be called multiple times per frame
            obj->GetMesh()->Animate(obj->GetMeshState(), dt);
        }

        if(obj->GetID() == localctrlid || !obj->GetParticleSystem())
            continue;

        obj->GetParticleSystem()->Update(dt, ticks, obj->GetOrigin());

        // FIXME use some kind of frustum test for the particle system
        //glPushMatrix();
        //glTranslatef(obj->GetOrigin().x, obj->GetOrigin().y, obj->GetOrigin().z);
        obj->GetParticleSystem()->Render(side, up, dir);
        //glPopMatrix();
    }
    glDepthMask(true);
    glColor4f(1,1,1,1);
    glEnable(GL_LIGHTING);

#ifdef DRAW_NORMALS
    // Draw vertex normals of level geometry (not face normals)
    if(world && world->GetBSP())
    {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        world->GetBSP()->RenderNormals();
        glEnable(GL_LIGHTING);
        glEnable(GL_DEPTH_TEST);
    }
#endif

    // Draw weapon
    if(m_shaderactive) // use shader for weapon
        glUseProgram(m_program);

    CModelMD5* viewmodel;
    md5_state_t* viewmodelstate;
    m_world->m_hud.GetModel(&viewmodel, &viewmodelstate);
    glDisable(GL_LIGHTING);
    if(viewmodel)
    {
        glClear(GL_DEPTH_BUFFER_BIT);
        glPushMatrix(); // why is this necessary, hidden bug?
        glLoadIdentity();
        //glTranslatef(0.4f, -3.5f, 0.3f); // weapon offset

        viewmodel->Render(viewmodelstate);
        viewmodel->Animate(viewmodelstate, dt);
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);

    // Draw HUD
    if(m_shaderactive)
        glUseProgram(0); // no shader for HUD

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, m_width, m_height, 0, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glColor4f(1,1,1,1);
    glBindTexture(GL_TEXTURE_2D, m_crosshair);

    // Draw center crosshair
    glBegin(GL_QUADS);
        glTexCoord2d(0,1);
        glVertex3f((m_width - m_crosshair_width)*0.5f, (m_height - m_crosshair_height)*0.5f, 0.0f);
        glTexCoord2d(0,0);
        glVertex3f((m_width - m_crosshair_width)*0.5f, (m_height + m_crosshair_height)*0.5f, 0.0f);
        glTexCoord2d(1,0);
        glVertex3f((m_width + m_crosshair_width)*0.5f, (m_height + m_crosshair_height)*0.5f, 0.0f);
        glTexCoord2d(1,1);
        glVertex3f((m_width + m_crosshair_width)*0.5f, (m_height - m_crosshair_height)*0.5f, 0.0f);
    glEnd();

    // draw current score
    char hudtextbuf[64];
    sprintf(hudtextbuf, "Frags: %i", m_world->m_hud.score);
    m_font.DrawGL(10.0f, m_height - 30.0f, 0.0f, hudtextbuf);
    sprintf(hudtextbuf, "%i", m_world->m_hud.health);
    m_font.DrawGL(10.0f, m_height - 35.0f - (float)m_font.GetHeight(), 0.0f, hudtextbuf);

#ifdef DRAW_SHADOWMAP
    if(m_shaderactive)
    {
        //glBindTexture(GL_TEXTURE_2D, m_depthTextureId);
        glBindTexture(GL_TEXTURE_2D, g_fboShadowCamColor);
        const float shadowmap_debug_width = 200.0f;
        const float shadowmap_debug_height = shadowmap_debug_width*(float)m_height/(float)m_width;
        glBegin(GL_QUADS);
            glTexCoord2d(0,1); // upper left
            glVertex3f(0.0f, 0.0f, 0.0f);
            glTexCoord2d(0,0); //lower left
            glVertex3f(0.0f, shadowmap_debug_height, 0.0f);
            glTexCoord2d(1,0); //lower right
            glVertex3f(shadowmap_debug_width, shadowmap_debug_height, 0.0f);
            glTexCoord2d(1,1); // upper right
            glVertex3f(shadowmap_debug_width, 0.0f, 0.0f);
        glEnd();
    }
#endif

    glBindTexture(GL_TEXTURE_2D, 0);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
}

void CRenderer::UpdatePerspective()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(RENDERER_FOV,
                    (float)m_width/(float)m_height,
                    PLANE_NEAR,
                    PLANE_FAR);
    glMatrixMode(GL_MODELVIEW);
}

static GLuint LoadAndCompileShader(unsigned int type, std::string path)
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

bool CRenderer::InitShader()
{
    int glerr = 0;
    int islinked = GL_FALSE;

    fprintf(stderr, "Compiling vertex shader...\n");
    m_vshader = LoadAndCompileShader(GL_VERTEX_SHADER, CLynx::GetBaseDirFX() + "vshader.txt");
    if(m_vshader < 1)
        return false;

    fprintf(stderr, "Compiling fragment shader...\n");
    m_fshader = LoadAndCompileShader(GL_FRAGMENT_SHADER, CLynx::GetBaseDirFX() + "fshader.txt");
    if(m_fshader < 1)
        return false;

    m_program = glCreateProgram();

    glAttachShader(m_program, m_vshader);
    if((glerr = glGetError()) != 0)
    {
        fprintf(stderr, "Failed to attach vertex shader\n");
        return false;
    }
    glAttachShader(m_program, m_fshader);
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
    m_shadowMapUniform = glGetUniformLocation(m_program, "ShadowMap");
    m_tex = glGetUniformLocation(m_program, "tex");
    m_lightmap = glGetUniformLocation(m_program, "lightmap");
    m_uselightmap = glGetUniformLocation(m_program, "uselightmap");
    m_normalMap = glGetUniformLocation(m_program, "normalMap");
    glUniform1i(m_tex, 0); // diffuse texture to channel 0
    glUniform1i(m_normalMap, 1); // tangent space normal map in channel 1
    glUniform1i(m_lightmap, 2); // pre-baked shadows to channel 2
    glUniform1i(m_shadowMapUniform, 7); // shadow space matrix to channel 7
    glUniform1i(m_uselightmap, 0);
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

bool CRenderer::CreateShadowFBO()
{
    // fbo + depth buffer
    int shadowMapWidth = (int)(m_width * SHADOW_MAP_RATIO);
    int shadowMapHeight = (int)(m_height * SHADOW_MAP_RATIO);

    GLenum FBOstatus;

    // Try to use a texture depth component
    glGenTextures(1, &m_depthTextureId);
    glBindTexture(GL_TEXTURE_2D, m_depthTextureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);

    // Remove artifact on the edges of the shadowmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

    // No need to force GL_DEPTH_COMPONENT24, drivers usually give you the max precision if available
    glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapWidth, shadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);

    glBindTexture(GL_TEXTURE_2D, 0);

#ifdef DRAW_SHADOWMAP
    glGenTextures(1, &g_fboShadowCamColor);
    glBindTexture(GL_TEXTURE_2D, g_fboShadowCamColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, shadowMapWidth, shadowMapHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
#endif

    // create a framebuffer object
    glGenFramebuffers(1, &m_fboId);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);

#ifdef DRAW_SHADOWMAP
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_fboShadowCamColor, 0);
#else
    // Instruct openGL that we won't bind a color texture with the current FBO
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
#endif

    // attach the texture to FBO depth attachment point
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTextureId, 0);

    // check FBO status
    FBOstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(FBOstatus != GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr, "GL_FRAMEBUFFER_COMPLETE failed, CANNOT use FBO\n");
        return false;
    }

    // switch back to window-system-provided framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

// Graveyard begins here

#if 0
void RenderCube() // this is handy sometimes
{
    glBegin(GL_QUADS);
        // Front Face
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);  // Bottom Left Of The Texture and Quad
        glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);  // Bottom Right Of The Texture and Quad
        glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);  // Top Right Of The Texture and Quad
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);  // Top Left Of The Texture and Quad
        // Back Face
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);  // Bottom Right Of The Texture and Quad
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);  // Top Right Of The Texture and Quad
        glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);  // Top Left Of The Texture and Quad
        glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);  // Bottom Left Of The Texture and Quad
        // Top Face
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);  // Top Left Of The Texture and Quad
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f,  1.0f,  1.0f);  // Bottom Left Of The Texture and Quad
        glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f,  1.0f,  1.0f);  // Bottom Right Of The Texture and Quad
        glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);  // Top Right Of The Texture and Quad
        // Bottom Face
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f, -1.0f, -1.0f);  // Top Right Of The Texture and Quad
        glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f, -1.0f, -1.0f);  // Top Left Of The Texture and Quad
        glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);  // Bottom Left Of The Texture and Quad
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);  // Bottom Right Of The Texture and Quad
        // Right face
        glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);  // Bottom Right Of The Texture and Quad
        glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);  // Top Right Of The Texture and Quad
        glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);  // Top Left Of The Texture and Quad
        glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);  // Bottom Left Of The Texture and Quad
        // Left Face
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);  // Bottom Left Of The Texture and Quad
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);  // Bottom Right Of The Texture and Quad
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);  // Top Right Of The Texture and Quad
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);  // Top Left Of The Texture and Quad
    glEnd();
}
#endif

#if 0
void DrawBBox(const vec3_t& min, const vec3_t& max)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glColor3f(1,1,1);

    glBegin(GL_LINE_LOOP);
        glVertex3fv(min.v);
        glVertex3f(max.x, min.y, min.z);
        glVertex3f(max.x, max.y, min.z);
        glVertex3f(min.x, max.y, min.z);
    glEnd();
    glBegin(GL_LINE_LOOP);
        glVertex3fv(max.v);
        glVertex3f(max.x, min.y, max.z);
        glVertex3f(min.x, min.y, max.z);
        glVertex3f(min.x, max.y, max.z);
    glEnd();
    glBegin(GL_LINES);
        glVertex3f(max.x, max.y, max.z);
        glVertex3f(max.x, max.y, min.z);
    glEnd();
    glBegin(GL_LINES);
        glVertex3f(min.x, max.y, max.z);
        glVertex3f(min.x, max.y, min.z);
    glEnd();
    glBegin(GL_LINES);
        glVertex3f(max.x, min.y, max.z);
        glVertex3f(max.x, min.y, min.z);
    glEnd();
    glBegin(GL_LINES);
        glVertex3f(min.x, min.y, max.z);
        glVertex3f(min.x, min.y, min.z);
    glEnd();

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
}
#endif

/*
    glGetFloatv(GL_MODELVIEW_MATRIX, &m.m[0][0]);
 */
    // DEBUG only. this piece of code draw the depth buffer onscreen
    // The only problem is: it does not work
    // glActiveTexture(GL_TEXTURE7);
    // glBindTexture(GL_TEXTURE_2D, 0);
    // glActiveTexture(GL_TEXTURE0);
    // glUseProgram(0);
    // glClear(GL_DEPTH_BUFFER_BIT);
    // glDisable(GL_LIGHTING);
    // glMatrixMode(GL_PROJECTION);
    // glLoadIdentity();
    // glOrtho(-m_width/2,m_width/2,-m_height/2,m_height/2,1,20);
    // glMatrixMode(GL_MODELVIEW);
    // glLoadIdentity();
    // glColor4f(1,1,1,1);
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, m_depthTextureId);
    // glEnable(GL_TEXTURE_2D);
    // glTranslated(0,0,-1);
    // glBegin(GL_QUADS);
    // glTexCoord2d(0,0);glVertex3f(0,0,0);
    // glTexCoord2d(1,0);glVertex3f(m_width/2,0,0);
    // glTexCoord2d(1,1);glVertex3f(m_width/2,m_height/2,0);
    // glTexCoord2d(0,1);glVertex3f(0,m_height/2,0);
    // glEnd();
    // glEnable(GL_LIGHTING);
    // UpdatePerspective();

