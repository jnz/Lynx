#pragma once

class CWorld;
#include <map>
#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif
#include <list>
#include "Obj.h"
#include "BSPLevel.h"
#include "ResourceManager.h"

/*
    CWorld is the core of the Lynx engine.
    The current state of the game is stored in CWorld.
    The data itself is in the world_state_t struct. The CWorld
    class provides the functions to access this data.
    Every frame on the server modifies the world_state_t and the
    task is to distribute the changes to the client. The client
    needs a copy of the current world_state_t to display the game.

    Update() is modifying the world each frame.
    The serialize function writes the world_state to a binary stream.
    This binary stream is send to each client. The major optimization
    is that the serialize function can create a delta snapshot of
    the world state. A new client gets the complete world_state transmitted.
    But then only delta updates are transmitted to the client.

    This is transparent to the server game logic. The game logic is modifying
    the server CWorld and the network logic takes care of distributing the
    deltas to the connected clients.
 */

/*
    world_state_t is not to be modified outside of CWorld.
    world_state_t represents a complete snapshot of the game state.

    If you change world_state_t, change:
    - GenerateWorldState
    - Serialize
 */

#ifdef __GNUC__
namespace stdext
{
    using namespace __gnu_cxx;
}
#endif

#define WORLD_STATE_OBJMAPTYPE    stdext::hash_map<int, int>
#define WORLD_STATE_OBJITER       stdext::hash_map<int, int>::iterator
#define WORLD_STATE_CONSTOBJITER  stdext::hash_map<int, int>::const_iterator

struct world_state_t
{
    uint32_t    leveltime; // time in [ms]
    uint32_t    worldid; // worldid increments every frame, unique world identifier
    std::string level; // path to .lbsp level file

    // Everything to describe the current state of the game
    // is stored in this struct.
    // The objstates vector contains all the objects.
    // The objindex is a map to the objstates vector
    // indices.
    // E.g. you have the OBJID of an object,
    // then you can access the obj_state_t in this way:
    //
    //  objstates[objindex[OBJID]]

    void        AddObjState(obj_state_t objstate, const int id);
    bool        ObjStateExists(const int id) const;
    bool        GetObjState(const int id, obj_state_t& objstate) const;
    WORLD_STATE_OBJITER ObjBegin() { return objindex.begin(); }
    WORLD_STATE_OBJITER ObjEnd() { return objindex.end(); }
    int         GetObjCount() const { return (int)objstates.size(); }

protected:
    std::vector<obj_state_t> objstates; // List with all objects
    WORLD_STATE_OBJMAPTYPE objindex; // ID to objstates index table. Key = obj id, value = index in objstates table
};

struct world_obj_trace_t // Search for objects hit by a ray, used by the TraceObj function
{
    // Input
    vec3_t  start; // start point
    vec3_t  dir; // end point = start + dir
    int     excludeobj_id; // Which object id should be ignored

    // Output
    vec3_t  hitpoint;
    vec3_t  hitnormal;
    CObj*   hitobj; // NULL, if no object was hit
};

// STL types for our object storage
#define OBJMAPTYPE   stdext::hash_map<int, CObj*>
#define OBJITER      stdext::hash_map<int, CObj*>::iterator
#define OBJITERCONST stdext::hash_map<int, CObj*>::const_iterator

class CWorld
{
public:
    CWorld(void);
    virtual ~CWorld(void);

    void            Shutdown();

    // If the world is not a client, some operations can be skipped, e.g. loading textures
    virtual bool    IsClient() const { return false; }

    virtual void    Update(const float dt, const uint32_t ticks); // Calculate a new frame.

    void            AddObj(CObj* obj, bool inthisframe=false); // Add object to world. World will free the memory of CObj*.
    void            DelObj(int objid); // Remove object from the world. Get deleted at the end of the frame.

    CObj*           GetObj(int objid); // Search for object with this id.

    // Number of currently active objects. Including ghost objects.
    int             GetObjCount() const { return (int)m_objlist.size(); }
    OBJITER         ObjBegin() { return m_objlist.begin(); } // Begin Iterator
    OBJITER         ObjEnd() { return m_objlist.end(); } // End Iterator

    // Get objects within radius and the specific type, or every object if type is < 0
    const std::vector<CObj*> GetNearObj(const vec3_t& origin,
                                        const float radius,
                                        const int exclude,
                                        const int type) const;

    // Returns objects within radius, where the obj type is in objtypes array
    const std::vector<CObj*> GetNearObjByTypeList(const vec3_t& origin,
                                                  const float radius,
                                                  const int exclude,
                                                  const std::vector<int>& objtypes) const;

    // Serialize the world state to a byte stream.
    // Returns true if the world has changed compared to the oldstate.
    virtual bool    Serialize(bool write, CStream* stream, const world_state_t* oldstate=NULL);

    bool            LoadLevel(const std::string path); // Load the level from a .lbsp file
    const virtual CBSPLevel* GetBSP() const { return &m_bsptree; }
    uint32_t        GetLeveltime() const { return state.leveltime; } // Leveltime in ms. Starts at 0 ms.
    uint32_t        GetWorldID() const { return state.worldid; } // WorldID get incremented by 1 for each Update() call

    virtual CResourceManager* GetResourceManager() { return &m_resman; }

    // Move object and perform collision detection with level geometry
    void            ObjMove(CObj* obj, const float dt) const;

    // TraceObj is a core engine function to check if an object can travel along
    // a vector or if it hits the level geometry.
    // The level geometry is stored as a KD tree, so this call scales pretty
    // well even if there are many triangles in the scene.
    //
    // returns true if something is hit.
    bool            TraceObj(world_obj_trace_t* trace, const float maxdist);

    world_state_t   GetWorldState();

protected:
    CResourceManager m_resman;
    world_state_t   state;
    uint32_t        m_leveltimestart;
    CBSPLevel       m_bsptree;

    OBJMAPTYPE      m_objlist;
    void            UpdatePendingObjs(); // Deletes objects and adds new objects (from m_addobj and m_removeobj list)
    void            DeleteAllObjs(); // Delete everything

    std::list<CObj*> m_addobj; // Objects that will be added by UpdatePendingObjs
    std::list<int>  m_removeobj; // Objects that will be deleted by UpdatePendingObjs
};

