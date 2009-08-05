#include "WorldClient.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#define MAX_CLIENT_HISTORY          3
#define RENDER_DELAY                100

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
    //ClientInterp();

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

bool CWorldClient::Serialize(bool write, CStream* stream, const world_state_t* oldstate)
{
    bool changed = CWorld::Serialize(write, stream, oldstate);
    /*
    if(!changed)
        return changed;

    worldclient_state_t clstate;
    clstate.state = GenerateWorldState();
    clstate.localtime = CLynx::GetTicks();
    m_history.push_front(clstate);
    if(m_history.size() > MAX_CLIENT_HISTORY)
        m_history.pop_back();
    */
    return changed;
}

void CWorldClient::ClientInterp()
{
    if(m_history.size() < 2)
        return;

    std::list<worldclient_state_t>::iterator iter = m_history.begin();
    const DWORD tlocal = CLynx::GetTicks(); // aktuelle zeit
    const DWORD tlocal_n = (*iter).localtime; // zeit von letztem packet
    const DWORD dtupdate = tlocal - tlocal_n; // zeit seit letztem update

    if(dtupdate > RENDER_DELAY)
    {
        fprintf(stderr, "CWorldClient: Server lag, no update since %i ms.\n", dtupdate);
        return;
    }

    worldclient_state_t staten = (*iter);
    iter++;
    worldclient_state_t staten1 = (*iter);

    const DWORD tlocal_n1 = (*iter).localtime; // zeit von vorletztem packet
    const DWORD dt = tlocal_n - tlocal_n1; // zeit zwischen letztem und vorletztem packet
    const DWORD rendertime = tlocal_n + dtupdate - RENDER_DELAY; // Zeitpunkt für den interpoliert werden soll
    const int a = rendertime - tlocal_n1;
    const float f = (float)a/dt; // lineare interpolation mit faktor f (f liegt zw. 0 und 1)
    
    OBJITER objiter;
    for(objiter = ObjBegin(); objiter != ObjEnd(); objiter++)
    {
        WORD id = ((*objiter).second)->GetID();

    }
}
