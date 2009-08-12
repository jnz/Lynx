#include "SDL.h"
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

//#define DRAWFRUSTUM

// Local Render Functions
#ifdef DRAWFRUSTUM
static void RenderFrustum(void* frustum);
#endif
static void BSP_RenderTree(const CBSPTree* tree, 
						   const vec3_t* origin, 
						   CFrustum* frustum,
                           int* leafs_visited);

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
}

CRenderer::~CRenderer(void)
{
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
	UpdatePerspective();
/**/
	// Vertex Light
	float mat_specular[] = {1,1,1,1};
	float mat_shininess[] = { 50 };
	float light_pos[] = { 0, 0, 1, 0 };
	float white_light[] = {1,1,1,1};
	float lmodel_ambient[] = { 0.99f, 0.99f, 0.99f, 1.0f };
	
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
	glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
	glLightfv(GL_LIGHT0, GL_SPECULAR, white_light);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);

	glEnable(GL_LIGHT0);
/**/
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

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    localctrl = m_world->GetLocalController();
	localctrlid = m_world->GetLocalObj()->GetID();
	world = m_world->GetInterpWorld();
	m.SetCamTransform((localctrl->GetOrigin()+localctrl->GetEyePos()), 
					   localctrl->GetRot());
	
    m.GetVec3Cam(&dir, &up, &side);
	glLoadMatrixf(m.pm);

/* <Frust drawing> */
#ifdef DRAWFRUSTUM
	static CPosition lastpos;
	if(CLynx::GetKeyState()[SDLK_f])
		lastpos = obj->pos;
	matrix_t tmp;
	tmp.SetCamTransform(&(lastpos.origin), &lastpos.rot);
	tmp.GetVec3Cam(&dir, &up, &side);
	frustum.Setup(lastpos.origin+vec3_t(0,1,0), dir, up, side, 
				  obj->GetFOV(), (float)m_width/(float)m_height,
				  PLANE_NEAR, 
				  PLANE_FAR);
#else
	frustum.Setup(localctrl->GetOrigin()+localctrl->GetEyePos(), dir, up, side, 
				  localctrl->GetFOV(), (float)m_width/(float)m_height,
				  PLANE_NEAR, 
				  PLANE_FAR); 
#endif
/* </Frust drawing> */

	stat_obj_hidden = 0;
	stat_obj_visible = 0;
    stat_bsp_leafs_visited = 0;

	glBindTexture(GL_TEXTURE_2D, 
		world->GetResourceManager()->GetTexture(CLynx::GetBaseDirLevel() + "testlvl/wall.tga")); // FIXME
#ifdef DRAWFRUSTUM
	BSP_RenderTree(&world->m_bsptree, &lastpos.origin, &frustum, &stat_bsp_leafs_visited);
#else
	BSP_RenderTree(world->GetBSP(), &localctrl->GetOrigin(), &frustum, &stat_bsp_leafs_visited);
#endif

	glEnable(GL_LIGHTING);
	for(iter=world->ObjBegin();iter!=world->ObjEnd();iter++)
	{
		obj = (*iter).second;
        if(obj->GetID() == localctrlid)
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
		glMultMatrixf(obj->m.pm);
		if(obj->m_mesh)
		{
			obj->m_mesh->Render(&obj->m_mesh_state);
			obj->m_mesh->Animate(&obj->m_mesh_state, dt);
		}
		else
			RenderCube();
		glPopMatrix();
	}
	glDisable(GL_LIGHTING);

#ifdef DRAWFRUSTUM
	RenderFrustum(&frustum);
#endif

	/*
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	glTranslatef(0,0,-2);
	glScalef(0.1,0.1,0.1);
	RenderCube();
	glEnable(GL_DEPTH_TEST);
	*/

	SDL_GL_SwapBuffers();
}

void CRenderer::UpdatePerspective()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(m_world->GetLocalObj()->GetFOV(), 
					(float)m_width/(float)m_height, 
					PLANE_NEAR, 
					PLANE_FAR);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void BSP_RenderPolygons(const CBSPTree* tree, 
						const std::vector<bsp_poly_t>& polylist)
{
	int c;
	int t;
	int tcount;
	int polycount = (int)polylist.size();
	bsp_poly_t* poly;
	int vi, ni, ti;

	assert(polycount > 0);

	for(c=0;c<polycount;c++)
	{
		poly = (bsp_poly_t*)&polylist[c];
		tcount = (int)poly->vertices.size();

        glBegin(GL_POLYGON);
		for(t=0;t<tcount;t++)
		{
			vi = poly->vertices[t];
			ni = poly->normals[t];
			ti = poly->texcoords[t];
			glTexCoord2f(tree->m_texcoords[ti].x, tree->m_texcoords[ti].y);
			glNormal3fv(tree->m_normals[ni].v);
			glVertex3fv(tree->m_vertices[vi].v);
		}
		glEnd();
	}
}

/*
	In-order BSP Tree walking
*/
void BSP_RenderNode(const CBSPTree* tree, 
					const CBSPTree::CBSPNode* node, 
					const vec3_t* pos, 
					CFrustum* frustum,
                    int* leafs_visited)
{
	if(!frustum->TestSphere(node->sphere_origin, node->sphere))
		return;

	if(node->IsLeaf())
	{
#ifdef COLORLEAFS
		if(node->marker)
		{
			int index = node->marker%(sizeof(g_colortable)/sizeof(g_colortable[0]));
			vec3_t color = g_colortable[index];
			glColor3fv(color.v);
		}
		else
			glColor3f(1,1,1);
#endif
		BSP_RenderPolygons(tree, node->polylist);
        (*leafs_visited)++;
		return;
	}

	switch(node->plane.Classify(*pos))
	{
	case POINTPLANE_FRONT:
		if(node->back)
			BSP_RenderNode(tree, node->back, pos, frustum, leafs_visited);
		if(node->front)
			BSP_RenderNode(tree, node->front, pos, frustum, leafs_visited);
		break;
	case POINTPLANE_BACK:
	case POINT_ON_PLANE:
		if(node->front)
			BSP_RenderNode(tree, node->front, pos, frustum, leafs_visited);
		if(node->back)
			BSP_RenderNode(tree, node->back, pos, frustum, leafs_visited);
		break;
	}
}

void BSP_RenderTree(const CBSPTree* tree, const vec3_t* origin, CFrustum* frustum, int* leafs_visited)
{
	if(!tree || !tree->m_root)
		return;

	//tree->ClearMarks(tree->m_root);
	//tree->MarkLeaf(frustum->pos, 10, tree->m_root);
    *leafs_visited = 0;
	
	glDepthFunc(GL_ALWAYS); // We do this, because the tree is already sorted, but we need the z-buffer info for the models
	BSP_RenderNode(tree, tree->m_root, origin, frustum, leafs_visited);
	glDepthFunc(GL_LEQUAL);
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

#ifdef DRAWFRUSTUM
void RenderFrustum(void* frustum)
{
	CFrustum* frust = (CFrustum*)frustum;

	glColor3f(1,1,1);

	glBegin(GL_LINES);
		glVertex3fv(frust->ntl.v);
		glVertex3fv(frust->ftl.v);
	glEnd();
	glBegin(GL_LINES);
		glVertex3fv(frust->ntr.v);
		glVertex3fv(frust->ftr.v);
	glEnd();
	glBegin(GL_LINES);
		glVertex3fv(frust->nbr.v);
		glVertex3fv(frust->fbr.v);
	glEnd();
	glBegin(GL_LINES);
		glVertex3fv(frust->nbl.v);
		glVertex3fv(frust->fbl.v);
	glEnd();

	glColor3f(1,0,0);
	glBegin(GL_LINE_LOOP);
		glVertex3fv(frust->nbl.v);
		glVertex3fv(frust->ntl.v);
		glVertex3fv(frust->ntr.v);
		glVertex3fv(frust->nbr.v);
	glEnd();
	glColor3f(0,0,1);
	glBegin(GL_LINE_LOOP);
		glVertex3fv(frust->fbl.v);
		glVertex3fv(frust->ftl.v);
		glVertex3fv(frust->ftr.v);
		glVertex3fv(frust->fbr.v);
	glEnd();
	
	glColor3f(1,1,1);
}
#endif

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
