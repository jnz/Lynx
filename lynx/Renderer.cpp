#include "SDL.h"
#include "GL/glew.h"
#define NO_SDL_GLEXT
#include "SDL_opengl.h"
#include "lynx.h"
#include "Renderer.h"
#include "Frustum.h"
#include <stdio.h>
#include "ModelMD2.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#define PLANE_NEAR		0.1f
#define PLANE_FAR		10000.0f
#define RENDERER_FOV    90.0f

// Local Render Functions
static void BSP_RenderTree(const CBSPLevel* tree, 
						   const vec3_t* origin, 
						   CFrustum* frustum);

static void RenderCube();

//#define COLORLEAFS

#ifdef COLORLEAFS
vec3_t g_colortable[] = 
{
	vec3_t(0,0,1),
	vec3_t(0,1,0),
	vec3_t(0,1,1),
	vec3_t(0.95f,0,0),
	vec3_t(1,0,1),
	vec3_t(1,0.94f,0),
	vec3_t(0,0,0.6f),
	vec3_t(0,0.5f,0),
	vec3_t(0,0.45f,0.55f),
	vec3_t(0.65f,0,0),
	vec3_t(0.45f,0,0.75f),
	vec3_t(0.45f,0.25f,0),
	vec3_t(0.75f,0.35f,0.65f),
	vec3_t(0,0,0.65f),
	vec3_t(0,0.65f,0),
	vec3_t(0,0.45f,0.65f),
	vec3_t(0.65f,0,0),
	vec3_t(0.75f,0,0.35f),
	vec3_t(0.55f,0.65f,0),
	vec3_t(0.95f,0.35f,0.65f)
};
#endif

CRenderer::CRenderer(CWorldClient* world)
{
	m_world = world;
    m_vshader = 0;
    m_fshader = 0;
    m_program = 0;
}

CRenderer::~CRenderer(void)
{
    glDetachShader(m_program, m_vshader);
    glDeleteShader(m_vshader);
    m_vshader = 0;
    glDetachShader(m_program, m_fshader);
    glDeleteShader(m_fshader);
    m_fshader = 0;
    glDeleteProgram(m_program);

	Shutdown();
}

bool CRenderer::Init(int width, int height, int bpp, int fullscreen)
{
	int status;
	SDL_Surface* screen;

	fprintf(stderr, "SDL Init... ");
	status = SDL_InitSubSystem(SDL_INIT_VIDEO);
    assert(status == 0);
	if(status)
	{
		fprintf(stderr, " failed\n");
		return false;
	}
	fprintf(stderr, " successful\n");

	screen = SDL_SetVideoMode(width, height, bpp, 
				SDL_HWSURFACE | 
				SDL_ANYFORMAT | 
				SDL_DOUBLEBUF | 
				SDL_OPENGL | 
				(fullscreen ? SDL_FULLSCREEN : 0));
	assert(screen);
	if(!screen)
		return false;

	m_width = width;
	m_height = height;

	glClearColor(0.48f, 0.58f, 0.72f, 0.0f);
	glClearDepth(1.0f);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
	glShadeModel(GL_SMOOTH);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	UpdatePerspective();

	// Vertex Light
	float mat_specular[] = {1,1,1,1};
	float mat_shininess[] = { 50 };
	float light_pos[] = { 0, 5, 0, 0 };
	float white_light[] = {1,1,1,1};
	float lmodel_ambient[] = { 0.9f, 0.9f, 0.9f, 1.0f };
	
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
	glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
	glLightfv(GL_LIGHT0, GL_SPECULAR, white_light);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);

	glEnable(GL_LIGHT0);

	glEnable(GL_LIGHTING);

    GLenum err = glewInit();
    if(GLEW_OK != err)
    {
        fprintf(stderr, "glew error: %s\n", glewGetErrorString(err));
        return false;
    }

    if(glewIsSupported("GL_VERSION_2_0"))
    {
        fprintf(stderr, "OpenGL 2.0 support\n");
    }
    else
    {
        fprintf(stderr, "Error: No OpenGL 2.0 support\n");
        return false;
    }

    bool shader = InitShader();
    if(!shader)
    {
        fprintf(stderr, "Init shader failed\n");
        return false;
    }

	return true;
}

void CRenderer::Shutdown()
{

}

void CRenderer::Update(const float dt, const DWORD ticks)
{
	CObj* obj, *localctrl;
	int localctrlid;
	OBJITER iter;
	matrix_t m;
	vec3_t dir, up, side;
	CFrustum frustum;
	CWorld* world;

	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    localctrl = m_world->GetLocalController();
	localctrlid = m_world->GetLocalObj()->GetID();
	world = m_world->GetInterpWorld();
	m.SetCamTransform((localctrl->GetOrigin()+localctrl->GetEyePos()), 
					   localctrl->GetRot());
	
    m.GetVec3Cam(&dir, &up, &side);
    dir = -dir;
	glLoadMatrixf(m.pm);

	frustum.Setup(localctrl->GetOrigin()+localctrl->GetEyePos(), dir, up, side, 
				  RENDERER_FOV, (float)m_width/(float)m_height,
				  PLANE_NEAR, 
				  PLANE_FAR); 

	stat_obj_hidden = 0;
	stat_obj_visible = 0;


    glDisable(GL_LIGHTING);
	BSP_RenderTree(world->GetBSP(), &localctrl->GetOrigin(), &frustum);
    glEnable(GL_LIGHTING);

	for(iter=world->ObjBegin();iter!=world->ObjEnd();iter++)
	{
		obj = (*iter).second;
        if(obj->GetID() == localctrlid)
            continue;

        if(!obj->GetMesh())
            continue;

		if(!frustum.TestSphere(obj->GetOrigin(), obj->GetRadius()))
		{
			stat_obj_hidden++;
			continue;
		}
		stat_obj_visible++;

		glPushMatrix();
		glTranslatef(obj->GetOrigin().x, obj->GetOrigin().y, obj->GetOrigin().z);
        glTranslatef(0.0f, -obj->GetRadius(), 0.0f);

        glMultMatrixf(obj->GetRotMatrix()->pm);
		obj->GetMesh()->Render(obj->GetMeshState());
		obj->GetMesh()->Animate(obj->GetMeshState(), dt);
		glPopMatrix();
	}
	
    glDisable(GL_LIGHTING);

    // Particle Draw
    //glEnable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    glDepthMask(false);
	for(iter=world->ObjBegin();iter!=world->ObjEnd();iter++)
	{
		obj = (*iter).second;
        if(obj->GetID() == localctrlid || !obj->GetParticleSystem())
            continue;

        obj->GetParticleSystem()->Update(dt, ticks);

        // FIXME frustum test for particle system!
		//if(!frustum.TestSphere(obj->GetOrigin(), obj->GetRadius()))
		//{
		//	stat_obj_hidden++;
		//	continue;
		//}
		glPushMatrix();
		glTranslatef(obj->GetOrigin().x, obj->GetOrigin().y, obj->GetOrigin().z);
        obj->GetParticleSystem()->Render(side, up, dir);
		glPopMatrix();
	}
    glDepthMask(true);
    glDisable(GL_BLEND);
    //glDisable(GL_ALPHA_TEST);
    glColor4f(1,1,1,1);

    CModelMD2* viewmodel;
    md2_state_t* viewmodelstate;
    m_world->m_hud.GetModel(&viewmodel, &viewmodelstate);
    if(viewmodel)
    {
    	glClear(GL_DEPTH_BUFFER_BIT);
	    glLoadIdentity();
	    glTranslatef(0,-2.0f,1.25f);

        viewmodel->Render(viewmodelstate);
        viewmodel->Animate(viewmodelstate, dt);
    }
    glEnable(GL_LIGHTING);

	SDL_GL_SwapBuffers();
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
	glLoadIdentity();
}

std::string ReadShader(std::string path)
{
    FILE* f = fopen(path.c_str(), "rb");
    if(!f)
        return "";

    fseek(f, 0, SEEK_END);
    unsigned int fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buff = new char[fsize+10];
    if(!buff)
        return "";

    fread(buff, fsize, 1, f);
    buff[fsize] = NULL;
    std::string shader(buff);

    delete[] buff;
    fclose(f);

    return shader;
}

GLuint LoadAndCompileShader(unsigned int type, std::string path)
{
    unsigned int shader = glCreateShader(type);
    if(shader < 1)
        return shader;

    std::string shadersrc = ReadShader(path);
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
    glAttachShader(m_program, m_fshader);

    glLinkProgram(m_program);

    glUseProgram(m_program);

    return true;
}

void BSP_RenderTree(const CBSPLevel* tree, const vec3_t* origin, CFrustum* frustum)
{
	if(!tree)
		return;

    tree->RenderGL(*origin, *frustum);
}

void RenderCube() // this is handy sometimes
{
	glBegin(GL_QUADS);
		// Front Face
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);	// Bottom Left Of The Texture and Quad
		glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);	// Bottom Right Of The Texture and Quad
		glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);	// Top Right Of The Texture and Quad
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);	// Top Left Of The Texture and Quad
		// Back Face
		glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);	// Bottom Right Of The Texture and Quad
		glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);	// Top Right Of The Texture and Quad
		glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);	// Top Left Of The Texture and Quad
		glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);	// Bottom Left Of The Texture and Quad
		// Top Face
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);	// Top Left Of The Texture and Quad
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f,  1.0f,  1.0f);	// Bottom Left Of The Texture and Quad
		glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f,  1.0f,  1.0f);	// Bottom Right Of The Texture and Quad
		glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);	// Top Right Of The Texture and Quad
		// Bottom Face
		glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f, -1.0f, -1.0f);	// Top Right Of The Texture and Quad
		glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f, -1.0f, -1.0f);	// Top Left Of The Texture and Quad
		glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);	// Bottom Left Of The Texture and Quad
		glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);	// Bottom Right Of The Texture and Quad
		// Right face
		glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);	// Bottom Right Of The Texture and Quad
		glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);	// Top Right Of The Texture and Quad
		glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);	// Top Left Of The Texture and Quad
		glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);	// Bottom Left Of The Texture and Quad
		// Left Face
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);	// Bottom Left Of The Texture and Quad
		glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);	// Bottom Right Of The Texture and Quad
		glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);	// Top Right Of The Texture and Quad
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);	// Top Left Of The Texture and Quad
	glEnd();
}

#ifdef DRAW_BBOX
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
