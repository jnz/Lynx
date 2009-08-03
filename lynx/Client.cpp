#include "NetMsg.h"
#include "Client.h"
#include "math/mathconst.h"
#include <math.h>
#include "SDL.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#define CLIENT_UPDATERATE   50

CClient::CClient(CWorldClient* world)
{
	m_world = world;
	enet_initialize();
	m_client = NULL;
	m_server = NULL;
	m_isconnecting = false;

	m_forward = 0;
	m_backward = 0;
	m_strafe_left = 0;
	m_strafe_right = 0;

    m_lastupdate = CLynx::GetTicks();
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

void CClient::Update(const float dt)
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

	// FIXME: no SDL code here
	int dx, dy;
	BYTE* keystate = CLynx::GetKeyState();
	CLynx::GetMouseDelta(&dx, &dy);
	m_forward = !!(keystate[SDLK_UP] | keystate[SDLK_w]);
	m_backward = !!(keystate[SDLK_DOWN] | keystate[SDLK_s]);
    m_strafe_left = !!(keystate[SDLK_LEFT] | keystate[SDLK_a]);
    m_strafe_right = !!(keystate[SDLK_RIGHT] | keystate[SDLK_d]);
    InputMouseMove(dx, dy);
    InputCalcDir();

	SendClientState();
}

void CClient::SendClientState()
{
    DWORD ticks = CLynx::GetTicks();
    if(ticks - m_lastupdate > CLIENT_UPDATERATE && IsConnected())
    {
        ENetPacket* packet;
        CObj* localctrl = GetLocalController();
        CStream stream(128);
        CNetMsg::WriteHeader(&stream, NET_MSG_CLIENT_CTRL); // Writing Header
		stream.WriteDWORD(m_world->GetWorldID());
        stream.WriteVec3(localctrl->GetOrigin());
        stream.WriteVec3(localctrl->GetVel());
        stream.WriteVec3(localctrl->GetRot());
        
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
}

void CClient::OnReceive(CStream* stream)
{
	BYTE type;
	DWORD localobj;

	fprintf(stderr, "Client Incoming data: %i bytes\n", stream->GetBytesToRead());

	type = CNetMsg::ReadHeader(stream);
	switch(type)
	{
	case NET_MSG_SERIALIZE_WORLD:
		stream->ReadDWORD(&localobj);
		m_world->Serialize(false, stream);
		m_world->SetLocalObj(localobj);
		break;
	case NET_MSG_INVALID:
	default:
		assert(0);
	}
}

void CClient::InputMouseMove(int dx, int dy)
{
	CObj* obj = GetLocalController();
	const float sensitivity = 0.5f; // FIXME
	vec3_t rot = obj->GetRot();

	rot.v[0] += (float)dy * sensitivity;
	if(rot.v[0] >= 89)
		rot.v[0] = 89;
	else if(rot.v[0] <= -89)
		rot.v[0] = -89;

	rot.v[1] -= (float)dx * sensitivity;
	rot.v[1] = CLynx::AngleMod(rot.v[1]);
	obj->SetRot(rot);
}

void CClient::InputCalcDir()
{
	vec3_t velocity, rot;
	vec3_t dir, side;
	vec3_t newdir(0,0,0);

    velocity = GetLocalController()->GetVel();
    rot = GetLocalController()->GetRot();
	vec3_t::AngleVec3(rot, &dir, NULL, &side);

	newdir += (float)m_forward * dir;
	newdir -= (float)m_backward * dir;
	newdir -= (float)m_strafe_left * side;
	newdir += (float)m_strafe_right * side;
	GetLocalController()->SetVel(newdir);
}

CObj* CClient::GetLocalController()
{
	return m_world->GetLocalController();
}

/*
	GRAVEYARD

sl sr  f  b |  dir (x, y, z)
 0  0  0  0   ( 0, 0, 0)	0
 0  0  0  1   ( 0, 0, 1)	1
 0  0  1  0   ( 0, 0,-1)	2
 0  0  1  1   ( 0, 0, 0)	3
 0  1  0  0   ( 1, 0, 0)	4
 0  1  0  1   ( S, 0, S)	5
 0  1  1  0   ( S, 0,-S)	6
 0  1  1  1   ( 1, 0, 0)	7
 1  0  0  0   (-1, 0, 0)	8
 1  0  0  1   (-S, 0, S)	9
 1  0  1  0   (-S, 0,-S)	10
 1  0  1  1   (-1, 0, 0)	11
 1  1  0  0   ( 0, 0, 0)	12
 1  1  0  1   ( 0, 0, 1)	13
 1  1  1  0   ( 0, 0,-1)	14
 1  1  1  1   ( 0, 0, 0)	15
*/
/*
#define S SQRT_2_HALF
static const vec3_t g_dir_table[] =
{
	vec3_t( 0, 0, 0),
	vec3_t( 0, 0, 1),
	vec3_t( 0, 0,-1),
	vec3_t( 0, 0, 0),
	vec3_t( 1, 0, 0),
	vec3_t( S, 0, S),
	vec3_t( S, 0,-S),
	vec3_t( 1, 0, 0),
	vec3_t(-1, 0, 0),
	vec3_t(-S, 0, S),
	vec3_t(-S, 0,-S),
	vec3_t(-1, 0, 0),
	vec3_t( 0, 0, 0),
	vec3_t( 0, 0, 1),
	vec3_t( 0, 0,-1),
	vec3_t( 0, 0, 0)
};
#undef S
*/
