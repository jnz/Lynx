#pragma once

#include "GameLogic.h"
#include "GameObj.h"
#include "GameObjPlayer.h"

class CGameZombie : public CGameLogic
{
public:
    CGameZombie(CWorld* world, CServer* server);
	~CGameZombie(void);

	virtual void InitGame();
	virtual void Update(const float dt, const DWORD ticks);

protected:
	virtual void Notify(EventNewClientConnected);
	virtual void Notify(EventClientDisconnected);

    virtual void ProcessClientCmds(CGameObjPlayer* clientobj, CClientInfo* client);
};
