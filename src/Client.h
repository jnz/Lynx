#pragma once

#include <enet/enet.h>
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
    ~CClient(void);

    bool Connect(char* server, int port);
    void Shutdown();

    bool IsConnecting() { return m_isconnecting && m_server; }
    bool IsConnected() { return m_server && !m_isconnecting; }
    bool IsRunning() { return (m_server != NULL); }

    void Update(const float dt, const DWORD ticks);

protected:
    void OnReceive(CStream* stream);

    void InputMouseMove(); // update m_lat and m_lon
    void InputGetCmdList(std::vector<std::string>* clcmdlist, bool* forcesend); // forcesend: gibt es cmds, die unbedingt sofort abgeschickt werden müssen
    void SendClientState(const std::vector<std::string>& clcmdlist, bool forcesend);
    CObj* GetLocalController(); // Geist Objekt, das nur auf Client Seite existiert. Sozusagen die virtuelle Kamera
    CObj* GetLocalObj(); // Tatsächliches Objekt mit dem der Client verknüpft ist

private:
    ENetHost* m_client;
    ENetPeer* m_server;
    bool m_isconnecting;

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

    DWORD m_lastupdate;
};
