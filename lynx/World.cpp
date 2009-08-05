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

#define MAX_CLIP_PLANES		5
void CWorld::ObjMove(CObj* obj, float dt)
{
    obj->SetOrigin(obj->GetOrigin() + obj->GetVel() * dt);
	/*
	vec3_t planes[MAX_CLIP_PLANES];
	vec3_t newpos;
	vec3_t original_velocity, primal_velocity, new_velocity;
	bsp_trace_t trace;
	int numbumps = 4;
	int bumpcount;
	int numplanes=0, i, j;
	vec3_t min, max;

	obj->GetAABB(&min, &max);
	original_velocity = obj->pos.velocity;
	primal_velocity = obj->pos.velocity;

	for(bumpcount=0;bumpcount<numbumps;bumpcount++)
	{
		newpos = obj->pos.origin + obj->pos.velocity * dt;
		if(newpos == obj->pos.origin)
			return;

		m_bsptree.TraceBBox(obj->pos.origin, newpos, min, max, &trace);

		if(trace.f < 0)
		{
			fprintf(stderr, "Phys: Negative trace %.2f\n", trace.f);
			obj->pos.origin = trace.endpoint;
			break;
		}
		if(trace.f >= 0)
		{
			obj->pos.origin = trace.endpoint;
			original_velocity = obj->pos.velocity;
			numplanes = 0;
		}

		if(trace.f == 1.0f)
			break;

		dt -= dt * trace.f;

		if(numplanes >= MAX_CLIP_PLANES)
		{
			assert(0); // this shouldn't really happen
			obj->pos.velocity = vec3_t::origin;
			break;
		}

		planes[numplanes] = trace.p.m_n;
		numplanes++;
		
		// modify original_velocity so it parallels all of the clip planes
		for(i=0;i<numplanes;i++)
		{
			ClipVelocity(original_velocity, planes[i], &new_velocity, 1);

			for(j=0;j<numplanes;j++)
			{
				if((j != i) && planes[i] != planes[j])
				{
					if(new_velocity * planes[j] < 0)
						break;
				}
			}
			if(j == numplanes)
				break;
		}

		if(i != numplanes)
		{
			obj->pos.velocity = new_velocity;
		}
		else
		{
			if(numplanes != 2)
			{
				obj->pos.velocity = vec3_t::origin;
				return;
			}
			vec3_t dir = planes[0] ^ planes[1];
			float d = dir * obj->pos.velocity;
			obj->pos.velocity = dir * d;
		}
		
		if(obj->pos.velocity * primal_velocity <= 0.0f)
		{
			obj->pos.velocity = vec3_t::origin;
			return;
		}
	}
	*/
}
/*
		plane_t p = trace.p;
		trace.p.m_d += trace.offset;
		slide =		trace.end +
					trace.p.m_n * -trace.p.GetDistFromPlane(trace.end);*/

void CWorld::UpdatePendingObjs()
{
	if(m_removeobj.size() > 0)
	{
		// Zu löschende Objekte entfernen
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
		// Objekte für das Frame hinzufügen
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

#define WORLD_STATE_NO_REAL_CHANGE	(WORLD_STATE_WORLDID|WORLD_STATE_LEVELTIME) // worldid und leveltime ändern sich sowieso immer
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
        CStream tempstream(16384); // FIXME, das geht eleganter, ohne den tempstream. würde ein memcpy sparen wenn man den platz für die updateflags spart und hinterher mit dem korrekten wert füllt

        // Zuerst in tempstream schreiben um gleichzeitig die updateflags zu erkennen
        DeltaDiffDWORD(&state.worldid, oldstate ? &oldstate->worldid : NULL, WORLD_STATE_WORLDID, &updateflags, &tempstream);
        DeltaDiffDWORD(&state.leveltime, oldstate ? &oldstate->leveltime : NULL, WORLD_STATE_LEVELTIME, &updateflags, &tempstream);
        DeltaDiffString(&state.level, oldstate ? &oldstate->level : NULL, WORLD_STATE_LEVEL, &updateflags, &tempstream);
		// [NEUE ATTRIBUTE HIER]

		if(updateflags > WORLD_STATE_NO_REAL_CHANGE)
			changes++;
		
        stream->WriteDWORD(updateflags); // Jetzt kennen wir die Updateflags und können sie in den tatsächlichen stream schreiben
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
        std::map<int,int> objread; // objekte die gelesen wurden, was hier nicht steht, muss gelöscht werden
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
        // Alle Objekte löschen, die in dem neuen State nicht mehr vorhanden sind
        for(iter=m_objlist.begin();iter!=m_objlist.end();iter++)
        {
            obj = (*iter).second;
            if(objread.find(obj->GetID()) != objread.end())
                continue;
            //assert(0); // nur mal zum sehen, ob hier alles klappt. danach zeile löschen
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
