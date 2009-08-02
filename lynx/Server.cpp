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

void CServer::Update(const float dt)
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
            fprintf(stderr, "A packet of length %u containing %s was received from %s on channel %u.\n",
                    event.packet->dataLength,
                    event.packet->data,
                    event.peer->data,
                    event.channelID);

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

	if(CLynx::GetTicks() - m_lastupdate > SERVER_UPDATETIME)
	{
		std::map<int, CClientInfo*>::iterator iter;
		for(iter = m_clientlist.begin();iter!=m_clientlist.end();iter++)
			SendWorldToClient((*iter).second);
		
		m_lastupdate = CLynx::GetTicks();
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

bool CServer::SendWorldToClient(CClientInfo* client)
{
	ENetPacket* packet;
	int localobj = client->m_obj;
    m_stream.ResetWritePosition();

	CNetMsg::WriteHeader(&m_stream, NET_MSG_SERIALIZE_WORLD); // Writing Header
	m_stream.WriteDWORD(localobj); // FIXME DWORD ist zu groß, WORD reicht
	m_world->Serialize(true, &m_stream);

	packet = enet_packet_create(m_stream.GetBuffer(), 
								m_stream.GetBytesWritten(), 
								ENET_PACKET_FLAG_RELIABLE);
	assert(packet);
	if(!packet)
		return false;

	//fprintf(stderr, "SV: Complete World: %i bytes\n", stream.GetBytesWritten());
	client->m_state = S1_CLIENT_SENDING_WORLD;
	return enet_peer_send(client->GetPeer(), 0, packet) == 0;
}
