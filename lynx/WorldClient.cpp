#include "WorldClient.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#pragma warning(disable: 4355)
CWorldClient::CWorldClient(void) : m_ghostobj(this)
{
	m_localobj = &m_ghostobj;
	m_ghostobj.SetOrigin(vec3_t(0,8.0f,0));
	m_ghostobj.SetSpeed(50.0f);
}

CWorldClient::~CWorldClient(void)
{
}

CObj* CWorldClient::GetLocalObj()
{
	return m_localobj;
}

void CWorldClient::SetLocalObj(int id)
{
	CObj* obj;

	if(id == 0)
	{
		m_localobj = &m_ghostobj;
	}
	else
	{
		obj = GetObj(id);
		assert(obj);
		if(obj)
			m_localobj = obj;
	}
}

void CWorldClient::Update(const float dt)
{
	CWorld::Update(dt);

	static int pressed = 0;
	static vec3_t location;

	if(CLynx::GetKeyState()[104])
	{
		float f = 999.9f;
		vec3_t forward;
		CObj* obj = GetLocalController();
		vec3_t::AngleVec3(obj->GetRot(), &forward, NULL, NULL);
		forward *= 1000.0f;
		m_bsptree.ClearMarks(m_bsptree.m_root);

		bsp_trace_t trace;
		m_bsptree.TraceBBox(obj->GetOrigin(), 
							obj->GetOrigin() + forward, vec3_t(-1,-1,-1),
							vec3_t(1,1,1), &trace);
		if(!trace.allsolid)
		{
			location = trace.endpoint;
			pressed = 1;
		}
	}
	else
	{
		if(pressed)
		{
			pressed = 0;
			CObj* obj = new CObj(this);
			obj->SetOrigin(location);
			obj->UpdateMatrix();
			AddObj(obj);
		}
	}

    CObj* controller = GetLocalController();
    vec3_t vel = controller->GetVel();
    vel.SetLength(controller->GetSpeed());
    controller->SetVel(vel);
	ObjCollision(controller, dt);
}
