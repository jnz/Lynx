#include <assert.h>
#include "math/mathconst.h"
#include "World.h"
#include <math.h>
#include <list>
#include "lynxsys.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#pragma warning(disable:4355)
CWorld::CWorld(void) : m_resman(this)
{
    state.worldid = 0;
    m_leveltimestart = CLynxSys::GetTicks();
    state.leveltime = 0;
}

CWorld::~CWorld(void)
{
    Shutdown();
}

void CWorld::Shutdown()
{
    DeleteAllObjs();
    assert(m_objlist.size()==0);
    assert(m_addobj.size()==0);
    assert(m_removeobj.size()==0);
}

void CWorld::AddObj(CObj* obj, bool inthisframe)
{
    assert(obj && !GetObj(obj->GetID()));
    assert(GetObjCount() < INT_MAX);
    if(!obj)
        return;

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

    UpdatePendingObjs(); // clear pending queues
    for(iter = ObjBegin();iter!=ObjEnd();iter++)
        delete (*iter).second;
    m_objlist.clear();
}

const std::vector<CObj*> CWorld::GetNearObj(const vec3_t& origin, const float radius, const int exclude, const int type) const
{
    // FIXME this function should use some sort of binary space partitioning
    // naive implementation:
    std::vector<CObj*> objlist;
    const float radius2 = radius * radius;
    CObj* obj;
    OBJITERCONST iter;
    for(iter = m_objlist.begin();iter!=m_objlist.end();iter++)
    {
        obj = (*iter).second;
        if(obj->GetFlags() & OBJ_FLAGS_GHOST)
            continue;

        if(obj->GetID() != exclude && (obj->GetOrigin() - origin).AbsSquared() < radius2 &&
            ((obj->GetType() == type) || type < 0 ))
            objlist.push_back(obj);
    }
    return objlist;
}

// Returns objects within radius, where the obj type is in objtypes array
const std::vector<CObj*> CWorld::GetNearObjByTypeList(const vec3_t& origin,
                                                      const float radius,
                                                      const int exclude,
                                                      const std::vector<int>& objtypes) const
{
    // FIXME this function also should use some sort of binary space partitioning
    // naive implementation:
    std::vector<CObj*> objlist;
    OBJITERCONST iter;
    for(iter = m_objlist.begin(); iter != m_objlist.end(); iter++)
    {
        CObj* obj = (*iter).second;
        if(obj->GetFlags() & OBJ_FLAGS_GHOST) // ignore ghosts always
            continue;

        if( ( obj->GetID() == exclude ) ||
            ( (obj->GetOrigin() - origin).AbsFast() > radius + obj->GetRadius() ) )
        {
            continue;
        }

        // STL really sometimes creates an unreadable mess.
        // Check the objtypes vector, if the type is in the list.
        // If yes, the caller wants this obj in the return list.
        for(std::vector<int>::const_iterator typeiter = objtypes.begin();
            typeiter != objtypes.end(); typeiter++)
        {
            if(obj->GetType() == (*typeiter))
                objlist.push_back(obj);
        }
    }
    return objlist;
}

void CWorld::Update(const float dt, const uint32_t ticks)
{
    if(!IsClient())
    {
        state.leveltime = ticks - m_leveltimestart;
        state.worldid++;
    }

    UpdatePendingObjs();

    if(!m_bsptree.IsLoaded())
        return;
}

#define STOP_EPSILON           (0.2f)

void PM_ClipVelocity(const vec3_t& in, const vec3_t& normal, vec3_t& out, const float overbounce)
{
    float backoff;
    float change;
    int i;

    // Determine how far along plane to slide based on incoming direction.
    // Scale by overbounce factor.
    backoff = in*normal * overbounce;

    for(i=0; i<3; i++)
    {
        change = normal.v[i]*backoff;
        out.v[i] = in.v[i] - change;

        if(fabsf(out.v[i]) < STOP_EPSILON) // If out velocity is too small, zero it out.
           out.v[i] = 0.0f;
    }
}

#define GRAVITY                (30.00f) // should this be a world property?
const static vec3_t gravity(0, -GRAVITY, 0);

#define MAX_CLIP_PLANES      5
#define OVERCLIP            (1.01f)
#define F_SCALE             (0.98f)
#define MAX_VELOCITY        (55.0f) // m/s

void CWorld::ObjMove(CObj* obj, const float dt) const
{
    if(obj->GetFlags() & OBJ_FLAGS_GHOST)
        return;

    bool groundhit = false; // obj on ground?
    bool wallhit = false; // contact with level geometry
    plane_t wallplane;

    bsp_sphere_trace_t trace;
    trace.radius = obj->GetRadius();

    // quake style movement clipping
    vec3_t vel = obj->GetVel();
    vel.MaxLength(MAX_VELOCITY); // the velocity is too damn high

    vec3_t planes[MAX_CLIP_PLANES];
    vec3_t pos = obj->GetOrigin();
    vec3_t dir;
    float d;
    int numbumps = 4; // bump up to 4 times
    int bumpcount;
    int numplanes = 0; // and not sliding anything
    vec3_t original_velocity = vel; // store original velocity
    vec3_t primal_velocity = vel;
    vec3_t new_velocity;
    float all_fraction = 0.0f;
    float time_left = dt;
    vec3_t end;
    float f; // trace.f: fraction of path along trace direction
    int i, j;

    for(bumpcount = 0; bumpcount < numbumps; bumpcount++)
    {
        if(vel == vec3_t::origin)
            break;

        // Assume we can move all the way from the current origin to the
        // end point.
        // See, if we can move along this path:
        trace.start = pos;
        trace.dir = time_left*vel;
        GetBSP()->TraceSphere(&trace);
        f = trace.f;
        if(f > 1.0f)
            f = 1.0f;
        if(f < 0.0f)
            f = 0.0f;

        all_fraction += f;
        // If we moved some portion of the total distance, then
        //  copy the end position into pos and
        //  zero the plane counter.
        if(f > 0.0f)
        {
            pos += trace.dir*f*F_SCALE;
            original_velocity = vel;
            numplanes = 0;
        }

        // If we covered the entire distance, we are done
        //  and can return.
        if(f == 1.0f)
            break;

        wallplane = trace.p;
        wallhit = true;

        // Reduce amount of frametime (dt) left by total time left * fraction
        //  that we covered.
        time_left = time_left * (1-f);

        // If the plane we hit has a high y component in the normal, then
        //  it's probably a floor
        if(trace.p.m_n.v[1] > 0.7)
            groundhit = true;

        // Did we run out of planes to clip against?
        if(numplanes >= MAX_CLIP_PLANES)
        {   // this shouldn't really happen
            //  Stop our movement if so.
            vel = vec3_t::origin;
            //fprintf(stderr, "Too many planes %i\n", MAX_CLIP_PLANES);
            break;
        }

        // Set up next clipping plane
        planes[numplanes] = trace.p.m_n;
        numplanes++;

        for(i=0; i<numplanes; i++)
        {
            PM_ClipVelocity(original_velocity, planes[i], new_velocity, OVERCLIP);
            for(j=0; j<numplanes; j++)
            {
                if(j != i && !(planes[i] == planes[j]))
                {
                    // Are we now moving against this plane?
                    if (new_velocity*planes[j] < 0)
                        break;  // not ok
                }
            }
            if(j == numplanes)  // Didn't have to clip, so we're ok
                break;
        }

        // Did we go all the way through plane set
        if(i != numplanes)
        {   // go along this plane
            vel = new_velocity;
        }
        else
        {   // go along the crease
            if(numplanes != 2)
            {
                vel = vec3_t::origin;
                //fprintf(stderr, "Trapped 4, clip velocity, numplanes == %i\n", numplanes);
                break;
            }
            dir = planes[0] ^ planes[1];
            // dir = planes[1] ^ planes[0];
            d = dir * vel;
            vel = dir * d;
        }

        // if original velocity is against the original velocity, stop dead
        // to avoid tiny oscillations in sloping corners
        //
        if(vel*primal_velocity <= 0.0f)
        {
            vel = vec3_t::origin;
            break;
        }
    }

    if(all_fraction == 0.0f)
    {
        vel = vec3_t::origin;
    }

    if(wallhit)
    {
        obj->OnHitWall(pos, wallplane.m_n);
    }

    if(!(obj->GetFlags() & OBJ_FLAGS_NOGRAVITY)) // Objekt reagiert auf Gravity
    {
        if(groundhit)
        {
            //vel -= gravity*dt;
            vel.y = 0.0f;
        }
        else
        {
            vel += gravity*dt;
        }
    }

    if(GetBSP()->IsSphereStuck(pos, obj->GetRadius()))
    {
        // OK something went wrong with our movement

        // let's see if our original position was OK
        // If it is also stuck (if cond. here), then
        // the TryUnstuck function will set the obj->Origin to something
        // that is hopefully OK.
        // otherwise we just use the old obj position before
        if(GetBSP()->IsSphereStuck(obj->GetOrigin(), obj->GetRadius()))
        {
            if(TryUnstuck(obj))
            {
                fprintf(stderr, "Object successfully unstuck\n");
            }
            else
            {
                fprintf(stderr, "Failed to unstuck object\n");
                assert(0); // uh oh, should not happen
            }
        }
        pos = obj->GetOrigin(); // this is either the original position or some position from TryUnstuck()
    }

    obj->m_locIsOnGround = groundhit;
    obj->SetOrigin(pos);
    obj->SetVel(vel);
}

bool CWorld::TryUnstuck(CObj* obj) const
{
    static const vec3_t offset_table[] =
    {
        vec3_t( -1,  1, -1),
        vec3_t(  1,  1, -1),
        vec3_t(  1,  1,  1),
        vec3_t( -1,  1,  1),
        vec3_t( -1, -1, -1),
        vec3_t(  1, -1, -1),
        vec3_t(  1, -1,  1),
        vec3_t( -1, -1,  1)
    };
    static const float offset_table_size = sizeof(offset_table)/sizeof(offset_table[0]);
    const float radius = obj->GetRadius();
    const float radius_scale = radius * 0.1f;
    int i, j;

    for(j=1; j<4; j++)
    {
        for(i=0;i<offset_table_size;i++)
        {
            const vec3_t try_pos = obj->GetOrigin() + j * radius_scale * offset_table[i];
            if(!GetBSP()->IsSphereStuck(try_pos, radius))
            {
                obj->SetOrigin(try_pos);
                return true;
            }
        }
    }

    return false;
}

// returns true if something was hit (level or obj)
// maxdist: max distance in world units.
bool CWorld::TraceObj(world_obj_trace_t* trace, const float maxdist)
{
    OBJITER iter;
    float minf = MAX_TRACE_DIST;
    float cf;
    CObj* obj;
    CObj* pobjhit = NULL;

    // baaaad method. this code tests every (!) object in the world.
    // FIXME: use some space partitioning method to speed things up.
    for(iter = ObjBegin(); iter != ObjEnd(); iter++)
    {
        obj = (*iter).second;

        if(obj->GetID() == trace->excludeobj_id ||  // normally the player that shoots
           obj->GetFlags() & OBJ_FLAGS_GHOST ||     // ghost objects are not traceable
           obj->GetRadius() == 0.0f)                // 0 radius objects too
            continue;

        if((obj->GetOrigin() - trace->start).AbsFast() > maxdist) // object is too far away
            continue;

        if(!vec3_t::RaySphereIntersect(trace->start, trace->dir,
                                   obj->GetOrigin(), obj->GetRadius(),
                                   &cf))
        {
            continue;
        }

        if(cf >= 0.0f && cf < minf)
        {
            pobjhit = obj;
            minf = cf;
        }
    }

    if(pobjhit == NULL) // okay, we've hit nothing
    {
        trace->hitobj = NULL;
    }
    else // we have hit an object
    {
        trace->hitpoint = trace->start + minf*trace->dir;
        trace->hitnormal = (trace->hitpoint - pobjhit->GetOrigin()).Normalized();
        trace->hitobj = pobjhit;
    }

    // now check if there is something from the level in our way
    bsp_sphere_trace_t spheretrace;
    spheretrace.start = trace->start;
    spheretrace.dir = trace->dir.Normalized() * maxdist;
    spheretrace.radius = 0.01f; // small epsilon
    GetBSP()->TraceSphere(&spheretrace);
    if(spheretrace.f < 1.0f) // we have hit level geometry
    {
        const vec3_t hitpointlevel = spheretrace.start + spheretrace.f*spheretrace.dir;
        if(trace->hitobj) // we have hit an object earlier
        {
            if((trace->hitpoint - trace->start).AbsFast() <
               (hitpointlevel - trace->start).AbsFast())
            {
                return true; // the object is nearer than the level geometry
            }
        }

        trace->hitpoint = hitpointlevel;
        trace->hitnormal = spheretrace.p.m_n;
        trace->hitobj = NULL;

        return true;
    }
    else
    {
        return (trace->hitobj != NULL);
    }
}

void CWorld::UpdatePendingObjs()
{
    if(m_removeobj.size() > 0)
    {
        // remove objects from queue
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
        // add pending objects
        std::list<CObj*>::const_iterator additer;
        for(additer=m_addobj.begin();additer!=m_addobj.end();additer++)
        {
            assert(GetObj((*additer)->GetID()) == NULL);
            m_objlist[(*additer)->GetID()] = (*additer);
        }
        m_addobj.clear();
    }
}

bool CWorld::LoadLevel(const std::string path)
{
    bool success = m_bsptree.Load(path, IsClient() ? GetResourceManager() : NULL);
    if(success)
        state.level = m_bsptree.GetFilename();
    return success;
}

// DELTA COMPRESSION CODE ------------------------------------

#define WORLD_STATE_WORLDID         (1 <<  0)
#define WORLD_STATE_LEVELTIME       (1 <<  1)
#define WORLD_STATE_LEVEL           (1 <<  2)

#define WORLD_STATE_NO_REAL_CHANGE  (WORLD_STATE_WORLDID|WORLD_STATE_LEVELTIME) // worldid und leveltime ändern sich sowieso immer
#define WORLD_STATE_FULLUPDATE      ((1 <<  3)-1)

bool CWorld::Serialize(bool write, CStream* stream, const world_state_t* oldstate)
{
    assert(stream);
    CObj* obj;
    OBJITER iter;
    int changes = 0;
    if(!stream)
        return false;

    if(write)
    {
        uint32_t updateflags = 0;
        CStream tempstream = stream->GetStream();
        stream->WriteAdvance(sizeof(uint32_t)); // An dieser Stelle sollten als DWORD die Updateflags stehen, diese kennen wir erst, nachdem wir sie geschrieben haben

        // Write to tempstream to check for updateflags
        DeltaDiffDWORD(&state.worldid, oldstate ? &oldstate->worldid : NULL, WORLD_STATE_WORLDID, &updateflags, stream);
        DeltaDiffDWORD(&state.leveltime, oldstate ? &oldstate->leveltime : NULL, WORLD_STATE_LEVELTIME, &updateflags, stream);
        DeltaDiffString(&state.level, oldstate ? &oldstate->level : NULL, WORLD_STATE_LEVEL, &updateflags, stream);
        // [NEW ATTRIBUTES HERE]

        if(updateflags > WORLD_STATE_NO_REAL_CHANGE)
            changes++;

        assert(oldstate ? 1 : (updateflags == WORLD_STATE_FULLUPDATE)); // this has to be enforced
        tempstream.WriteDWORD(updateflags); // Now we know the updateflags and can write them to the actual stream

        // Write all objects
        assert(GetObjCount() < INT_MAX);
        stream->WriteDWORD((uint32_t)GetObjCount());

        obj_state_t obj_oldstate;
        obj_state_t* p_obj_oldstate;
        for(iter = m_objlist.begin();iter!=m_objlist.end();iter++)
        {
            obj = (*iter).second;
            p_obj_oldstate = NULL;
            if(oldstate) // Delta Compression
            {
                if(oldstate->ObjStateExists(obj->GetID()))
                {
                    oldstate->GetObjState(obj->GetID(), obj_oldstate);
                    p_obj_oldstate = &obj_oldstate;
                }
            }
            stream->WriteDWORD(obj->GetID());
            if(obj->Serialize(true, stream, obj->GetID(), p_obj_oldstate))
                changes++;
        }
    }
    else
    {
        uint32_t updateflags;
        uint32_t worldid;
        std::string level;
        uint32_t objcount;
        uint32_t objid;

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
                if(LoadLevel(level)==false)
                {
                    // FIXME error handling
                    assert(0);
                    return false;
                }
            }
        }
        // new attributes here
        if(updateflags > WORLD_STATE_NO_REAL_CHANGE)
            changes++;

        stream->ReadDWORD(&objcount);
        // objread contains all read objects. every object that is not here, has
        // to be deleted
        std::map<int,int> objread;
        for(unsigned int i=0;i<objcount;i++)
        {
            stream->ReadDWORD(&objid);
            obj = GetObj(objid);
            if(!obj)
            {
                obj = new CObj(this);
                changes += obj->Serialize(false, stream, objid) ? 1 : 0;
                AddObj(obj);
            }
            else
                changes += obj->Serialize(false, stream, objid) ? 1 : 0;
            assert(objread.find(objid) == objread.end()); // is not supposed to be already in stream
            objread[obj->GetID()] = obj->GetID();
        }
        // Delete all objects that are not in the latest state
        for(iter=m_objlist.begin();iter!=m_objlist.end();iter++)
        {
            obj = (*iter).second;
            if(objread.find(obj->GetID()) != objread.end())
                continue;
            DelObj(obj->GetID());
        }

        UpdatePendingObjs();
    }

    return changes > 0;
}

world_state_t CWorld::GetWorldState()
{
    world_state_t worldstate = state;

    assert(worldstate.GetObjCount() == 0);

    OBJITER iter;
    for(iter = m_objlist.begin();iter!=m_objlist.end();iter++)
    {
        CObj* obj = (*iter).second;
        worldstate.AddObjState(obj->GetObjState(), obj->GetID());
    }

    return worldstate;
}

// world_state_t struct methods for managed and secure access

void world_state_t::AddObjState(obj_state_t objstate, const int id)
{
    assert(!ObjStateExists(id));
    objstates.push_back(objstate);
    objindex[id] = (int)objstates.size()-1;
}

bool world_state_t::ObjStateExists(const int id) const
{
    WORLD_STATE_CONSTOBJITER indexiter = objindex.find(id);
    return indexiter != objindex.end();
}

bool world_state_t::GetObjState(const int id, obj_state_t& objstate) const
{
    WORLD_STATE_CONSTOBJITER indexiter = objindex.find(id);
    if(indexiter == objindex.end())
        return false;
    objstate = objstates[indexiter->second];
    return true;
}

