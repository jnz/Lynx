#pragma once

#include "Subject.h"
#include "Events.h"
#include "World.h"

class CGameLogic : public CObserver<EventNewClientConnected>
{
public:
	CGameLogic(CWorld* world);
	~CGameLogic(void);

	void Notify(EventNewClientConnected);

private:
	CWorld* m_world;
};
