#pragma once
#include "World.h"
#include "ClientHUD.h"

/*
    CWorldClient collects the world snapshots from the server.
    CWorldClient itself is the latest known snapshot from the server.
    The snapshots are normally delta-compressed.
    For a nice interpolation, the latest snapshot is not rendered,
    but instead a interpolated snapshot is created, based on the
    history buffer (collected worldstate snapshots) with a
    delay of 100 ms. This interpolated snapshot has the CWorldInterp
    class.
 */

struct worldclient_state_t
{
    world_state_t state;
    uint32_t   localtime; // in ms
};

// Interpolierte Welt für Renderer

class CWorldClient;
class CWorldInterp : public CWorld
{
public:
    CWorldInterp() { f = 1.0f; }
    ~CWorldInterp() {}

    virtual bool                IsClient() const { return true; }

    const virtual CBSPLevel*    GetBSP() const { return m_pbsp; }
    virtual CResourceManager*   GetResourceManager() { return m_presman; }

    // Lerp this world snapshot
    void                        Update(const float dt, const uint32_t ticks);

protected:
    CBSPLevel*                  m_pbsp;
    CResourceManager*           m_presman;
    worldclient_state_t         state1;
    worldclient_state_t         state2;
    float                       f; // Current lerp factor (0..1)

    friend class CWorldClient;
};

class CWorldClient :
    public CWorld
{
public:
    CWorldClient(void);
    ~CWorldClient(void);

    virtual bool    IsClient() const { return true; }

    // LocalObj is the object, the server has assigned to the player
    CObj*           GetLocalObj() const;
    void            SetLocalObj(int id);
    // This is a client side only object, that the player directly controls
    // with the keyboard and the mouse. The server does not know about this
    // object.
    CObj*           GetLocalController() { return &m_ghostobj; }

    // The Update function takes care of:
    //  - Collision detection for the LocalController object
    //  - Updating the lerped world snapshot
    void            Update(const float dt, const uint32_t ticks);

    // When a new snapshot from the server arrives, it is processed here
    virtual bool    Serialize(bool write, CStream* stream, const world_state_t* oldstate=NULL);

    CWorld*         GetInterpWorld() { return &m_interpworld; } // Get lerped snapshot

    CClientHUD      m_hud;

protected:
     // Push world snapshot to history buffer:
    void            AddWorldToHistory();
    // Create a lerped snapshot from two snapshots from the history buffer:
    void            CreateClientInterp();

    std::list<worldclient_state_t> m_history; // world snapshot history buffer
    CWorldInterp m_interpworld; // Lerped snapshot

private:
    // Pointer to the object that the server linked us to (the player object).
    // We normally don't want to render this object in a first person shooter.
    CObj* m_localobj;
    // This is a client side only object, that the player directly controls
    // with the keyboard and the mouse. The server does not know about this
    // object. You can access this object with the
    // GetLocalController object.
    CObj m_ghostobj;
};

