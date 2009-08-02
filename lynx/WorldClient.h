#pragma once
#include "world.h"

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

private:
	CObj* m_localobj; // Objekt mit dem dieser Client verknüpft ist
	CObj m_ghostobj; // Controller Object, das wir direkt steuern
};
