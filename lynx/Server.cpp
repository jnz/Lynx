#include "NetMsg.h"
#include "Server.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#define MAXCLIENTS			8
#define SERVER_UPDATETIME	500

CServer::CServer(CWorld* world)
{
	enet_initialize();
	m_server = NULL;
	m_lastupdate = 0;
	m_world = world;
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

void CServer::Update(const float dt)
{
    ENetEvent event;
	CClientInfo* clientinfo;
	std::map<int, CClientInfo*>::iterator iter;
	bool success;

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
			EventNewClientConnected e;
			e.client = clientinfo;
			NotifyAll(e);

			event.peer->data = clientinfo;
			m_clientlist[clientinfo->GetID()] = clientinfo;
			success = SendWorldToClient(clientinfo);
			assert(success);
			// FIXME
			// Disconnect client on error

            break;

        case ENET_EVENT_TYPE_RECEIVE:
            fprintf(stderr, "A packet of length %u containing %s was received from %s on channel %u.\n",
                    event.packet->dataLength,
                    event.packet->data,
                    event.peer->data,
                    event.channelID);

			enet_packet_destroy (event.packet);
            
            break;
           
        case ENET_EVENT_TYPE_DISCONNECT:
			clientinfo = (CClientInfo*)event.peer->data;
			assert(clientinfo);
			fprintf(stderr, "Client %i disconnected.\n", clientinfo->GetID());
			
			iter = m_clientlist.find(clientinfo->GetID());
			assert(iter != m_clientlist.end());
			delete (*iter).second;
			m_clientlist.erase(iter);
            event.peer->data = NULL;
        }
    }

	if(CLynx::GetTicks() - m_lastupdate > SERVER_UPDATETIME)
	{
		std::map<int, CClientInfo*>::iterator iter;
		for(iter = m_clientlist.begin();iter!=m_clientlist.end();iter++)
			SendDeltaWorldToClient((*iter).second);
		
		m_lastupdate = CLynx::GetTicks();
	}
}

bool CServer::SendWorldToClient(CClientInfo* client)
{
	CStream stream;
	ENetPacket* packet;
	int reqsize = m_world->Serialize(true, NULL);
	assert(reqsize > 0);
	int localobj = client->m_obj;

	if(!stream.Resize(reqsize+CNetMsg::MaxHeaderLen()+sizeof(localobj)))
		return false;

	CNetMsg::WriteHeader(&stream, NET_MSG_SERIALIZE_WORLD); // Writing Header
	stream.WriteDWORD(localobj);
	if(m_world->Serialize(true, &stream) != reqsize)
	{
		assert(0);
		return false;
	}

	packet = enet_packet_create(stream.GetBuffer(), 
								stream.GetBytesWritten(), 
								ENET_PACKET_FLAG_RELIABLE);
	assert(packet);
	if(!packet)
		return false;

	//fprintf(stderr, "SV: Complete World: %i bytes\n", stream.GetBytesWritten());
	client->m_state = S1_CLIENT_SENDING_WORLD;
	return enet_peer_send(client->GetPeer(), 0, packet) == 0;
}

bool CServer::SendDeltaWorldToClient(CClientInfo* client)
{
	CStream stream;
	ENetPacket* packet;
	const int mtu = 1400; // FIXME do not hardcode this here
	int lastobj = 0;
	int objs;
	int totalsize = 0;

	if(!stream.Resize(mtu+CNetMsg::MaxHeaderLen()))
		return false;

	do
	{
		stream.ResetWritePosition();
		CNetMsg::WriteHeader(&stream, NET_MSG_UPDATE_WORLD); // Writing Header
		objs = m_world->SerializePositions(true, &stream, client->m_obj, mtu, &lastobj);
		if(objs < 1)
			break;

		totalsize += stream.GetBytesWritten();
		packet = enet_packet_create(stream.GetBuffer(), 
									stream.GetBytesWritten(), 0);

		assert(packet);
		if(!packet)
			return false;
		if(enet_peer_send(client->GetPeer(), 0, packet) != 0)
		{
			assert(0);
			return false;
		}
	}while(lastobj > 0);

	//fprintf(stderr, "SV: Client Update: %i bytes\n", totalsize);

	return true;
}