#include "WorldClient.h"
#include "ServerClient.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#define MAX_CLIENT_HISTORY          500

#pragma warning(disable: 4355)
CWorldClient::CWorldClient(void) : m_ghostobj(this)
{
	m_localobj = &m_ghostobj;
	m_ghostobj.SetOrigin(vec3_t(0,1000.0f,0));

	m_interpworld.m_pbsp = &m_bsptree; // FIXME ist das sicher bei einem level change?
	m_interpworld.m_presman = &m_resman;
    m_interpworld.state1.localtime = 0;
    m_interpworld.state2.localtime = 0;
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
        if(m_localobj->GetID() != id)
        {
            m_ghostobj.CopyObjStateFrom(obj);
        }else if((m_ghostobj.GetOrigin()-m_localobj->GetOrigin()).AbsSquared() >= MAX_SV_CL_POS_DIFF)
        {
            fprintf(stderr, "CL: Reset position\n");
            m_ghostobj.SetOrigin(m_localobj->GetOrigin());
        }
		if(obj)
			m_localobj = obj;
	}
}

void CWorldClient::Update(const float dt, const DWORD ticks)
{
	CWorld::Update(dt, ticks);
    
    CObj* controller = GetLocalController();
	ObjMove(controller, dt);

	if(m_interpworld.f >= 1.0f)
		CreateClientInterp();
    m_interpworld.Update(dt, ticks);
}

bool CWorldClient::Serialize(bool write, CStream* stream, const world_state_t* oldstate)
{
    bool changed = CWorld::Serialize(write, stream, oldstate);
    if(changed && !write)
        AddWorldToHistory();

    return changed;
}

void CWorldClient::AddWorldToHistory()
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
}

/*
	Vorbereiten von Interpolierter Welt
 */
void CWorldClient::CreateClientInterp()
{
	if(m_history.size() < 2)
        return;

    std::list<worldclient_state_t>::iterator iter = m_history.begin();
    const DWORD tlocal = CLynx::GetTicks(); // aktuelle zeit
    const DWORD tlocal_n = (*iter).localtime; // zeit von letztem packet
    const DWORD dtupdate = tlocal - tlocal_n; // zeit seit letztem update
    const DWORD rendertime = tlocal - RENDER_DELAY; // Zeitpunkt für den interpoliert werden soll

    if(dtupdate > RENDER_DELAY)
    {
        //if(m_history.size() > 2)
		//	fprintf(stderr, "CWorldClient: Server lag, no update since %i ms.\n", dtupdate);
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
	if(state1 == state2)
        return;
	if((*state1).localtime > rendertime)
	{
		return;
	}
	if(state1 == m_history.end() || state2 == m_history.end())
	{
        assert(0);
		return;
	}

	worldclient_state_t w1 = (*state1);
	worldclient_state_t w2 = (*state2);
	assert(w1.localtime < rendertime && w2.localtime >= rendertime);

	m_interpworld.state1 = w1;
	m_interpworld.state2 = w2;

	CObj* obj;
	WORLD_STATE_OBJITER objiter;
	for(objiter =  w1.state.ObjBegin();
		objiter != w1.state.ObjEnd(); objiter++)
    {
		int id = (*objiter).first;

		// Prüfen, ob Obj auch in w2 vorkommt
        if(!w2.state.ObjStateExists(id))
		{
			//assert(0); // OK soweit?
            // Könnte man hier gleich löschen und die Schleife am Ende sparen?
			continue;
		}

        obj_state_t objstate;
        w1.state.GetObjState(id, objstate);
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
        if(!w1.state.ObjStateExists(obj->GetID()) ||
           !w2.state.ObjStateExists(obj->GetID()))
            m_interpworld.DelObj(obj->GetID());
    }

	m_interpworld.UpdatePendingObjs();

    assert(m_interpworld.state1.state.GetObjCount() ==
           m_interpworld.state2.state.GetObjCount());
}

void CWorldInterp::Update(const float dt, const DWORD ticks)
{
	const DWORD tlocal = ticks;
    const DWORD rendertime = tlocal - RENDER_DELAY; // Zeitpunkt für den interpoliert werden soll
	const DWORD updategap = state2.localtime - state1.localtime;

    if(updategap < 1)
        return;

	const float a = (float)(rendertime - state1.localtime);
	f = a/updategap;

	if(f > 1.0f)
	{
		return;
	}

	std::map<int, int>::iterator iter1, iter2;
	OBJITER iter;
	CObj* obj;
	vec3_t origin1, origin2, origin;
	quaternion_t rot1, rot2, rot;
    obj_state_t obj1, obj2;
	for(iter = ObjBegin();iter != ObjEnd(); iter++)
	{
		obj = (*iter).second;
        assert(state1.state.ObjStateExists(obj->GetID()));
        assert(state2.state.ObjStateExists(obj->GetID())); // wenn das schief geht, muss geprüft werden, ob das objekt richtig in die interp world eingefügt wurde
        state1.state.GetObjState(obj->GetID(), obj1);
        state2.state.GetObjState(obj->GetID(), obj2);

		origin1 = obj1.origin;
		origin2 = obj2.origin;
		rot1 = obj1.rot;
		rot2 = obj2.rot;
		
		origin = vec3_t::Lerp(origin1, origin2, f);
        quaternion_t::Slerp(&rot, rot1, rot2, f);

        obj->SetOrigin(origin);
		obj->SetRot(rot);
	}
}
