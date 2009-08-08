#include "NetMsg.h"
#include "Server.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#define MAXCLIENTS				8
#define SERVER_UPDATETIME		50
#define SERVER_MAX_WORLD_AGE	5000				// wie viele ms heben wir für den client eine welt auf
#define MAX_WORLD_BACKLOG		(10*MAXCLIENTS)		// wie viele welten werden gespeichert

CServer::CServer(CWorld* world)
{
	enet_initialize();
	m_server = NULL;
	m_lastupdate = 0;
	m_world = world;
    m_stream.Resize(16000);
}

CServer::~CServer(void)
{
	Shutdown();
	enet_deinitialize();
}

bool CServer::Create(int port)
{
	ENetAddress addr;
	
	addr.host = ENET_HOST_ANY;
	addr.port = port;

	m_server = enet_host_create(&addr, MAXCLIENTS, 0, 0);
	if(!m_server)
		return false;

	return true;
}

void CServer::Shutdown()
{
	if(m_server)
	{
		enet_host_destroy(m_server);
		m_server = NULL;
	}

	std::map<int, CClientInfo*>::iterator iter;
	for(iter = m_clientlist.begin();iter!=m_clientlist.end();iter++)
		delete (*iter).second;
	m_clientlist.clear();
}

void CServer::Update(const float dt, const DWORD ticks)
{
    ENetEvent event;
	CClientInfo* clientinfo;
	std::map<int, CClientInfo*>::iterator iter;
    CStream stream;

    while(enet_host_service(m_server, &event, 0) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
            fprintf(stderr, "A new client connected from %x:%u.\n", 
                    event.peer->address.host,
                    event.peer->address.port);

            clientinfo = new CClientInfo(event.peer);
			
			// Fire Event
            {
			EventNewClientConnected e;
			e.client = clientinfo;
            CSubject<EventNewClientConnected>::NotifyAll(e);
            }

			event.peer->data = clientinfo;
			m_clientlist[clientinfo->GetID()] = clientinfo;
            break;

        case ENET_EVENT_TYPE_RECEIVE:
			stream.SetBuffer(event.packet->data,
							(int)event.packet->dataLength,
							(int)event.packet->dataLength);
			OnReceive(&stream, (CClientInfo*)event.peer->data);

			enet_packet_destroy (event.packet);
            
            break;
           
        case ENET_EVENT_TYPE_DISCONNECT:
			clientinfo = (CClientInfo*)event.peer->data;
			assert(clientinfo);
			fprintf(stderr, "Client %i disconnected.\n", clientinfo->GetID());
			
            // Observer benachrichtigen
            {
            EventClientDisconnected e;
            e.client = clientinfo;
            CSubject<EventClientDisconnected>::NotifyAll(e);
            }

            // Client aus Client-Liste löschen und Speicher freigeben
			iter = m_clientlist.find(clientinfo->GetID());
			assert(iter != m_clientlist.end());
			delete (*iter).second;
			m_clientlist.erase(iter);
            event.peer->data = NULL;
        }
    }

	if(ticks - m_lastupdate > SERVER_UPDATETIME)
	{
		int sent = 0;
		std::map<int, CClientInfo*>::iterator iter;
		for(iter = m_clientlist.begin();iter!=m_clientlist.end();iter++)
        {
			if(SendWorldToClient((*iter).second))
				sent++;
        }
		
		m_lastupdate = ticks;
		if(sent > 0)
			m_history[m_world->GetWorldID()] = m_world->GenerateWorldState();
		UpdateHistoryBuffer();
	}
}

void CServer::OnReceive(CStream* stream, CClientInfo* client)
{
	BYTE type;

	type = CNetMsg::ReadHeader(stream);
	switch(type)
	{
	case NET_MSG_CLIENT_CTRL:
        {
			DWORD worldid;
			stream->ReadDWORD(&worldid);
			ClientHistoryACK(client, worldid);

            CObj* obj = m_world->GetObj(client->m_obj);
            assert(obj);
            vec3_t origin, vel, rot;
            stream->ReadVec3(&origin);
            stream->ReadVec3(&vel);
            stream->ReadVec3(&rot);
            obj->SetOrigin(origin);
            obj->SetVel(vel);
            obj->SetRot(rot);
            // FIXME prüfen ob bewegung plausibel ist
        }
        break;
	case NET_MSG_INVALID:
	default:
		assert(0);
	}


}

void CServer::UpdateHistoryBuffer()
{
	DWORD lowestworldid = 0;
	CClientInfo* client;
	std::map<DWORD, world_state_t>::iterator worlditer;
	std::map<int, CClientInfo*>::iterator clientiter;
	DWORD worldtime;
	DWORD curtime = m_world->GetLeveltime();

	for(clientiter = m_clientlist.begin();clientiter!=m_clientlist.end();clientiter++)
	{
		client = (*clientiter).second;

		if(client->worldidACK == 0) // dieser client hat keine bekannte welt
			continue;

		worlditer = m_history.find(client->worldidACK);
		if(worlditer == m_history.end())
		{
			//assert(0);
			client->worldidACK = 0;
			fprintf(stderr, "Client last known world is not in history buffer\n");
			continue;
		}
		worldtime = ((*worlditer).second).leveltime;
		if(curtime - worldtime > SERVER_MAX_WORLD_AGE)
		{
			fprintf(stderr, "Client world is too old (%i ms)\n", curtime - worldtime);
			client->worldidACK = 0;
			continue;
		}
		if((lowestworldid == 0 || client->worldidACK < lowestworldid) && client->worldidACK > 0)
		{
			lowestworldid = client->worldidACK;
		}
	}

	for(worlditer = m_history.begin();worlditer != m_history.end();)
	{
		worldtime = ((*worlditer).second).leveltime;
		if(curtime - worldtime > SERVER_MAX_WORLD_AGE ||
			((*worlditer).second).worldid < lowestworldid)
		{
			worlditer = m_history.erase(worlditer);
			continue;
		}
		worlditer++;
	}

	assert(m_history.size() < MAX_WORLD_BACKLOG);
	if(m_history.size() >= MAX_WORLD_BACKLOG)
	{
		fprintf(stderr, "Server History Buffer too large. Reset History Buffer.\n");
		m_history.clear();
	}
}

void CServer::ClientHistoryACK(CClientInfo* client, DWORD worldid)
{
	if(client->worldidACK < worldid)
		client->worldidACK = worldid;
}

bool CServer::SendWorldToClient(CClientInfo* client)
{
	ENetPacket* packet;
	int localobj = client->m_obj;
    m_stream.ResetWritePosition();

	CNetMsg::WriteHeader(&m_stream, NET_MSG_SERIALIZE_WORLD); // Writing Header
	m_stream.WriteDWORD(localobj); // FIXME DWORD ist zu groß, WORD reicht

	std::map<DWORD, world_state_t>::iterator iter;
	iter = m_history.find(client->worldidACK);
	if(iter == m_history.end())
	{
		m_world->Serialize(true, &m_stream, NULL);
	}
	else
	{
		if(!m_world->Serialize(true, &m_stream, &(*iter).second))
		{
			// Seit dem letzten bestätigtem Update vom Client hat sich nichts getan.
			client->worldidACK = m_world->GetWorldID();
			return true; // client benötigt kein update
		}
	}

	assert(client->GetPeer()->mtu > m_stream.GetBytesWritten());
	packet = enet_packet_create(m_stream.GetBuffer(), 
								m_stream.GetBytesWritten(), 
								0);
	assert(packet);
	if(!packet)
		return false;

	//fprintf(stderr, "SV: Complete World: %i bytes\n", stream.GetBytesWritten());
	client->m_state = S1_CLIENT_SENDING_WORLD;
	return enet_peer_send(client->GetPeer(), 0, packet) == 0;
}
