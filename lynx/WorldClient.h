#pragma once
#include "world.h"

struct worldclient_state_t
{
    world_state_t state;
    DWORD   localtime; // in ms
};

// Interpolierte Welt für Renderer

class CWorldClient;
class CWorldInterp : public CWorld
{
public:
	CWorldInterp() {}
	~CWorldInterp() {}
	const virtual CBSPTree* GetBSP() const { return m_pbsp; }
	virtual CResourceManager* GetResourceManager() { return m_presman; }

	void Update(const float dt);

protected:
	CBSPTree* m_pbsp;
	CResourceManager* m_presman;
	worldclient_state_t state1;
	worldclient_state_t state2;

	friend CWorldClient;
};

class CWorldClient :
	public CWorld
{
public:
	CWorldClient(void);
	~CWorldClient(void);

    virtual bool IsClient() { return true; }

	CObj* GetLocalObj();
	void SetLocalObj(int id);
    CObj* GetLocalController() { return &m_ghostobj; }

	void Update(const float dt);

	virtual bool Serialize(bool write, CStream* stream, const world_state_t* oldstate=NULL);

	CWorld* GetInterpWorld() { return m_pinterpworld; }

protected:
    std::list<worldclient_state_t> m_history; // History buffer for interpolation
    void CreateClientInterp(); // Prepare Interp World
	void ClientInterp(); // Interp World korrigieren

	CWorld* m_pinterpworld;
	CWorldInterp m_interpworld;

private:
	CObj* m_localobj; // Objekt mit dem dieser Client verknüpft ist
	CObj m_ghostobj; // Controller Object, das wir direkt steuern
};

