#pragma once

class CWorld;
#include <map>
#ifdef __linux  // Linux
  #include <unordered_map>
#elif defined(__APPLE__) || defined(__APPLE_CC__) // Apple
  #include <ext/hash_map>
#else // the rest (Windows)
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

// For the hash_map on Apple
#if defined(__APPLE__) || defined(__APPLE_CC__) // Apple
namespace stdext
{
    using namespace __gnu_cxx;
}
#endif

#ifdef __linux
  #define WORLD_STATE_OBJMAPTYPE    std::unordered_map<int, int>
  #define WORLD_STATE_OBJITER       std::unordered_map<int, int>::iterator
  #define WORLD_STATE_CONSTOBJITER  std::unordered_map<int, int>::const_iterator
  // STL types for our object storage
  #define OBJMAPTYPE                std::unordered_map<int, CObj*>
  #define OBJITER                   std::unordered_map<int, CObj*>::iterator
  #define OBJITERCONST              std::unordered_map<int, CObj*>::const_iterator
#else
  #define WORLD_STATE_OBJMAPTYPE    stdext::hash_map<int, int>
  #define WORLD_STATE_OBJITER       stdext::hash_map<int, int>::iterator
  #define WORLD_STATE_CONSTOBJITER  stdext::hash_map<int, int>::const_iterator
  // STL types for our object storage
  #define OBJMAPTYPE                stdext::hash_map<int, CObj*>
  #define OBJITER                   stdext::hash_map<int, CObj*>::iterator
  #define OBJITERCONST              stdext::hash_map<int, CObj*>::const_iterator
#endif

// CPlayerInfo and world_player_t (name, score, ping, team etc.):
// These data structures are used as a property of a world.
// Normally only the server module has the information about the connected
// clients. The server is then using the CPlayerInfo class to update
// the world with informations about the connected players. So the clients
// can e.g. display a scoreboard.
//
// world_player_t is stored in a vector in CPlayerInfo.

/*
 *enum
 *{
 *    WORLD_PLAYER_UPDATE_NAME    = 2,
 *    WORLD_PLAYER_UPDATE_SCORE   = 4,
 *    WORLD_PLAYER_UPDATE_LATENCY = 8,
 *};
 *
 *struct world_player_t
 *{
 *    world_player_t()
 *    {
 *        id = 0;
 *        name = "not init";
 *        score = 0;
 *        latency = 0;
 *    }
 *
 *    uint32_t     id;      // player id, copied from CServer's CClientInfo
 *    std::string  name;    // player name, human readable
 *    uint16_t     score;   // score count
 *    uint16_t     latency; // network latency / ping
 *};
 *
 *class CPlayerInfo
 *{
 *public:
 *    void AddPlayer(world_player_t player);
 *    void UpdatePlayer(int id, world_player_t);
 *    void RemovePlayer(int id);
 *    void RemoveAllPlayer();
 *    bool GetPlayer(const int id, world_player_t** player);
 *
 *    // Serialize the world state to a byte stream.
 *    // Returns true if the state has changed compared to the oldstate.
 *    bool Serialize(bool write, CStream* stream, const CPlayerInfo* oldstate=NULL);
 *
 *private:
 *    // infos about all players in current snapshot is stored here in this vector
 *    std::vector<world_player_t> m_playerlist;
 *};
 */

// Essential game engine struct: world_state_t
// This struct holds one complete snapshot of the game.
//
// Everything to describe the current state of the game
// is stored in this struct.
// The objstates vector contains all the objects.
// The objindex is a map to the objstates vector
// indices.
// E.g. you have the OBJID of an object,
// then you can access the obj_state_t in this way:
//
//  objstates[objindex[OBJID]]

struct world_state_t
{
    uint32_t    leveltime;  // time in [ms]
    uint32_t    worldid;    // worldid increments every frame, unique identifier
    std::string level;      // path to .lbsp level file
    //CPlayerInfo playerinfo; // current active players (name, score, ping)

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

// world_obj_trace_t:
// Search for objects hit by a ray, used by the CWorld::TraceObj function
struct world_obj_trace_t
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

class CWorld
{
public:
    CWorld();
    virtual ~CWorld();

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
    bool            TryUnstuck(CObj* obj) const; // find a position that is not in the world geometry

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

private:
    // Rule of three
    CWorld(const CWorld&);
    CWorld& operator=(const CWorld&);
};

