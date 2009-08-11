#include <assert.h>
#include "math/mathconst.h"
#include "World.h"
#include <math.h>
#include <list>

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#pragma warning(disable:4355)
CWorld::CWorld(void) : m_resman(this)
{
    state.worldid = 0;
    m_leveltimestart = CLynx::GetTicks();
    state.leveltime = 0;
}

CWorld::~CWorld(void)
{
	DeleteAllObjs();
}

void CWorld::AddObj(CObj* obj, bool inthisframe)
{
	assert(obj && !GetObj(obj->GetID()));
	assert(GetObjCount() < USHRT_MAX);

	if(inthisframe)
		m_objlist[obj->GetID()] = obj;
	else
		m_addobj.push_back(obj);
}

void CWorld::DelObj(int objid)
{
	assert(GetObj(objid));
	if(GetObj(objid))
		m_removeobj.push_back(objid);
}

CObj* CWorld::GetObj(int objid)
{
	OBJITER iter;
	iter = m_objlist.find(objid);
	if(iter == m_objlist.end())
		return NULL;
	return (*iter).second;
}

void CWorld::DeleteAllObjs()
{
	OBJITER iter;

	for(iter = m_objlist.begin();iter!=m_objlist.end();iter++)
		delete (*iter).second;
	m_objlist.clear();
}

void CWorld::Update(const float dt, const DWORD ticks)
{
    if(!IsClient())
    {
        state.leveltime = ticks - m_leveltimestart;
        state.worldid++;
    }

	UpdatePendingObjs();

	if(!m_bsptree.m_root)
		return;

	m_bsptree.ClearMarks(m_bsptree.m_root);
}

#define STOP_EPSILON		(0.1f)
void ClipVelocity(const vec3_t& in, const vec3_t& normal, vec3_t* out, float overbounce)
{
	float backoff;
	float change;
	
	backoff = (in * normal) * overbounce;

	for(int i=0;i<3;i++)
	{
		change = normal.v[i] * backoff;
		out->v[i] = in.v[i] - change;
		if(out->v[i] > -STOP_EPSILON && out->v[i] < STOP_EPSILON)
			out->v[i] = 0.0f;
	}
}

#define MAX_CLIP_PLANES		8
void CWorld::ObjMove(CObj* obj, float dt)
{
    bsp_sphere_trace_t trace;
    vec3_t p1 = obj->GetOrigin();
    vec3_t p2 = obj->GetOrigin() + obj->GetVel() * dt;
    vec3_t q;
    vec3_t p3;
    vec3_t vel = obj->GetVel();
    trace.radius = obj->GetRadius();

    for(int i=0;(p2 - p1).AbsSquared()>lynxmath::EPSILON && i < MAX_CLIP_PLANES; i++)
    {
        trace.start = p1;
        trace.end = p2;
        trace.dir = trace.end - trace.start;
        trace.f = MAX_TRACE_DIST;
        GetBSP()->TraceSphere(&trace);
        assert(trace.f > 0.0f);
        if(trace.f < 1.0f)
        {
            q = trace.start + trace.f*trace.dir + 
                trace.p.m_n * 1000 * lynxmath::EPSILON;
            p3 = p2 -((p2 - q)*trace.p.m_n)*trace.p.m_n;
            vel = vel - 2*(vel*trace.p.m_n)*trace.p.m_n;
            p1 = q;
            p2 = p3;
        }
        else
            break;
    }

    obj->SetOrigin(p2);
    obj->SetVel(vel); // bounce

/*
    const bool moved = obj->GetVel() != vec3_t::origin;
    if(moved && m_bsptree.m_root)
    {
        vec3_t q;
        bsp_sphere_trace_t trace;

        trace.start = obj->GetOrigin();
        trace.radius = obj->GetRadius();
//        do
//        {
            trace.end = p2;
            trace.dir = trace.end - trace.start;
            trace.f = MAX_TRACE_DIST;
            m_bsptree.TraceSphere(&trace, m_bsptree.m_root);
            assert(trace.f > 0.0f);
            if(trace.f < 1.0f)
            {
                q = trace.start + trace.f*trace.dir + trace.p.m_n * lynxmath::EPSILON;
                //p2 = p2 -((p2 - q)*trace.p.m_n)*trace.p.m_n;
                p2 = q;
            }

//        }while(trace.f > lynxmath::EPSILON && trace.f < 1.0f);
    }
    obj->SetOrigin(p2);
    */
}

void CWorld::UpdatePendingObjs()
{
	if(m_removeobj.size() > 0)
	{
		// Zu l�schende Objekte entfernen
		std::list<int>::iterator remiter;
		OBJITER iter;
		for(remiter=m_removeobj.begin();remiter!=m_removeobj.end();remiter++)
		{
			iter = m_objlist.find((*remiter));
			assert(iter != m_objlist.end());
			delete (*iter).second;
			m_objlist.erase(iter);
		}
		m_removeobj.clear();
	}

	if(m_addobj.size() > 0)
	{
		// Objekte f�r das Frame hinzuf�gen
		std::list<CObj*>::iterator additer;
		for(additer=m_addobj.begin();additer!=m_addobj.end();additer++)
		{
			m_objlist[(*additer)->GetID()] = (*additer);
		}
		m_addobj.clear();
	}
}

bool CWorld::LoadLevel(const std::string path)
{
    bool success = m_bsptree.Load(path);
    if(success)
        state.level = path;
    return success;
}

// DELTA COMPRESSION CODE (ugly atm) ------------------------------------

#define WORLD_STATE_WORLDID			(1 <<  0)
#define WORLD_STATE_LEVELTIME		(1 <<  1)
#define WORLD_STATE_LEVEL			(1 <<  2)

#define WORLD_STATE_NO_REAL_CHANGE	(WORLD_STATE_WORLDID|WORLD_STATE_LEVELTIME) // worldid und leveltime �ndern sich sowieso immer
#define WORLD_STATE_FULLUPDATE		((1 <<  3)-1)

bool CWorld::Serialize(bool write, CStream* stream, const world_state_t* oldstate)
{
    assert(stream);
	int size = 0;
	CObj* obj;
    OBJITER iter;
	int changes = 0;

	if(write)
	{
        DWORD updateflags = 0;
        CStream tempstream(16384); // FIXME, das geht eleganter, ohne den tempstream. w�rde ein memcpy sparen wenn man den platz f�r die updateflags spart und hinterher mit dem korrekten wert f�llt

        // Zuerst in tempstream schreiben um gleichzeitig die updateflags zu erkennen
        DeltaDiffDWORD(&state.worldid, oldstate ? &oldstate->worldid : NULL, WORLD_STATE_WORLDID, &updateflags, &tempstream);
        DeltaDiffDWORD(&state.leveltime, oldstate ? &oldstate->leveltime : NULL, WORLD_STATE_LEVELTIME, &updateflags, &tempstream);
        DeltaDiffString(&state.level, oldstate ? &oldstate->level : NULL, WORLD_STATE_LEVEL, &updateflags, &tempstream);
		// [NEUE ATTRIBUTE HIER]

		if(updateflags > WORLD_STATE_NO_REAL_CHANGE)
			changes++;
		
        stream->WriteDWORD(updateflags); // Jetzt kennen wir die Updateflags und k�nnen sie in den tats�chlichen stream schreiben
        stream->WriteStream(tempstream);

        assert(oldstate ? 1 : (updateflags == WORLD_STATE_FULLUPDATE)); // muss zwingend eingehalten werden

        // Alle Objekte schreiben
        assert(GetObjCount() < USHRT_MAX);
        stream->WriteWORD((WORD)GetObjCount());

        obj_state_t* obj_oldstate;
		for(iter = m_objlist.begin();iter!=m_objlist.end();iter++)
		{
			obj = (*iter).second;
            obj_oldstate = NULL;
            if(oldstate) // Delta Compression
            {
                std::map<int,int>::iterator indexiter;
				std::map<int, int>* pmap = (std::map<int, int>*)&oldstate->objindex;
				indexiter = pmap->find(obj->GetID());
                if(indexiter != oldstate->objindex.end())
                    obj_oldstate = (obj_state_t*)&oldstate->objstates[(*indexiter).second];
            }
            stream->WriteWORD(obj->GetID()); // FIXME wird damit doppelt geschrieben
            if(obj->Serialize(true, stream, obj_oldstate))
				changes++;
		}
	}
	else
	{
        DWORD updateflags;
        DWORD worldid;
        std::string level;
        WORD objcount;
        WORD objid;

        stream->ReadDWORD(&updateflags);
        assert(updateflags > 0);
        assert(updateflags & WORLD_STATE_WORLDID);
        if(updateflags & WORLD_STATE_WORLDID)
            stream->ReadDWORD(&worldid);
		else
			return false;
        if(worldid < state.worldid)
        {
            assert(0); // anschauen, ob ok
            return false;
        }
        state.worldid = worldid;
        if(updateflags & WORLD_STATE_LEVELTIME)
            stream->ReadDWORD(&state.leveltime);
        if(updateflags & WORLD_STATE_LEVEL)
        {
            stream->ReadString(&level);
            assert(level.size() > 0);
            if(level != m_bsptree.GetFilename())
            {
                if(m_bsptree.Load(level)==false)
			    {
    				// FIXME error handling
					assert(0);
				    return false;
			    }
            }
        }
		// [NEUE ATTRIBUTE HIER]
		if(updateflags > WORLD_STATE_NO_REAL_CHANGE)
			changes++;

        stream->ReadWORD(&objcount);
        assert(objcount < USHRT_MAX);
        std::map<int,int> objread; // objekte die gelesen wurden, was hier nicht steht, muss gel�scht werden
        for(int i=0;i<objcount;i++)
        {
            stream->ReadWORD(&objid);
            obj = GetObj(objid);
            if(!obj)
            {
                obj = new CObj(this);
				changes += obj->Serialize(false, stream) ? 1 : 0;
                AddObj(obj);
            }
            else
				changes += obj->Serialize(false, stream) ? 1 : 0;
            objread[obj->GetID()] = obj->GetID();
        }
        // Alle Objekte l�schen, die in dem neuen State nicht mehr vorhanden sind
        for(iter=m_objlist.begin();iter!=m_objlist.end();iter++)
        {
            obj = (*iter).second;
            if(objread.find(obj->GetID()) != objread.end())
                continue;
            //assert(0); // nur mal zum sehen, ob hier alles klappt. danach zeile l�schen
            DelObj(obj->GetID());
        }

		UpdatePendingObjs();
	}

	return changes > 0;
}

world_state_t CWorld::GenerateWorldState()
{
	world_state_t worldstate = state;

    OBJITER iter;
	for(iter = m_objlist.begin();iter!=m_objlist.end();iter++)
	{
		CObj* obj = (*iter).second;
        worldstate.objstates.push_back(obj->GetState());
        worldstate.objindex[obj->GetID()] = (int)worldstate.objstates.size()-1;
    }

	return worldstate;
}
