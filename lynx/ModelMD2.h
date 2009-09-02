#pragma once

#include "math/vec3.h"

struct frame_t;
struct anim_t;
struct vertex_t;

class CModelMD2;
#include "ResourceManager.h"

// Animationssystem in md2_state_t:
// Auf md2_state_t sollte über die CModelMD2 Methoden zugegriffen werden:
// StopAnimation
// SetAnimation
// SetNextAnimation
// SetAnimationByName
// SetNextAnimationByName

// Innere Funktionsweise von cur_anim und next_anim
// Schleife: next_anim == cur_anim (z.B. für "idle" Animation)
// 1x eine andere animation: z.B. cur_anim = jump und next_anim = default. Ergibt: 1x jump-Animation, danach wieder idle
// Einmal spielen, dann anhalten: cur_anim = die und next_anim = -1. Ergibt: Todesanimation wird einmal gespielt, danach wird die Animation angehalten

struct md2_state_t
{
	md2_state_t()
	{
		cur_anim = 0;
		cur_frame = 0;
        next_anim = 0;
        stop_anim = 0;
		step = 0.0f;
	};
	int cur_anim;
	int cur_frame;
    int next_anim;
    int stop_anim; // Animation anhalten
	float step;
};

class CModelMD2
{
public:
	CModelMD2(void);
	~CModelMD2(void);

	bool	Load(char *path, CResourceManager* resman, bool loadtexture=true);
	void	Unload();

	void	Render(const md2_state_t* state) const;
	void	Animate(md2_state_t* state, float dt) const;

    void    StopAnimation(md2_state_t* state) const { state->stop_anim = 1; }
	bool	SetAnimation(md2_state_t* state, int i) const;
	bool	SetNextAnimation(md2_state_t* state, int i) const;
	bool	SetAnimationByName(md2_state_t* state, const char* name) const;
    bool    SetNextAnimationByName(md2_state_t* state, const char* name) const;
    int		FindAnimation(const char* name) const; // -1 on error
	void	SetFPS(float fps);
	float	GetFPS() const;
    static int GetAnimation(const md2_state_t* state) { return state->cur_anim; }
    static int GetNextAnimation(const md2_state_t* state) { return state->next_anim; }

	float	GetSphere() const;
	void	GetAABB(vec3_t* min, vec3_t* max) const;
	void	GetCenter(vec3_t* center) const;

private:
	int GetNextFrameInAnim(const md2_state_t* state, int increment) const;

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
