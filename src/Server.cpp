#include "NetMsg.h"
#include "Server.h"
#include "ServerClient.h"
#include <algorithm> // remove and remove_if

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CServer::CServer(CWorld* world)
{
    enet_initialize();
    m_server = NULL;
    m_lastupdate = 0;
    m_world = world;
    m_stream.SetSize(MAX_SV_PACKETLEN);
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

    // channel limit 2
    m_server = enet_host_create(&addr, MAXCLIENTS, 2, 0, 0);
    if(!m_server)
        return false;

#ifdef USE_RANGE_ENCODER
    enet_host_compress_with_range_coder(m_server);
#endif

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

CClientInfo* CServer::GetClient(int id)
{
    CLIENTITER iter;
    iter = m_clientlist.find(id);
    if(iter == GetClientEnd())
        return NULL;
    return (*iter).second;
}

int CServer::GetClientCount() const
{
    return (int)m_clientlist.size();
}

void CServer::Update(const float dt, const uint32_t ticks)
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
            // Fire Event
            {
            // first we get a human readable hostname
            char hostname[1024];
            enet_address_get_host_ip(&event.peer->address,
                                     hostname, sizeof(hostname));

            // create client object
            clientinfo = new CClientInfo(event.peer, hostname, ticks);
            event.peer->data = clientinfo;
            m_clientlist[clientinfo->GetID()] = clientinfo;

            fprintf(stderr, "A new client connected from %s:%u.\n",
                    hostname,
                    event.peer->address.port);

            EventNewClientConnected e;
            e.client = clientinfo;
            CSubject<EventNewClientConnected>::NotifyAll(e);
            }

            break;

        case ENET_EVENT_TYPE_RECEIVE:
            stream.SetBuffer(event.packet->data,
                             event.packet->dataLength,
                             event.packet->dataLength);
            OnReceive(&stream, (CClientInfo*)event.peer->data);
            enet_packet_destroy (event.packet);
            break;

        case ENET_EVENT_TYPE_DISCONNECT:
            clientinfo = (CClientInfo*)event.peer->data;
            assert(clientinfo);
            if(!clientinfo)
                break; // this should not happen

            fprintf(stderr, "Client %i disconnected.\n", clientinfo->GetID());

            // Message to all observer
            {
            EventClientDisconnected e;
            e.client = clientinfo;
            CSubject<EventClientDisconnected>::NotifyAll(e);
            }

            // Remove client from list and free memory
            iter = m_clientlist.find(clientinfo->GetID());
            assert(iter != m_clientlist.end());
            delete (*iter).second;
            m_clientlist.erase(iter);
            event.peer->data = NULL;
            break;

        case ENET_EVENT_TYPE_NONE:
            break;
        }
    }

    if((ticks - m_lastupdate) > SERVER_UPDATETIME)
    {
        int sent = 0;
        for(iter = m_clientlist.begin();iter!=m_clientlist.end();iter++)
        {
            CClientInfo* client = (*iter).second;
            if(client->disconnected)
                continue;

            // check if we still need a client challenge msg.
            // if we have not received this message after a certain
            // time (SV_MAX_CHALLENGE_TIME in ms) we disconnect the client.
            if(!client->got_challenge &&
               (ticks - client->GetConnecttime() > SV_MAX_CHALLENGE_TIME))
            {
                fprintf(stderr, "Client challenge timeout.\n");
                enet_peer_disconnect_later(client->GetPeer(), 0);
                client->disconnected = true;
                continue;
            }

            if(SendWorldToClient(client))
                sent++;
        }

        m_lastupdate = ticks;
        if(sent > 0)
        {
            assert(m_history.find(m_world->GetWorldID()) == m_history.end());
            m_history[m_world->GetWorldID()] = m_world->GetWorldState();
        }
        UpdateHistoryBuffer();
    }
}

void CServer::OnReceive(CStream* stream, CClientInfo* client)
{
    if(client->disconnected) // don't listen to this guy anymore
        return;

    uint8_t type = CNetMsg::ReadHeader(stream);
    switch(type)
    {
    case NET_MSG_CLIENT_CTRL:
        OnReceiveClientCtrl(stream, client);
        break;
    case NET_MSG_CLIENT_CHALLENGE:
        OnReceiveChallenge(stream, client);
        break;
    case NET_MSG_INVALID:
    default:
        fprintf(stderr, "Invalid client message\n");
        assert(0);
        break;
    }
}

void CServer::OnReceiveClientCtrl(CStream* stream, CClientInfo* client)
{
    if(!client->got_challenge) // don't accept this until we have a challenge msg
        return;

    uint32_t worldid;
    stream->ReadDWORD(&worldid);
    ClientHistoryACK(client, worldid);

    CObj* obj = m_world->GetObj(client->m_obj);
    assert(obj);
    if(!obj)
    {
        fprintf(stderr, "Invalid client message\n");
        return;
    }
    vec3_t origin, vel;
    stream->ReadVec3(&origin);
    stream->ReadVec3(&vel);
    stream->ReadFloat(&client->lat);
    stream->ReadFloat(&client->lon);
    if((origin - obj->GetOrigin()).AbsSquared() < MAX_SV_CL_POS_DIFF)
    {
        obj->SetOrigin(origin);
    }
    else
    {
        fprintf(stderr, "SV: Not accepting client position\n");
    }
    obj->SetVel(vel);

    assert(client->clcmdlist.size() < 30);
    uint16_t cmdcount;
    stream->ReadWORD(&cmdcount);
    client->clcmdlist.clear();
    for(int i=0;i<cmdcount;i++)
    {
        std::string cmd;
        stream->ReadString(&cmd);
        client->clcmdlist.push_back(cmd);
    }
}

void CServer::OnReceiveChallenge(CStream* stream, CClientInfo* client)
{
    std::string clientname;

    stream->ReadString(&clientname);

    // validate client name
    if(clientname.length() < 1 ||
       clientname.length() > MAX_CLIENT_NAME_LEN)
    {
        // bad client name
        fprintf(stderr, "Bad client name. Disconnecting client.\n");
        enet_peer_disconnect_later(client->GetPeer(), 0);
        client->disconnected = true;
        return;
    }
    client->name = clientname;
    client->got_challenge = true;

    fprintf(stderr, "SV: Accepting new player: %s.\n", clientname.c_str());

    // OK, so we like this client, now we send him the
    // CHALLENGE_OK msg, so he knows, he is in the game
    CStream responsestream;
    responsestream.SetSize(128); // 128 bytes for challenge_ok

    CNetMsg::WriteHeader(&responsestream, NET_MSG_CLIENT_CHALLENGE_OK);
    responsestream.WriteWORD(0);

    ENetPacket* packet;
    const uint32_t packetflags = ENET_PACKET_FLAG_RELIABLE;
    packet = enet_packet_create(responsestream.GetBuffer(),
                                responsestream.GetBytesWritten(),
                                packetflags);
    assert(packet);
    if(!packet)
        return;

    if(enet_peer_send(client->GetPeer(), 0, packet) != 0)
    {

    }
}

void CServer::UpdateHistoryBuffer()
{
    uint32_t lowestworldid = 0;
    CClientInfo* client;
    std::map<uint32_t, world_state_t>::iterator worlditer;
    std::map<int, CClientInfo*>::iterator clientiter;
    uint32_t worldtime;
    uint32_t curtime = m_world->GetLeveltime();

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
            fprintf(stderr, "Client world is too old (%i ms)\n", (int)(curtime - worldtime));
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
            //worlditer = m_history.erase(worlditer);
            m_history.erase(worlditer++);
        }
        else
        {
            ++worlditer;
        }
    }

    assert(m_history.size() < MAX_WORLD_BACKLOG);
    if(m_history.size() >= MAX_WORLD_BACKLOG)
    {
        fprintf(stderr, "Server History Buffer too large. Reset History Buffer.\n");
        m_history.clear();
    }
}

void CServer::ClientHistoryACK(CClientInfo* client, uint32_t worldid)
{
    if(client->worldidACK < worldid)
        client->worldidACK = worldid;
}

bool CServer::SendWorldToClient(CClientInfo* client)
{
    if(client->got_challenge == false) // don't send this client until auth'd
        return true;

    ENetPacket* packet;
    int localobj = client->m_obj;
    m_stream.ResetWritePosition();

    CNetMsg::WriteHeader(&m_stream, NET_MSG_SERIALIZE_WORLD); // Writing Header
    m_stream.WriteDWORD((uint32_t)localobj); // which object is linked to the player
    client->hud.Serialize(true, &m_stream, NULL); // player head up display information

    std::map<uint32_t, world_state_t>::iterator iter;
    iter = m_history.find(client->worldidACK);
    if(iter == m_history.end())
    {
        m_world->Serialize(true, &m_stream, NULL);
        fprintf(stderr, "NET: Full update. Bytes to be send: %i (MTU: %i)\n",
                m_stream.GetBytesWritten(),
                client->GetPeer()->mtu);
    }
    else
    {
        world_state_t diffstate = (*iter).second;
        if(!m_world->Serialize(true, &m_stream, &diffstate))
        {
            // No change since last update, client needs no update
            client->worldidACK = m_world->GetWorldID();
            return true;
        }
    }

    if(m_stream.GetWriteOverflow())
    {
        fprintf(stderr,
                "Failed to send packet to client. Pending data too large: %i\n",
                m_stream.GetBytesWritten());
        assert(0);
        return false;
    }

    const uint32_t packetflags = ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT;
    packet = enet_packet_create(m_stream.GetBuffer(),
                                m_stream.GetBytesWritten(),
                                packetflags);
    assert(packet);
    if(!packet)
        return false;

    return enet_peer_send(client->GetPeer(), 0, packet) == 0;
}

