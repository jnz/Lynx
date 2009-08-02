#pragma once
#include <enet/enet.h>
//#include "World.h"
//#include <list>

enum clientstate_t {
		S0_CLIENT_CONNECTED=0, 
		S1_CLIENT_SENDING_WORLD
		};

class CClientInfo
{
public:
	CClientInfo(ENetPeer* peer)
	{
		m_id = ++m_idpool;
		m_peer = peer;
		m_state = S0_CLIENT_CONNECTED;
		m_obj = 0;
	}

	int GetID() { return m_id; }
	ENetPeer* GetPeer() { return m_peer; }

	clientstate_t m_state;
	int m_obj; // Objekt in Welt, das diesen Client repräsentiert

    //std::list<world_state_t> worldstates;

private:
	int m_id;
	ENetPeer* m_peer;
	static int m_idpool;
};