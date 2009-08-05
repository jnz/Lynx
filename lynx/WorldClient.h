#pragma once
#include "world.h"

struct worldclient_state_t
{
    world_state_t state;
    DWORD   localtime; // in ms
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

protected:
    std::list<worldclient_state_t> m_history; // History buffer for interpolation

    void ClientInterp();


private:
	CObj* m_localobj; // Objekt mit dem dieser Client verknüpft ist
	CObj m_ghostobj; // Controller Object, das wir direkt steuern
};
