#pragma once

#include "Subject.h"
#include "Events.h"
#include "World.h"

class CGameLogic : public CObserver<EventNewClientConnected>,
	               public CObserver<EventClientDisconnected>
{
public:
	CGameLogic(CWorld* world);
	~CGameLogic(void);

	void InitGame();

protected:
	void Notify(EventNewClientConnected);
	void Notify(EventClientDisconnected);

private:
	CWorld* m_world;
};
