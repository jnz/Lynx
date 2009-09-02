#include "lynx.h"
#include "math/vec3.h"
#include "ModelMD2.h"
#include <stdio.h>
#include <memory.h>
#include "GL/glew.h"
#define NO_SDL_GLEXT
#include "SDL_opengl.h"
#include <list>

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#pragma warning(push)
#pragma warning(disable: 4305)
#define NUMVERTEXNORMALS 162
typedef float vec3array[3];
vec3array g_bytedirs[NUMVERTEXNORMALS] = // quake 2 normal lookup table
{
	#include "anorms.h"
};
#pragma warning(pop)

#define	MAX_SKINNAME	64
#define MD2_LYNX_SCALE	(0.1f)

#pragma pack(push, 1)

// every struct here, that is byte aligned, is "on-disk" layout

struct md2header_t // (from quake2 src)
{
	int			ident;
	int			version;

	int			skinwidth;
	int			skinheight;
	int			framesize;		// byte size of each frame

	int			num_skins;
	int			num_xyz;
	int			num_st;			// greater than num_xyz for seams
	int			num_tris;
	int			num_glcmds;		// dwords in strip/fan command list
	int			num_frames;

	int			ofs_skins;		// each skin is a MAX_SKINNAME string
	int			ofs_st;			// byte offset from start for stverts
	int			ofs_tris;		// offset for dtriangles
	int			ofs_frames;		// offset for first frame
	int			ofs_glcmds;	
	int			ofs_end;		// end of file
};

struct md2vertex_t
{
	BYTE v[3];		// compressed vertex
	BYTE n;			// normalindex
};

struct md2frame_t
{
	vec3_t	scale;
	vec3_t	translate;
	char	name[16];
	md2vertex_t v[1];
};

#pragma pack(pop)

struct vertex_t
{
	vec3_t v; // vector
	vec3_t n; // normal
};

struct frame_t
{
	char name[16];
	int num_xyz;
	vertex_t* vertices;
};

struct anim_t
{
	char name[16];
	int start;
	int end;
};

/*
	We only use the GLcmd version here.

	Quick 'n dirty benchmark:

	glcmds:
		276 fps (25 Objs)
	no glcmds
		156 fps (25 Objs)
*/

CModelMD2::CModelMD2(void)
{
	m_frames = NULL;
	m_framecount = 0;
	m_anims = NULL;
	m_vertices = NULL;
	m_vertices_per_frame = 0;
	m_animcount = 0;
	m_glcmds = NULL;
	m_glcmdcount = 0;
	m_sphere = 0;
	SetFPS(6.0f);
	m_tex = 0;
}

CModelMD2::~CModelMD2(void)
{
	Unload();
}

void CModelMD2::Render(const md2_state_t* state) const
{
	assert(m_frames);
	
	vertex_t* cur_vertices  = m_frames[state->cur_frame].vertices;
	vertex_t* next_vertices = m_frames[GetNextFrameInAnim(state, 1)].vertices;
	vertex_t* cur_vertex;
	vertex_t* next_vertex;
	vec3_t inter_xyz, inter_n; // interpolated
	int count;
	float *u, *v;
	int* cmd = m_glcmds;
	
	glBindTexture(GL_TEXTURE_2D, m_tex);
	glTranslatef(0,2.5f,0);

	while(count = *cmd++)
	{
		if(count > 0)
		{
			glBegin(GL_TRIANGLE_STRIP);
		}
		else
		{
			glBegin(GL_TRIANGLE_FAN);
			count = -count;
		}

		do
		{
			u = (float*)cmd++;
			v = (float*)cmd++;
			glTexCoord2f(*u, 1.0f-*v);

			cur_vertex = &cur_vertices[*cmd];
			next_vertex = &next_vertices[*cmd];

			inter_n = cur_vertex->n + 
						(next_vertex->n - cur_vertex->n) * 
						state->step * m_fps;
			glNormal3fv(inter_n.v);
			
			inter_xyz = cur_vertex->v + 
						(next_vertex->v - cur_vertex->v) * 
						state->step * m_fps;
			glVertex3fv(inter_xyz.v);
			cmd++;
		}
		while(--count);

		glEnd();
	}
}

bool CModelMD2::Load(char *path, CResourceManager* resman, bool loadtexture)
{
	FILE* f;
	std::string texpath;
	int i;
	BYTE* frame;
	md2frame_t* framehead;
	md2header_t header;
	vertex_t* vertex;
	// For anim setup
	std::list<anim_t>::iterator iter;
	std::list<anim_t> animlist;
	anim_t curanim;
	char* tmpname;
	char skinname[MAX_SKINNAME];

	Unload();
	
	texpath = CLynx::ChangeFileExtension(path, "tga");
	if(loadtexture) // Server muss nicht die Textur laden (oder?)
		m_tex = resman->GetTexture((char*)texpath.c_str());

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
		fprintf(stderr, "Skin: %s\n", skinname);
	}

	// vertices	
	m_vertices = new vertex_t[header.num_frames * header.num_xyz];
	if(!m_vertices)
	{
		fprintf(stderr, "MD2: Out of memory for vertices: %i\n", header.num_frames * header.num_xyz);
		goto loaderr;
	}
	m_vertices_per_frame = header.num_xyz;

	// glcmds
	assert(sizeof(int) == 4);
	m_glcmds = new int[header.num_glcmds];
	if(!m_glcmds)
	{
		fprintf(stderr, "MD2: Out of memory for glcmds size: %i\n", header.num_glcmds);
		goto loaderr;
	}
	m_glcmdcount = header.num_glcmds;
	fseek(f, header.ofs_glcmds, SEEK_SET);
	if(fread(m_glcmds, sizeof(DWORD), m_glcmdcount, f) != m_glcmdcount)
	{
		fprintf(stderr, "MD2: Failed to read glcmds from file\n");
		goto loaderr;		
	}

	// frames
	frame = new BYTE[header.framesize];
	if(!frame)
	{
		fprintf(stderr, "MD2: Out of memory for frame buffer: %i\n", header.framesize);
		goto loaderr;
	}
	framehead = (md2frame_t*)frame;
	m_frames = new frame_t[header.num_frames];
	if(!m_frames)
	{
		fprintf(stderr, "MD2: Out of memory for frames: %i\n", header.num_frames);
		goto loaderr;
	}
	m_framecount = header.num_frames;

	fseek(f, header.ofs_frames, SEEK_SET);
	curanim.name[0] = NULL;
    m_min = vec3_t::origin;
    m_max = vec3_t::origin;
	for(i=0;i<header.num_frames;i++) // iterating through every frame
	{
		m_frames[i].num_xyz = header.num_xyz;

		if(fread(frame, 1, header.framesize, f) != header.framesize) // read complete frame to memory
		{
			fprintf(stderr, "MD2: Error reading frame from model\n");
			goto loaderr;
		}
		strcpy(m_frames[i].name, framehead->name);
		if(tmpname = strtok(framehead->name, "0123456789")) // keep track of animation sequence name
		{
			if(strcmp(tmpname, curanim.name))
			{
				if(curanim.name[0] != NULL)
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
			for(int m=0;m<3;m++)
			{
				if(vertex->v.v[m] < m_min.v[m])
					m_min.v[m] = vertex->v.v[m];
				else if(vertex->v.v[m] > m_max.v[m])
					m_max.v[m] = vertex->v.v[m];
			}

			vertex->n.z = -(g_bytedirs[framehead->v[j].n % NUMVERTEXNORMALS][0]);
			vertex->n.x = g_bytedirs[framehead->v[j].n % NUMVERTEXNORMALS][1];
			vertex->n.y = g_bytedirs[framehead->v[j].n % NUMVERTEXNORMALS][2];
			if(framehead->v[j].n >= NUMVERTEXNORMALS)
				fprintf(stderr, "MD2: Vertexnormal out of bounds. Frame: %i, vertex: %i\n", i, j);
		}
	}
	if(curanim.name[0] != NULL)
	{
		curanim.end = i-1;
		animlist.push_back(curanim);
	}
	delete[] frame;
	fclose(f);
	f = NULL;

	m_anims = new anim_t[animlist.size()];
	if(!m_anims)
	{
		fprintf(stderr, "MD2: Failed to create animation table - out of memory\n");
		goto loaderr;
	}
	m_animcount = (int)animlist.size();
	i = 0;
	for(iter=animlist.begin();iter!=animlist.end();iter++)
	{
		m_anims[i] = (*iter);
		/*
		fprintf(stderr, "Animation: %s (%i-%i)\n", 
					m_anims[i].name, 
					m_anims[i].start,
					m_anims[i].end);
		*/
		i++;
	}

	m_center = (m_max - m_min)*0.5f;
	m_sphere = m_center.Abs()*0.5f;

	fprintf(stderr, "MD2: Model %s: (Sphere: %.2f)\n", path, m_sphere);

	return true;
loaderr:
	if(f)
		fclose(f);
	Unload();
	return false;
}

void CModelMD2::GetCenter(vec3_t* center) const
{
	*center = m_center;
}

void CModelMD2::Unload()
{
	if(m_frames)
	{
		delete[] m_frames;
		m_frames = NULL;
	}
	m_framecount = 0;

	if(m_vertices)
	{
		delete[] m_vertices;
		m_vertices = NULL;
	}
	m_vertices_per_frame = 0;

	if(m_anims)
	{
		delete[] m_anims;
		m_anims = NULL;
	}
	m_animcount = 0;

	if(m_glcmds)
	{
		delete[] m_glcmds;
		m_glcmds = NULL;
	}
	m_glcmdcount = 0;

	m_sphere = 0.0f;
	m_tex = 0;
}

void CModelMD2::Animate(md2_state_t* state, float dt) const
{
    if(state->stop_anim)
        return;

	state->step += dt;
	if(state->step >= m_invfps)
	{
        state->cur_frame++;
		state->step = 0.0f;

        if(state->cur_frame >= m_anims[state->cur_anim].end) // animation ist 1x durchgelaufen
        {
            if(state->next_anim != state->cur_anim) // soll keine schleife gespielt werden?
            {
                if(state->next_anim >= 0) // es soll die nächste animation 1x gespielt werden
                {
                    SetAnimation(state, state->next_anim);
                    return;
                }
                else
                {
                    state->cur_frame = m_anims[state->cur_anim].end;
                    StopAnimation(state);
                    return;
                }
            }
            state->cur_frame = GetNextFrameInAnim(state, 0); // wieder auf anfang zurücksetzen
        }
	}
}

int	CModelMD2::GetNextFrameInAnim(const md2_state_t* state, int increment) const
{
    if(state->stop_anim)
        return state->cur_anim; // Angehalten

	int size = m_anims[state->cur_anim].end - m_anims[state->cur_anim].start + 1;
	int step = state->cur_frame - m_anims[state->cur_anim].start + increment;
	return m_anims[state->cur_anim].start + (step % size);
}

bool CModelMD2::SetAnimation(md2_state_t* state, int i) const
{
	if(i < 0 || i >= m_animcount)
	{
		assert(0); // unknown animation
		return false;
	}

	state->cur_anim = i;
	state->cur_frame = m_anims[i].start;
	state->step = 0.0f;
    state->stop_anim = 0;
	return true;
}

bool CModelMD2::SetNextAnimation(md2_state_t* state, int i) const
{
    state->next_anim = i;
    return true;
}

bool CModelMD2::SetAnimationByName(md2_state_t* state, const char* name) const
{
	int i = FindAnimation(name);
	return SetAnimation(state, i);
}

bool CModelMD2::SetNextAnimationByName(md2_state_t* state, const char* name) const
{
	int i = FindAnimation(name);
	return SetNextAnimation(state, i);
}

int CModelMD2::FindAnimation(const char* name) const
{
	if(strcmp(name, "default")==0)
		return 0;
	for(int i=0;i<m_animcount;i++)
	{
		if(strcmp(m_anims[i].name, name)==0)
			return i;
	}
	assert(0);
	return 0;
}

float CModelMD2::GetSphere() const
{
	return m_sphere;
}

void CModelMD2::GetAABB(vec3_t* min, vec3_t* max) const
{
	*min = m_min;
	*max = m_max;
}

void CModelMD2::SetFPS(float fps)
{
	m_fps = fps;
	m_invfps = 1/fps;
}

float CModelMD2::GetFPS() const
{
	return m_fps;
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