#include "NetMsg.h"
#include "Client.h"
#include "math/mathconst.h"
#include <math.h>
#include "SDL.h"
#include "lynxsys.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#define CLIENT_UPDATERATE   50

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

bool CClient::Connect(char* server, int port)
{
	ENetAddress address;

	if(IsConnecting() || IsConnected())
		Shutdown();

	m_client = enet_host_create(NULL, 1, 0, 0);
	if(!m_client)
		return false;

	enet_address_set_host(&address, server);
	address.port = port;

	m_server = enet_host_connect(m_client, & address, 1);
	if(m_server == NULL)
	{
		Shutdown();
		return false;
	}
    m_isconnecting = true;

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

void CClient::Update(const float dt, const DWORD ticks)
{
	ENetEvent event;
	CStream stream;
	assert(m_client && m_server);

	while(m_client && enet_host_service(m_client, &event, 0) > 0)
	{
		switch (event.type)
		{
		case ENET_EVENT_TYPE_RECEIVE:
			//fprintf(stderr, "CL: A packet of length %u was received \n",
			//	event.packet->dataLength);

			stream.SetBuffer(event.packet->data,
							(int)event.packet->dataLength,
							(int)event.packet->dataLength);
			OnReceive(&stream);
			enet_packet_destroy(event.packet);

			break;

		case ENET_EVENT_TYPE_CONNECT:
			m_isconnecting = false;
			fprintf(stderr, "CL: Connected to server. \n");
			break;

		case ENET_EVENT_TYPE_DISCONNECT:
			fprintf(stderr, "CL: Disconnected\n");
			Shutdown();
			break;
		}
	}

    // Eingabe und Steuerung
    std::vector<std::string> clcmdlist;
    bool forcesend = false;
    InputGetCmdList(&clcmdlist, &forcesend);
    InputMouseMove(); // update m_lat and m_lon
    m_gamelogic->ClientMove(GetLocalController(), clcmdlist);

    // Sende Eingabe an Server
    SendClientState(clcmdlist, forcesend);
}

void CClient::SendClientState(const std::vector<std::string>& clcmdlist, bool forcesend)
{
    DWORD ticks = CLynxSys::GetTicks();
    if(!IsConnected() || m_world->GetBSP()->GetFilename() == "")
        return;

    if(ticks - m_lastupdate < CLIENT_UPDATERATE && !forcesend)
        return;

    ENetPacket* packet;
    std::vector<std::string>::iterator iter;
    CObj* localctrl = GetLocalController();
    CStream stream(1024);

    CNetMsg::WriteHeader(&stream, NET_MSG_CLIENT_CTRL); // Writing Header
	stream.WriteDWORD(m_world->GetWorldID());
    stream.WriteVec3(localctrl->GetOrigin());
    stream.WriteVec3(localctrl->GetVel());
    stream.WriteFloat(m_lat);
    stream.WriteFloat(m_lon);

    assert(clcmdlist.size() < USHRT_MAX);
    stream.WriteWORD((WORD)clcmdlist.size());
    for(size_t i=0;i<clcmdlist.size();i++)
        stream.WriteString(clcmdlist[i]);
    
	packet = enet_packet_create(stream.GetBuffer(), 
							    stream.GetBytesWritten(), 0);
    assert(packet);
    if(packet)
    {
        int success = enet_peer_send(m_server, 0, packet);
        assert(success == 0);
    }
    m_lastupdate = ticks;
}

void CClient::OnReceive(CStream* stream)
{
	BYTE type;
	WORD localobj;

	// fprintf(stderr, "%i Client Incoming data: %i bytes\n", CLynx::GetTicks()&255, stream->GetBytesToRead());

    type = CNetMsg::ReadHeader(stream);
    switch(type)
    {
    case NET_MSG_SERIALIZE_WORLD:
        stream->ReadWORD(&localobj);
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
	const float sensitivity = 0.5f; // FIXME
	int dx, dy;
	CLynxSys::GetMouseDelta(&dx, &dy);

	m_lat += (float)dy * sensitivity;
	if(m_lat >= 89)
		m_lat = 89;
	else if(m_lat <= -89)
		m_lat = -89;

	m_lon -= (float)dx * sensitivity;
	m_lon = CLynx::AngleMod(m_lon);

    quaternion_t qlat(vec3_t::xAxis, m_lat*lynxmath::DEGTORAD);
    quaternion_t qlon(vec3_t::yAxis, m_lon*lynxmath::DEGTORAD);
    GetLocalController()->SetRot(qlon*qlat);
}

void CClient::InputGetCmdList(std::vector<std::string>* clcmdlist, bool* forcesend)
{
	BYTE* keystate = CLynxSys::GetKeyState();    
    *forcesend = false;

    // Client fire animation prediction
    bool firedown = false;
    CModelMD2* model;
    md2_state_t* model_state;

    if(keystate[SDLK_UP] || keystate[SDLK_w])
        clcmdlist->push_back("+mf");
	if(keystate[SDLK_DOWN] || keystate[SDLK_s])
        clcmdlist->push_back("+mb");
    if(keystate[SDLK_LEFT] || keystate[SDLK_a])
        clcmdlist->push_back("+ml");
    if(keystate[SDLK_RIGHT] || keystate[SDLK_d])
        clcmdlist->push_back("+mr");
    if(keystate[SDLK_SPACE])
        clcmdlist->push_back("+jmp");
    if(keystate[SDLK_f] || CLynxSys::MouseLeftDown())
    {
        firedown = true;
        clcmdlist->push_back("+fire");

        // Weapon fire animation is client side only. Smells a bit like a hack, but works great
        m_world->m_hud.GetModel(&model, &model_state);
        if(model)
        {
            if(CModelMD2::GetAnimation(model_state) == HUD_WEAPON_IDLE_ANIMATION)
            {
                model->SetAnimation(model_state, HUD_WEAPON_FIRE_ANIMATION);
                model->SetNextAnimation(model_state, HUD_WEAPON_FIRE_ANIMATION);
            }
        }
    }
    if(keystate[SDLK_e])
    {
        // FIXME temp hack
        vec3_t pos = GetLocalController()->GetOrigin();
        fprintf(stderr, "[%.2f,%.2f,%.2f]\n", pos.x, pos.y, pos.z);
    }

    if(!firedown)
    {
        m_world->m_hud.GetModel(&model, &model_state);
        if(model && CModelMD2::GetAnimation(model_state) == HUD_WEAPON_FIRE_ANIMATION)
        {
            model->SetAnimation(model_state, HUD_WEAPON_IDLE_ANIMATION);
            model->SetNextAnimation(model_state, HUD_WEAPON_IDLE_ANIMATION);
        }
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