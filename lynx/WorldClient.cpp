#include "WorldClient.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#define MAX_CLIENT_HISTORY          500
#define RENDER_DELAY                100

#pragma warning(disable: 4355)
CWorldClient::CWorldClient(void) : m_ghostobj(this)
{
	m_localobj = &m_ghostobj;
	m_ghostobj.SetOrigin(vec3_t(0,1.0f,0));
	m_pinterpworld = this;
}

CWorldClient::~CWorldClient(void)
{
}

CObj* CWorldClient::GetLocalObj() const
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

void CWorldClient::Update(const float dt, const DWORD ticks)
{
	CWorld::Update(dt, ticks);
    
	static int pressed = 0;
	static vec3_t location;

	if(CLynx::GetKeyState()[102])
	{
		float f = 999.9f;
		vec3_t forward;
		CObj* obj = GetLocalController();
		vec3_t::AngleVec3(obj->GetRot(), &forward, NULL, NULL);
		forward *= 1000.0f;
		m_bsptree.ClearMarks(m_bsptree.m_root);
        vec3_t start = obj->GetOrigin()+obj->GetEyePos();
		m_bsptree.TraceRay(start, start + forward, &f, m_bsptree.m_root);
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
	ObjMove(controller, dt);

	if(m_pinterpworld == &m_interpworld)
	{
		if(m_interpworld.f >= 1.0)
			CreateClientInterp();
		m_interpworld.Update(dt, ticks);
	}
}

bool CWorldClient::Serialize(bool write, CStream* stream, const world_state_t* oldstate)
{
    bool changed = CWorld::Serialize(write, stream, oldstate);
    if(changed && !write)
	{
		worldclient_state_t clstate;
		clstate.state = GetWorldState();
		clstate.localtime = CLynx::GetTicks();
		m_history.push_front(clstate);
		assert(m_history.size() < 80);
		while(clstate.localtime - m_history.back().localtime > MAX_CLIENT_HISTORY)
			m_history.pop_back();
		while(m_history.size() > 80)
			m_history.pop_back();
		CreateClientInterp();
	}
    return changed;
}

/*
	Vorbereiten von Interpolierter Welt
 */
void CWorldClient::CreateClientInterp()
{
	m_pinterpworld = this;

	if(m_history.size() < 2)
        return;

    std::list<worldclient_state_t>::iterator iter = m_history.begin();
    const DWORD tlocal = CLynx::GetTicks(); // aktuelle zeit
    const DWORD tlocal_n = (*iter).localtime; // zeit von letztem packet
    const DWORD dtupdate = tlocal - tlocal_n; // zeit seit letztem update
    const DWORD rendertime = tlocal - RENDER_DELAY; // Zeitpunkt für den interpoliert werden soll

    if(dtupdate > RENDER_DELAY)
    {
        if(m_history.size() > 2)
			fprintf(stderr, "CWorldClient: Server lag, no update since %i ms.\n", dtupdate);
        return;
    }

	// Jetzt werden die beiden worldclient_state_t Objekte gesucht, die um den Renderzeitpunkt liegen
	// Wenn es das nicht gibt, muss extrapoliert werden

	// LINEARE INTERPOLATION

	std::list<worldclient_state_t>::iterator state1 = iter; // worldstate vor rendertime
	std::list<worldclient_state_t>::iterator state2 = iter; // worldstate nach rendertime
	for(iter = m_history.begin();iter != m_history.end();iter++)
	{
		if((*iter).localtime < rendertime)
		{
			state1 = iter;
			break;
		}
		else
			state2 = iter;
	}
	if((*state1).localtime > rendertime)
	{
		return;
	}
	if(state1 == m_history.end() || state2 == m_history.end() || state1 == state2)
	{
		assert(0);
		return;
	}

	worldclient_state_t w1 = (*state1);
	worldclient_state_t w2 = (*state2);
	assert(w1.localtime < rendertime && w2.localtime >= rendertime);
	
	//m_interpworld.DeleteAllObjs();
	m_interpworld.m_pbsp = &m_bsptree; // FIXME ist das sicher bei einem level change?
	m_interpworld.m_presman = &m_resman;
	m_interpworld.state1 = w1;
	m_interpworld.state2 = w2;

	CObj* obj;
	std::map<int, int>::iterator objiter;
	for(objiter =  w1.state.objindex.begin();
		objiter != w1.state.objindex.end(); objiter++)
    {
		int id = (*objiter).first;

		// Prüfen, ob Obj auch in w2 vorkommt
		if(w2.state.objindex.find(id) == w2.state.objindex.end())
		{
			//assert(0); // OK soweit?
            // Könnte man hier gleich löschen und die Schleife am Ende sparen?
			continue;
		}

		const obj_state_t objstate = w1.state.objstates[(*objiter).second];
        obj = m_interpworld.GetObj(id);
        if(!obj)
        {
            obj = new CObj(&m_interpworld);
		    obj->SetObjState(&objstate, id);
		    m_interpworld.AddObj(obj);
        }
        else
        {
            obj->SetObjState(&objstate, id);
        }
    }
    // Objekte löschen, die es jetzt nicht mehr gibt
    OBJITER deliter;
    for(deliter = m_interpworld.ObjBegin();deliter != m_interpworld.ObjEnd(); deliter++)
    {
        obj = (*deliter).second;
        if(w1.state.objindex.find(obj->GetID()) == w1.state.objindex.end())
            m_interpworld.DelObj(obj->GetID());
    }

	m_interpworld.UpdatePendingObjs();
	m_pinterpworld = &m_interpworld;
}

void CWorldInterp::Update(const float dt, const DWORD ticks)
{
	const DWORD tlocal = ticks;
    const DWORD rendertime = tlocal - RENDER_DELAY; // Zeitpunkt für den interpoliert werden soll
	const DWORD updategap = state2.localtime - state1.localtime;

	const float a = (float)(rendertime - state1.localtime);
	f = a/updategap;

	if(f > 1.5f)
	{
		fprintf(stderr, "Extrapolation factor > 1.5\n");
		return;
	}

	std::map<int, int>::iterator iter1, iter2;
	OBJITER iter;
	CObj* obj;
	vec3_t origin1, origin2, origin;
	vec3_t rot1, rot2, rot;
	for(iter = ObjBegin();iter != ObjEnd(); iter++)
	{
		obj = (*iter).second;
		iter1 = state1.state.objindex.find(obj->GetID());
		iter2 = state2.state.objindex.find(obj->GetID());
		origin1 = state1.state.objstates[(*iter1).second].origin;
		origin2 = state2.state.objstates[(*iter2).second].origin;
		rot1 = state1.state.objstates[(*iter1).second].rot;
		rot2 = state2.state.objstates[(*iter2).second].rot;
		
		origin = vec3_t::Lerp(origin1, origin2, f);
		rot = vec3_t::Lerp(rot1, rot2, f);
		obj->SetOrigin(origin);
		obj->SetRot(rot);
		obj->UpdateMatrix();
	}
}
