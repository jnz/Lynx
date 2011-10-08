#include "NetMsg.h"
#include "Client.h"
#include "ServerClient.h"
#include "math/mathconst.h"
#include <math.h>
#include <SDL/SDL.h>
#include "lynxsys.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CClient::CClient(CWorldClient* world, CGameLogic* gamelogic)
{
    m_world = world;
    enet_initialize();
    m_client = NULL;
    m_server = NULL;
    m_isconnecting = false;

    m_gamelogic = gamelogic;

    m_lat = 0;
    m_lon = 0;

    m_lastupdate = CLynxSys::GetTicks();
}

CClient::~CClient(void)
{
    Shutdown();
    enet_deinitialize();
}

bool CClient::Connect(const char* server, const int port)
{
    ENetAddress address;

    if(IsConnecting() || IsConnected())
        Shutdown();

    // channel limit 2
    m_client = enet_host_create(NULL, 1, 2, 0, OUTGOING_BANDWIDTH);
    if(!m_client)
        return false;

#ifdef USE_RANGE_ENCODER
    enet_host_compress_with_range_coder(m_client);
#endif

    enet_address_set_host(&address, server);
    address.port = port;

    m_server = enet_host_connect(m_client, & address, 1, 0);
    if(m_server == NULL)
    {
        Shutdown();
        return false;
    }
    m_isconnecting = true;

    // Precaching important resources while connecting
    m_gamelogic->Precache(m_world->GetResourceManager());

    // Load settings from config file
    m_cfg_mouse_sensitivity = CLynx::cfg.GetVarFloat("m_sensitiviy", 3.0f, true);
    m_cfg_mouse_invert = CLynx::cfg.GetVarFloat("m_invert", 0.0f, true);

    return true;
}

void CClient::Shutdown()
{
    if(IsConnected())
        enet_peer_disconnect_now(m_server, 0);

    if(m_server)
    {
        enet_peer_reset(m_server);
        m_server = NULL;
    }
    if(m_client)
    {
        enet_host_destroy(m_client);
        m_client = NULL;
    }
    m_isconnecting = false;
}

void CClient::Update(const float dt, const uint32_t ticks)
{
    ENetEvent event;
    CStream stream;
    assert(m_client && m_server);
    int result=0;

    while(m_client && (result = enet_host_service(m_client, &event, 0)) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_RECEIVE:
            //if(CLynxSys::GetKeyState()[SDLK_g])
                //fprintf(stderr, "CL: A packet of length %u was received \n",
                    //event.packet->dataLength);

            //if(rand()%5 == 2)
            //{
                //fprintf(stderr, "fake packet loss\n");
                //continue;
            //}
            fprintf(stderr, "ChannelID: %i", event.channelID);

            stream.SetBuffer(event.packet->data,
                             event.packet->dataLength,
                             event.packet->dataLength);
            if(event.channelID == 0)
            {
                OnReceive(&stream);
            }
            else
            {
                assert(0);
            }

            enet_packet_destroy(event.packet);

            break;

        case ENET_EVENT_TYPE_CONNECT:
            m_isconnecting = false;
            fprintf(stderr, "CL: Connected to server.\n");
            break;

        case ENET_EVENT_TYPE_DISCONNECT:
            fprintf(stderr, "CL: Disconnected\n");
            Shutdown();
            break;
        case ENET_EVENT_TYPE_NONE:
            break;
        }
    }
    // result 1 happens, when too much data is being sent
    if(result == -1)
        return;

    // Input and control
    std::vector<std::string> clcmdlist;
    bool forcesend = false;
    InputGetCmdList(&clcmdlist, &forcesend);
    InputMouseMove(); // update m_lat and m_lon
    m_gamelogic->ClientMove(GetLocalController(), clcmdlist);

    // Send input to server
    SendClientState(clcmdlist, forcesend, ticks);
}

void CClient::SendClientState(const std::vector<std::string>& clcmdlist, bool forcesend, uint32_t ticks)
{
    size_t i;
    if(!IsConnected() || m_world->GetBSP()->GetFilename() == "")
        return;

    if(forcesend)
        fprintf(stderr, "CL: Force client state send\n");

    if((ticks - m_lastupdate) < CLIENT_UPDATERATE && !forcesend)
        return;

    ENetPacket* packet;
    CObj* localctrl = GetLocalController();
    CStream stream(MAX_CL_PACKETLEN);

    CNetMsg::WriteHeader(&stream, NET_MSG_CLIENT_CTRL); // Writing Header
    stream.WriteDWORD(m_world->GetWorldID());
    stream.WriteVec3(localctrl->GetOrigin());
    stream.WriteVec3(localctrl->GetVel());
    stream.WriteFloat(m_lat);
    stream.WriteFloat(m_lon);

    assert(clcmdlist.size() < USHRT_MAX);
    stream.WriteWORD((uint16_t)clcmdlist.size());
    for(i=0;i<clcmdlist.size();i++)
        stream.WriteString(clcmdlist[i]);

    if(stream.GetWriteOverflow())
    {
        fprintf(stderr, "CL: Packet size too large.\n");
        assert(0);
        return;
    }

    const uint32_t packetflags = ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT;
    packet = enet_packet_create(stream.GetBuffer(),
                                stream.GetBytesWritten(),
                                packetflags);
    assert(packet);
    if(packet)
    {
        int success = enet_peer_send(m_server, 0, packet);
        if(success != 0)
        {
            fprintf(stdout, "Failed to send packet\n");
            assert(success == 0);
        }
    }
    m_lastupdate = ticks;
}

void CClient::OnReceive(CStream* stream)
{
    uint8_t type;
    uint32_t localobj;

    // fprintf(stderr, "%i Client Incoming data: %i bytes\n", CLynx::GetTicks()&255, stream->GetBytesToRead());

    type = CNetMsg::ReadHeader(stream);
    switch(type)
    {
    case NET_MSG_SERIALIZE_WORLD:
        stream->ReadDWORD(&localobj);
        m_world->m_hud.Serialize(false, stream, m_world->GetResourceManager());
        m_world->Serialize(false, stream);
        m_world->SetLocalObj(localobj);
        break;
    case NET_MSG_INVALID:
    default:
        assert(0);
    }
}

void CClient::InputMouseMove()
{
    CObj* obj = GetLocalController();
    const float sensitivity = m_cfg_mouse_sensitivity->value/3.0f * 0.5f;
    const float invert = (m_cfg_mouse_invert->value != 0.0f) ? -1.0f : 1.0f;
    int dx, dy;
    CLynxSys::GetMouseDelta(&dx, &dy);

    m_lat -= (float)dy * sensitivity * invert;
    if(m_lat >= 89)
        m_lat = 89;
    else if(m_lat <= -89)
        m_lat = -89;

    m_lon -= (float)dx * sensitivity;
    m_lon = CLynx::AngleMod(m_lon);

    quaternion_t qlat(vec3_t::xAxis, m_lat*lynxmath::DEGTORAD);
    quaternion_t qlon(vec3_t::yAxis, m_lon*lynxmath::DEGTORAD);
    obj->SetRot(qlon*qlat);
}

void CClient::InputGetCmdList(std::vector<std::string>* clcmdlist, bool* forcesend)
{
    uint8_t* keystate = CLynxSys::GetKeyState();
    *forcesend = false;
    bool firedown = false;

    /*
     * keystate[key] is 0 if not pressed.
     * keystate[key] is 1 if pressed once.
     * keystate[key] is > 1 if auto-key repeat is active.
     */

    if(keystate[SDLK_w])
        clcmdlist->push_back("+mf");
    if(keystate[SDLK_s])
        clcmdlist->push_back("+mb");
    if(keystate[SDLK_a])
        clcmdlist->push_back("+ml");
    if(keystate[SDLK_d])
        clcmdlist->push_back("+mr");
    if(keystate[SDLK_SPACE])
        clcmdlist->push_back("+jmp");
    if(keystate[SDLK_f] || CLynxSys::MouseLeftDown())
    {
        firedown = true;
        clcmdlist->push_back("+fire");
    }
    if(keystate[SDLK_1])
    {
        clcmdlist->push_back("w_rocket");
    }
    if(keystate[SDLK_2])
    {
        clcmdlist->push_back("w_gun");
    }
    if(keystate[SDLK_e] == 1) // key down event
    {
        // Print some stats to the console
        const vec3_t pos = GetLocalController()->GetOrigin();
        const quaternion_t orient = GetLocalController()->GetRot();
        fprintf(stderr, "POS: (%.2f,%.2f,%.2f)\n", pos.x, pos.y, pos.z);
        fprintf(stderr, "ROT: (%.2f,%.2f,%.2f,%.2f)\n", orient.x, orient.y, orient.z, orient.w);
        fprintf(stderr, "Network stats:\n");
        fprintf(stderr, "Mean round trip time: %i (msec)\n", m_server->roundTripTime);
        fprintf(stderr, "Incoming data total: %i (kbytes)\n", m_client->totalReceivedData/1024);
        fprintf(stderr, "Outgoing data total: %i (kbytes)\n", m_client->totalSentData/1024);
        fprintf(stderr, "Incoming packets total: %i\n", m_client->totalReceivedPackets);
        fprintf(stderr, "Outgoing data total: %i\n", m_client->totalSentPackets);
    }
    if(keystate[SDLK_r] == 1) // debug method: respawn local player
    {
        bspbin_spawn_t spawn = m_world->GetBSP()->GetRandomSpawnPoint();
        GetLocalController()->SetOrigin(spawn.point);
    }
}

CObj* CClient::GetLocalController()
{
    return m_world->GetLocalController();
}

CObj* CClient::GetLocalObj()
{
    return m_world->GetLocalObj();
}
