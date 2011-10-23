#pragma once

#include "../enet/enet.h"
#include "WorldClient.h"
#include "GameLogic.h"

/*
    CClient kümmert sich um die Netzwerk-Verwaltung auf Client-Seite.
    CClient stellt die Daten für CWorldClient bereit.

    Aktuell kümmert sich CClient auch im den User-Input. Das soll
    allerdings noch in eine eigene Klasse ausgelagert werden.
 */


class CClient
{
public:
    CClient(CWorldClient* world, CGameLogic* gamelogic);
    ~CClient();

    bool Connect(const char* server, const int port);
    void Shutdown();

    bool IsConnecting() { return m_isconnecting && m_server; } // are we trying to connect to a server?
    bool IsConnected() { return m_server && !m_isconnecting; } // do we have a network connection
    bool IsRunning() { return (m_server != NULL); } // if false, we are no longer connected
    bool IsInGame() { return IsConnected() && m_world->GetBSP()->IsLoaded(); } // we are connected and playing on the server

    void Update(const float dt, const uint32_t ticks);

protected:
    void OnReceive(CStream* stream);

    void InputMouseMove(); // update m_lat and m_lon
    void InputGetCmdList(std::vector<std::string>* clcmdlist, bool* forcesend); // forcesend: are there commands to be send immediately
    void SendClientState(const std::vector<std::string>& clcmdlist, bool forcesend, uint32_t ticks);
    void SendChallenge(); // after connecting, we send a challenge message to the server
    CObj* GetLocalController(); // object that does only exist on the client side. a virtual camera.
    CObj* GetLocalObj(); // real game object connected to the player

private:
    ENetHost* m_client;
    ENetPeer* m_server;
    bool m_isconnecting;
    bool m_challenge_ok; // if this is true, the server accepted us for the game

    // Input
    int m_forward;
    int m_backward;
    int m_strafe_left;
    int m_strafe_right;
    int m_jump;
    float m_lat; // mouse dx
    float m_lon; // mouse dy

    CWorldClient* m_world;
    CGameLogic* m_gamelogic;

    uint32_t m_lastupdate; // last time we have sent the client state to the server

    // Client input config settings
    cvar_t* m_cfg_mouse_sensitivity;
    cvar_t* m_cfg_mouse_invert;

    // Rule of three
    CClient(const CClient&);
    CClient& operator=(const CClient&);
};
