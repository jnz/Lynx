#include <assert.h>
#include "math/mathconst.h"
#include "World.h"
#include <math.h>

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CWorld::CWorld(void)
{

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

void CWorld::Update(const float dt)
{
	OBJITER iter;
	CObj* obj;

	UpdatePendingObjs();

	if(!m_bsptree.m_root)
		return;

	for(iter = m_objlist.begin();iter!=m_objlist.end();iter++)
	{
		obj = (*iter).second;
		obj->pos.velocity.SetLength(obj->GetSpeed());
		assert(obj->GetSpeed() > 0);
		ObjCollision(obj, dt);
	}
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
void CWorld::ObjCollision(CObj* obj, float dt)
{
	obj->pos.origin += obj->pos.velocity * dt;
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

int CWorld::Serialize(bool write, CStream* stream)
{
	int size = 0;
	CObj* obj;

	if(write)
	{
		OBJITER iter;
		WORD reqsize;

		if(stream)
		{
			stream->WriteDWORD((DWORD)GetObjCount());
			stream->WriteString(m_bsptree.GetFilename());
		}
		size += sizeof(DWORD);
		size += (int)CStream::StringSize(m_bsptree.GetFilename());

		for(iter = m_objlist.begin();iter!=m_objlist.end();iter++)
		{
			obj = (*iter).second;
			reqsize = obj->Serialize(true, NULL);
			size += reqsize+sizeof(WORD); // +2 für reqsize
			assert(reqsize <= USHRT_MAX);
			if(stream)
			{
				stream->WriteWORD(reqsize);
				obj->Serialize(true, stream);
			}
		}
	}
	else
	{
		assert(stream);
		DWORD objcount;
		WORD objsize, objsizeread;
		std::string level;

		DeleteAllObjs();

		stream->ReadDWORD(&objcount);
		stream->ReadString(&level);
		fprintf(stderr, "Obj count: %i\n", objcount);
		if(level != "")
			if(m_bsptree.Load(level)==false)
			{
				// FIXME error handling
				return 0;
			}

		while(stream->GetBytesToRead() > 0)
		{
			stream->ReadWORD(&objsize);
			assert(stream->GetBytesToRead() >= objsize);
			if(stream->GetBytesToRead() >= objsize)
			{
				obj = new CObj(this);
				objsizeread = obj->Serialize(false, stream);
				assert(objsizeread == objsize);
				if(objsizeread == objsize)
					AddObj(obj);
				else
				{
					fprintf(stderr, "Failed to serialize object from stream\n");
					delete obj;

					DeleteAllObjs();
					m_bsptree.Unload();
					// FIXME error handling
					return 0;
				}
			}
		}
		assert(m_addobj.size() == objcount);
		if(m_addobj.size() != objcount)
		{
			DeleteAllObjs();
			m_bsptree.Unload();
			// FIXME error handling
			return 0;
		}
		UpdatePendingObjs();
	}

	return size;
}

int CWorld::SerializePositions(bool write, CStream* stream, int objpvs, int mtu, int* lastobj)
{
	CObj* obj;
	int written = stream->GetBytesWritten();
	WORD objsize;
	int objs = 0;

	assert(stream);

	if(write)
	{
		OBJITER iter;
		
		assert(lastobj);

		iter = (*lastobj) ? m_objlist.find(*lastobj) : m_objlist.begin();
		for(;iter!=m_objlist.end();iter++)
		{
			obj = (*iter).second;
			objsize = obj->pos.Serialize(true, NULL);
			if(stream->GetBytesWritten()-written + 
				objsize + (int)sizeof(INT32)+(int)sizeof(WORD) > mtu)
			{
				*lastobj = obj->GetID();
				return objs;
			}

			assert(obj->GetID() <= INT_MAX);
			if(obj->GetID() > INT_MAX)
				continue;
			stream->WriteInt32((INT32)obj->GetID());
			stream->WriteWORD(objsize);
			obj->pos.Serialize(true, stream);
			objs++;
		}
		*lastobj = -1;
	}
	else
	{
		INT32 objid;
		int read;

		while(stream->GetBytesToRead() > 3)
		{
			objid = 0;
			stream->ReadInt32(&objid);
			stream->ReadWORD(&objsize);
			obj = GetObj(objid);
			//assert(obj);
			if(!obj)
				return -1;
			read = obj->pos.Serialize(false, stream);
			assert(read == objsize);
			if(read != objsize)
				return -1;
			objs++;
		}		
	}

	return objs;
}
