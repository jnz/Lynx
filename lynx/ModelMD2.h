#pragma once

#include "math/vec3.h"

struct frame_t;
struct anim_t;
struct vertex_t;

class CModelMD2;
#include "ResourceManager.h"

struct md2_state_t
{
	md2_state_t()
	{
		cur_anim = 0;
		cur_frame = 0;
		step = 0.0f;
	};
	int cur_anim;
	int cur_frame;
	float step;
};

class CModelMD2
{
public:
	CModelMD2(void);
	~CModelMD2(void);

	bool	Load(char *path, CResourceManager* resman);
	void	Unload();

	void	Render(md2_state_t* state);
	void	Animate(md2_state_t* state, float dt);

	bool	SetAnimation(md2_state_t* state, int i);
	bool	SetAnimationByName(md2_state_t* state, char* name);
	int		FindAnimation(char* name); // -1 on error
	void	SetFPS(float fps);
	float	GetFPS();

	float	GetSphere();
	void	GetAABB(vec3_t* min, vec3_t* max);
	void	GetCenter(vec3_t* center);

private:
	int GetNextFrameInAnim(md2_state_t* state);

	frame_t*		m_frames;
	int				m_framecount;
	vertex_t*		m_vertices;
	int				m_vertices_per_frame;
	anim_t*			m_anims;
	int				m_animcount;
	int*			m_glcmds;
	int				m_glcmdcount;

	float			m_fps;
	float			m_invfps; // 1/fps

	vec3_t			m_center;
	float			m_sphere;
	vec3_t			m_min, m_max;

	unsigned int	m_tex;
};
