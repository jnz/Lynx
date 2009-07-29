#pragma once

#include <enet/enet.h>
#include <map>
#include "World.h"
#include "ClientInfo.h"
#include "Subject.h"
#include "Events.h"

class CServer : public CSubject<EventNewClientConnected>
{
public:
	CServer(CWorld* world);
	~CServer(void);

	bool Create(int port); // Server an Port starten
	void Shutdown(); // Server herunterfahren

	void Update(const float dt);

protected:
	bool SendWorldToClient(CClientInfo* client);
	bool SendDeltaWorldToClient(CClientInfo* client);

private:
	ENetHost* m_server;
	std::map<int, CClientInfo*> m_clientlist;

	DWORD m_lastupdate;
	CWorld* m_world;
};
